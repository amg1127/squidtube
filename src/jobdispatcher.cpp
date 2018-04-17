#include "jobdispatcher.h"

void JobDispatcher::jobWorkerFinished () {
    this->numJobCarriers--;
    if (this->numJobCarriers == 0) {
        qDebug ("Finishing all request workers...");
        JobCarrier* jobCarrier;
        while (! this->jobCarriers.isEmpty ()) {
            jobCarrier = this->jobCarriers.takeLast ();
            if (jobCarrier != Q_NULLPTR) {
                delete (jobCarrier);
            }
        }
        qDebug ("All request workers finished.");
        emit finished();
    }
}

void JobDispatcher::stdinReaderFinished () {
    if (this->numJobCarriers) {
        emit finishWorkers ();
    } else {
        qDebug ("There is no worker instance running. Immediate finish will take place.");
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
    std::cout << QString("%1%2 message=%3 log=%3").arg(
        msgpart1,
        ((isError) ? ((AppRuntime::squidProtocol.compare("3.0")) ? "BH" : "ERR") : ((isMatch) ? "OK" : "ERR")),
        QString::fromUtf8 (QUrl::toPercentEncoding (msg)))
        .toLocal8Bit().constData() << std::endl;
    if (isError) {
        qWarning ("An internal error occurred while processing the request%s: %s",
            msgpart2.toLatin1().constData(),
            msg.toLatin1().constData());
    } else {
        qInfo ("Channel #%s answered: isMatch=%s ; message='%s'",
            channel.toLatin1().constData(),
            ((isMatch) ? "true" : "false"),
            msg.toLatin1().constData());
    }
}

void JobDispatcher::squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData) {
    if (requestChannelNumber >= 0 && requestUrl.isValid() && (! requestData.isEmpty())) {
        AppJobRequestFromSquid* request = new AppJobRequestFromSquid ();
        request->data.url = requestUrl;
        request->data.criteria = requestData;
        request->data.mathMatchOperator = OperatorString;
        request->data.caseSensitivity = Qt::CaseSensitive;
        request->data.patternSyntax = QRegExp::RegExp;
        request->data.invertMatch = false;
        // Evaluate and transform the property that the administrator wants to compare
        QStringList propertyItems (QUrl::fromPercentEncoding(request->data.criteria.takeFirst().toUtf8()).split(".", QString::KeepEmptyParts));
        // Dear supporter of the website https://regexr.com/ : thank you for your site. It helped me a lot!
        static QRegExp objectPropertyMatch ("(([A-Za-z_]\\w*)|\\[\"(\\S+)\"\\])");
        static QRegExp arrayPropertyMatch ("([<\\[])(|(-?\\d+(|:-?\\d+))(,-?\\d+(|:-?\\d+))*)([\\]>])");
#ifndef QT_NO_DEBUG
        // A sanity check...
        if (! objectPropertyMatch.isValid()) {
            qFatal ("'objectPropertyMatch' regular expression did not compile: '%s'", objectPropertyMatch.errorString().toLocal8Bit().constData());
        }
        if (! arrayPropertyMatch.isValid()) {
            qFatal ("'arrayPropertyMatch' regular expression did not compile: '%s'", arrayPropertyMatch.errorString().toLocal8Bit().constData());
        }
#endif
        QStringList propertyCapturedItems;
        for (QStringList::iterator token = propertyItems.begin(); token != propertyItems.end(); token++) {
            if (objectPropertyMatch.exactMatch (*token)) {
                propertyCapturedItems = objectPropertyMatch.capturedTexts ();
                AppSquidPropertyMatch propertyMatch;
                propertyMatch.matchType = MatchObject;
                propertyMatch.componentName = propertyCapturedItems.value (2);
                if (propertyMatch.componentName.isEmpty ()) {
                    propertyMatch.componentName = QUrl::fromPercentEncoding(propertyCapturedItems.value(3).toUtf8());
                }
                request->data.properties.append (propertyMatch);
            } else if (arrayPropertyMatch.exactMatch (*token)) {
                propertyCapturedItems = arrayPropertyMatch.capturedTexts ();
                AppSquidPropertyMatch propertyMatch;
                propertyMatch.matchType = MatchArray;
                propertyMatch.componentName = propertyCapturedItems.value(1) + propertyCapturedItems.value(7);
                if (propertyMatch.componentName == "<>") {
                    propertyMatch.matchQuantity = MatchAll;
                } else if (propertyMatch.componentName == "[]") {
                    propertyMatch.matchQuantity = MatchAny;
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
                request->data.properties.append (propertyMatch);
            } else {
                this->writeAnswerLine (requestChannel, "ACL has an invalid property syntax", true, false);
                return;
            }
        }
        // Decode the tokens
        for (QStringList::iterator token = request->data.criteria.begin(); token != request->data.criteria.end(); token++) {
            (*token) = QUrl::fromPercentEncoding (token->toUtf8());
        }
        // Will the match be negated?
        if (! request->data.criteria.isEmpty ()) {
            if (request->data.criteria[0] == "!") {
                request->data.invertMatch = true;
                request->data.criteria.removeFirst ();
            }
        }
        // Check the flags that may have set by the administrator
        short stringMatch = 0;
        short numericMatch = 0;
        while (! request->data.criteria.isEmpty ()) {
            QString requestFlag (request->data.criteria.takeFirst());
            if (requestFlag == "<" || requestFlag == "-lt") {
                request->data.mathMatchOperator = OperatorLessThan;
                numericMatch++;
            } else if (requestFlag == "<=" || requestFlag == "-le") {
                request->data.mathMatchOperator = OperatorLessThanOrEquals;
                numericMatch++;
            } else if (requestFlag == "=" || requestFlag == "==" || requestFlag == "-eq") {
                request->data.mathMatchOperator = OperatorEquals;
                numericMatch++;
            } else if (requestFlag == "<>" || requestFlag == "!=" || requestFlag == "-ne") {
                request->data.mathMatchOperator = OperatorNotEquals;
                numericMatch++;
            } else if (requestFlag == ">" || requestFlag == "-gt") {
                request->data.mathMatchOperator = OperatorGreaterThan;
                numericMatch++;
            } else if (requestFlag == ">=" || requestFlag == "-ge") {
                request->data.mathMatchOperator = OperatorGreaterThanOrEquals;
                numericMatch++;
            } else if (requestFlag.left(1) == "-") {
                if (requestFlag == "-f" || requestFlag == "--fixed") {
                    if (request->data.patternSyntax == QRegExp::RegExp) {
                        request->data.patternSyntax = QRegExp::FixedString;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags", true, false);
                        return;
                    }
                } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                    if (request->data.patternSyntax == QRegExp::RegExp) {
                        request->data.patternSyntax = QRegExp::WildcardUnix;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags", true, false);
                        return;
                    }
                } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                    if (request->data.caseSensitivity == Qt::CaseSensitive) {
                        request->data.caseSensitivity = Qt::CaseInsensitive;
                        stringMatch++;
                    } else {
                        this->writeAnswerLine (requestChannel, "ACL specifies incompatible string matching flags", true, false);
                        return;
                    }
                } else if (requestFlag == "--") {
                    // No more flags to look for
                    break;
                } else {
                    this->writeAnswerLine (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag, true, false);
                    return;
                }
            } else {
                // No more flags to look for
                request->data.criteria.prepend (requestFlag);
                break;
            }
            if (numericMatch > 1) {
                this->writeAnswerLine (requestChannel, "More than one comparison operator is not allowed", true, false);
                return;
            }
        }
        // Now, send the request to a worker
        // Create if it does not exists yet
        while (requestChannelNumber >= this->jobCarriers.count()) {
            this->jobCarriers.append (Q_NULLPTR);
        }
        JobCarrier* carrier = this->jobCarriers[requestChannelNumber];
        if (carrier == Q_NULLPTR) {
            qInfo ("Creating a new handler for channel #%d...", requestChannelNumber);
            carrier = new JobCarrier (requestChannel, this);
            QObject::connect (carrier->worker(), &JobWorker::writeAnswerLine, this, &JobDispatcher::writeAnswerLine, Qt::QueuedConnection);
            QObject::connect (this, &JobDispatcher::finishWorkers, carrier->worker(), &JobWorker::quit, Qt::QueuedConnection);
            QObject::connect (carrier, &JobCarrier::finished, this, &JobDispatcher::jobWorkerFinished);
            this->jobCarriers[requestChannelNumber] = carrier;
            this->numJobCarriers++;
            carrier->start ();
        }
        carrier->squidRequestIn (request, this->currentTimestamp);
    } else {
        qFatal ("Invalid procedure call: either 'requestChannelNumber' is a negative number, 'requestUrl' is invalid or 'requestData' is empty!");
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
        qFatal ("Program tried to destruct a JobDispatcher unexpectedly!");
    }
}

void JobDispatcher::start (QThread::Priority priority) {
    if (this->started) {
        qFatal ("Invalid procedure call: this method must be called only once!");
    } else {
        this->setCurrentTimestamp ();
        this->clockTimer->start ();
        this->stdinReader.start (priority);
        this->started = true;
    }
}
