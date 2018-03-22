#include "jobdispatcher.h"

void JobDispatcher::jobWorkerFinished () {
    this->numJobCarriers--;
    if (this->numJobCarriers == 0) {
        qDebug() << "Finishing all request workers...";
        JobCarrier* jobCarrier;
        while (! this->jobCarriers.isEmpty ()) {
            jobCarrier = this->jobCarriers.takeLast ();
            if (jobCarrier != Q_NULLPTR) {
                delete (jobCarrier);
            }
        }
        qDebug() << "All request workers finished.";
        emit finished();
    }
}

void JobDispatcher::stdinReaderFinished () {
    if (this->numJobCarriers) {
        emit finishWorkers ();
    } else {
        qDebug() << "There is no worker instances running. Immediate finish will take place.";
        emit finished ();
    }
}

void JobDispatcher::writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch) {
    QString msgpart1("");
    QString msgpart2("");
    if (! channel.isEmpty()) {
        msgpart1 += channel + " ";
        msgpart2 += QString(" from channel #") + channel;
    }
    std::cout << QString("%1%2 message=%3 log=%3")
        .arg(msgpart1)
        .arg((isError) ? "BH" : ((isMatch) ? "OK" : "ERR"))
        .arg(QString::fromUtf8(QUrl::toPercentEncoding (msg)))
        .toLocal8Bit().constData() << std::endl;
    if (isError) {
        qWarning() << QString("An internal error occurred while processing the request") + msgpart2 + ": " + msg;
    } else {
        qInfo() << QString("Channel #%1 answered: isMatch=%2 ; message='%3'").arg(channel).arg((isMatch) ? "true" : "false").arg(msg);
    }
}

