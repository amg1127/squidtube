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
        // Evaluate and transform the property that the administrator wants to compare
        QStringList propertyItems (QUrl::fromPercentEncoding(squidRequest.requestCriteria.takeFirst().toUtf8()).split(".", QString::KeepEmptyParts));
        // Dear supporter of the website https://regexr.com/ : thank you for your site. It helped me a lot!
        static QRegExp propertyItemMatch("(|\\[(none|any|all)(|:(-?\\d+)\\.\\.?(-?\\d+))\\])(|[a-zA-Z_]\\w*)(|\\[(none|any|all)(|:(-?\\d+)\\.\\.?(-?\\d+))\\])");
#ifndef QT_NO_DEBUG
        // A sanity check...
        if (! propertyItemMatch.isValid()) {
            qFatal(QString("'propertyItemMatch' regular expression did not compile: '%1'").arg(propertyItemMatch.errorString()).toLocal8Bit().constData());
        }
#endif
        QStringList propertyCapturedItems;
        int intervalSpecOffset;
        for (QStringList::iterator token = propertyItems.begin(); token != propertyItems.end(); token++) {
            if (propertyItemMatch.exactMatch (*token)) {
                propertyCapturedItems = propertyItemMatch.capturedTexts ();
#ifndef QT_NO_DEBUG
                // Another sanity check...
                if (propertyCapturedItems.count() != 12) {
                    qFatal(QString("Unexpected number of captured groups: %1").arg(propertyCapturedItems.count()).toLocal8Bit().constData());
                }
#endif
                if (! propertyCapturedItems[1].isEmpty ()) {
                    intervalSpecOffset = 0;
                    if (! propertyCapturedItems[7].isEmpty ()) {
                        this->writeAnswerLine (requestChannel, "ACL property syntax is not valid.", true, false);
                        return;
                    }
                } else {
                    intervalSpecOffset = 6;
                }
                AppSquidPropertyMatch propertyMatch;
                propertyMatch.matchName = propertyCapturedItems[6];
                propertyMatch.matchFrom = 0;
                propertyMatch.matchTo = -1;
                propertyMatch.matchQuantity = PropertyMatchQuantity::MatchAll;
                if (! propertyCapturedItems[intervalSpecOffset + 1].isEmpty()) {
                    if (propertyCapturedItems[intervalSpecOffset + 2] == "none") {
                        propertyMatch.matchQuantity = PropertyMatchQuantity::MatchNone;
                    } else if (propertyCapturedItems[intervalSpecOffset + 2] == "any") {
                        propertyMatch.matchQuantity = PropertyMatchQuantity::MatchAny;
                    }
                    if (! propertyCapturedItems[intervalSpecOffset + 3].isEmpty()) {
                        propertyMatch.matchFrom = propertyCapturedItems[intervalSpecOffset + 4].toInt();
                        propertyMatch.matchTo = propertyCapturedItems[intervalSpecOffset + 5].toInt();
                    }
                }
                squidRequest.requestProperties.append (propertyMatch);
            } else {
                this->writeAnswerLine (requestChannel, "ACL has an invalid property syntax.", true, false);
                return;
            }
        }
        if (squidRequest.requestProperties.first().matchName.isEmpty()) {
            this->writeAnswerLine (requestChannel, "The first property token does not have a valid variable name!", true, false);
            return;
        }
        // Decode the tokens
        for (QStringList::iterator token = squidRequest.requestCriteria.begin(); token != squidRequest.requestCriteria.end(); token++) {
            (*token) = QUrl::fromPercentEncoding (token->toUtf8());
        }
        // Check the flags that may have set by the administrator
        bool stringMatch = false;
        bool numericMatch = false;
        while (! squidRequest.requestCriteria.isEmpty ()) {
            QString requestFlag (squidRequest.requestCriteria.takeFirst());
            if (requestFlag.left(1) == "-") {
                if (requestFlag == "-f" || requestFlag == "--fixed") {
                    squidRequest.requestPatternSyntax = QRegExp::FixedString;
                    stringMatch = true;
                } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                    squidRequest.requestPatternSyntax = QRegExp::WildcardUnix;
                    stringMatch = true;
                } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                    squidRequest.requestCaseSensitivity = Qt::CaseInsensitive;
                    stringMatch = true;
                } else if (requestFlag == "-<" || requestFlag == "-lt" || requestFlag == "--lessthan") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::LessThan;
                    numericMatch = true;
                } else if (requestFlag == "-<=" || requestFlag == "-le" || requestFlag == "--lessthanorequals") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::LessThanOrEquals;
                    numericMatch = true;
                } else if (requestFlag == "-=" || requestFlag == "-eq" || requestFlag == "--equals") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::Equals;
                    numericMatch = true;
                } else if (requestFlag == "-<>" || requestFlag == "-!=" || requestFlag == "-ne" || requestFlag == "--notequals") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::NotEquals;
                    numericMatch = true;
                } else if (requestFlag == "->" || requestFlag == "-gt" || requestFlag == "--greaterthan") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::GreaterThan;
                    numericMatch = true;
                } else if (requestFlag == "->=" || requestFlag == "-ge" || requestFlag == "--greaterthanorequals") {
                    squidRequest.requestMathMatchOperator = AppSquidMathMatchOperator::GreaterThanOrEquals;
                    numericMatch = true;
                } else {
                    this->writeAnswerLine (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag, true, false);
                    return;
                }
                if (stringMatch && numericMatch) {
                    this->writeAnswerLine (requestChannel, "ACL specifies an invalid combination of number and string flags", true, false);
                    return;
                }
            } else {
                // No more flags to look for
                squidRequest.requestCriteria.prepend (requestFlag);
                break;
            }
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