void JobDispatcher::squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData) {
    if (requestChannelNumber >= 0 && requestUrl.isValid() && (! requestData.isEmpty())) {
        AppSquidRequest squidRequest;
        squidRequest.requestUrl = requestUrl;
        squidRequest.requestCriteria = requestData;
        squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::String;
        squidRequest.requestCaseSensitivity = Qt::CaseSensitive;
        squidRequest.requestPatternSyntax = QRegExp::RegExp;
        squidRequest.requestInvertMatch = false;
        // Evaluate and transform the property that the administrator wants to compare
        QStringList propertyItems (QUrl::fromPercentEncoding(squidRequest.requestCriteria.takeFirst().toUtf8()).split(".", QString::KeepEmptyParts));
        // Dear supporter of the website https://regexr.com/ : thank you for your site. It helped me a lot!
        static QRegExp objectPropertyMatch ("(([A-Za-z_]\\w*)|\\[\"(\\S+)\"\\])");
        static QRegExp arrayPropertyMatch ("([<\\[])(|(-?\\d+(|:-?\\d+))(,-?\\d+(|:-?\\d+))*)([\\]>])");
#ifndef QT_NO_DEBUG
        // A sanity check...
        if (! objectPropertyMatch.isValid()) {
            qFatal(QString("'objectPropertyMatch' regular expression did not compile: '%1'").arg(objectPropertyMatch.errorString()).toLocal8Bit().constData());
        }
        if (! arrayPropertyMatch.isValid()) {
            qFatal(QString("'arrayPropertyMatch' regular expression did not compile: '%1'").arg(arrayPropertyMatch.errorString()).toLocal8Bit().constData());
        }
#endif
        QStringList propertyCapturedItems;
        for (QStringList::iterator token = propertyItems.begin(); token != propertyItems.end(); token++) {
            if (objectPropertyMatch.exactMatch (*token)) {
                AppSquidPropertyMatch propertyMatch;
                propertyMatch.matchType = PropertyMatchType::MatchObject;
                propertyCapturedItems = propertyItemMatch.capturedTexts ();
                propertyMatch.componentName = propertyCapturedItems.value (2);
                if (propertyMatch.componentName.isEmpty ()) {
                    propertyMatch.componentName = QUrl::fromPercentEncoding(propertyCapturedItems.value(3).toUtf8());
                }
                squidRequest.requestProperties.append (propertyMatch);
            } else if (arrayPropertyMatch.exactMatch (*token)) {
                AppSquidPropertyMatch propertyMatch;
                propertyMatch.matchType = PropertyMatchType::MatchArray;
                propertyCapturedItems = arrayPropertyMatch.capturedTexts ();
                if ((propertyCapturedItems.value(1) + propertyCapturedItems.value(7)) == "<>") {
                    propertyMatch.matchQuantity = PropertyMatchQuantity::MatchAll;
                } else if ((propertyCapturedItems.value(1) + propertyCapturedItems.value(7)) == "[]") {
                    propertyMatch.matchQuantity = PropertyMatchQuantity::MatchAny;
                } else {
                    this->writeAnswerLine (requestChannel, "ACL interval evaluation specification is not valid", true, false);
                    return;
                }
                if (propertyCapturedItems.value(2).isEmpty ()) {
                    propertyMatch.matchIntervals.append (QPair<int,int> (0, -1));
                } else {
                    QStringList matchIntervalSpecs = propertyCapturedItems.value(2).split(",");
                    for (QStringList::const_iterator matchIntervalSpec = matchIntervalSpecs.constBegin(); matchIntervalSpec != matchIntervalSpecs.constEnd(); matchIntervalSpec++) {
                        QStringList matchIntervalPair = matchIntervalSpec->split(":");
                        if (matchIntervalPair.count() == 2) {
                            propertyMatch.matchIntervals.append (QPair<int,int> (matchIntervalPair.value(0).toInt(), matchIntervalPair.value(1).toInt()));
                        } else {
                            int val = matchIntervalPair.value(0).toInt();
                            propertyMatch.matchIntervals.append (QPair<int,int> (val, val));
                        }
                    }
                }
                squidRequest.requestProperties.append (propertyMatch);
            } else {
                this->writeAnswerLine (requestChannel, "ACL has an invalid property syntax", true, false);
                return;
            }
        }
        // Decode the tokens
        for (QStringList::iterator token = squidRequest.requestCriteria.begin(); token != squidRequest.requestCriteria.end(); token++) {
            (*token) = QUrl::fromPercentEncoding (token->toUtf8());
        }
        // Will the match be negated?
        if (! squidRequest.requestCriteria.isEmpty ()) {
            if (squidRequest.requestCriteria[0] == "!") {
                squidRequest.requestInvertMatch = true;
                squidRequest.requestCriteria.removeFirst ();
            }
        }
        // Check the flags that may have set by the administrator
        short stringMatch = 0;
        short numericMatch = 0;
        while (! squidRequest.requestCriteria.isEmpty ()) {
            QString requestFlag (squidRequest.requestCriteria.takeFirst());
            if (requestFlag == "<" || requestFlag == "-lt") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::LessThan;
                numericMatch++;
            } else if (requestFlag == "<=" || requestFlag == "-le") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::LessThanOrEquals;
                numericMatch++;
            } else if (requestFlag == "=" || requestFlag == "==" || requestFlag == "-eq") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::Equals;
                numericMatch++;
            } else if (requestFlag == "<>" || requestFlag == "!=" || requestFlag == "-ne") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::NotEquals;
                numericMatch++;
            } else if (requestFlag == ">" || requestFlag == "-gt") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::GreaterThan;
                numericMatch++;
            } else if (requestFlag == ">=" || requestFlag == "-ge") {
                squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::GreaterThanOrEquals;
                numericMatch++;
            } else if (requestFlag.left(1) == "-") {
                if (requestFlag == "-f" || requestFlag == "--fixed") {
                    if (squidRequest.requestPatternSyntax == QRegExp::RegExp) {
                        squidRequest.requestPatternSyntax = QRegExp::FixedString;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags!", true, false);
                        return;
                    }
                } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                    if (squidRequest.requestPatternSyntax == QRegExp::RegExp) {
                        squidRequest.requestPatternSyntax = QRegExp::WildcardUnix;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags!", true, false);
                        return;
                    }
                } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                    if (squidRequest.requestCaseSensitivity == Qt::CaseSensitive) {
                        squidRequest.requestCaseSensitivity = Qt::CaseInsensitive;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags!", true, false);
                        return;
                    }
                } else {
                    this->writeAnswerLine (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag, true, false);
                    return;
                }
            } else {
                // No more flags to look for
                squidRequest.requestCriteria.prepend (requestFlag);
                break;
            }
            if (numericMatch > 1) {
                this->writeAnswerLine (requestChannel, "More than one mathematic operator is not allowed", true, false);
                return;
            }
        }
        if (stringMatch && numericMatch) {
            this->writeAnswerLine (requestChannel, "ACL specifies an invalid combination of number and string flags", true, false);
            return;
        }
        // Now, send the request to a worker
        // Create if it does not exists yet
        while (requestChannelNumber >= this->jobCarriers.count()) {
            this->jobCarriers.append (Q_NULLPTR);
        }
        JobCarrier* carrier = this->jobCarriers[requestChannelNumber];
        if (carrier == Q_NULLPTR) {
            qInfo() << QString("Creating a new handler for channel #%1...").arg(requestChannelNumber);
            carrier = new JobCarrier (requestChannel, this);
            QObject::connect (carrier->worker(), &JobWorker::writeAnswerLine, this, &JobDispatcher::writeAnswerLine, Qt::QueuedConnection);
            QObject::connect (this, &JobDispatcher::finishWorkers, carrier->worker(), &JobWorker::quit, Qt::QueuedConnection);
            QObject::connect (carrier, &JobCarrier::finished, this, &JobDispatcher::jobWorkerFinished);
            this->jobCarriers[requestChannelNumber] = carrier;
            this->numJobCarriers++;
            carrier->start ();
        }
        carrier->squidRequestIn (squidRequest, this->currentTimestamp);
    } else {
        qFatal("Invalid procedure call!");
    }
}

void JobDispatcher::setCurrentTimestamp () {
    this->currentTimestamp = AppRuntime::currentDateTime().toMSecsSinceEpoch() / 1000;
}

JobDispatcher::JobDispatcher (QObject *parent) :
    QObject (parent),
    started (false),
    numJobCarriers (0),
    clockTimer (new QTimer (this)),
    currentTimestamp (0) {
    this->clockTimer->setSingleShot (false);
    this->clockTimer->setInterval (AppConstants::AppHelperTimerTimeout);
    QObject::connect (&this->stdinReader, &StdinReader::finished, this, &JobDispatcher::stdinReaderFinished);
    QObject::connect (&this->stdinReader, &StdinReader::writeAnswerLine, this, &JobDispatcher::writeAnswerLine, Qt::QueuedConnection);
    QObject::connect (&this->stdinReader, &StdinReader::squidRequest, this, &JobDispatcher::squidRequest, Qt::QueuedConnection);
    QObject::connect (this->clockTimer, &QTimer::timeout, this, &JobDispatcher::setCurrentTimestamp);
}

JobDispatcher::~JobDispatcher () {
    if (this->stdinReader.isRunning () || this->numJobCarriers || (! this->jobCarriers.isEmpty ())) {
        qFatal("Program tried to destruct a JobDispatcher unexpectedly!");
    }
}

void JobDispatcher::start (QThread::Priority priority) {
    if (this->started) {
        qFatal("Invalid procedure call! This method must be called only once!");
    } else {
        this->setCurrentTimestamp ();
        this->clockTimer->start ();
        this->stdinReader.start (priority);
        this->started = true;
    }
}
