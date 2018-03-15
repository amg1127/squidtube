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
    emit finishWorkers ();
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
        qInfo() << QString("Channel #%1 answered: isMatch=%2 ; message=%3").arg(channel).arg((isMatch) ? "true" : "false").arg(msg);
    }
}

void JobDispatcher::squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData) {
    if (requestChannelNumber >= 0 && requestUrl.isValid() && (! requestData.isEmpty())) {
        QStringList requestTokens (requestData);
        // Decode the tokens
        for (QStringList::iterator token = requestTokens.begin(); token != requestTokens.end(); token++) {
            (*token) = QUrl::fromPercentEncoding ((*token).toUtf8());
        }
        QString requestProperty (requestTokens.takeFirst());
        Qt::CaseSensitivity requestCaseSensivity (Qt::CaseSensitive);
        QRegExp::PatternSyntax requestPatternSyntax (QRegExp::RegExp);
        // Check the flags that may have specified by the administrator
        for (QString requestFlag = requestTokens.takeFirst(); ! requestTokens.isEmpty(); requestFlag = requestTokens.takeFirst()) {
            if (requestFlag.left(1) == "-") {
                if (requestFlag == "-f" || requestFlag == "--fixed") {
                    requestPatternSyntax = QRegExp::FixedString;
                } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                    requestPatternSyntax = QRegExp::WildcardUnix;
                } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                    requestCaseSensivity = Qt::CaseInsensitive;
                } else {
                    this->writeAnswerLine (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag, true, false);
                    return;
                }
            } else {
                // No more flags to look for
                requestTokens.prepend (requestFlag);
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
            QObject::connect (carrier->worker(), &JobWorker::writeAnswerLine, this, &JobDispatcher::writeAnswerLine);
            QObject::connect (this, &JobDispatcher::finishWorkers, carrier->worker(), &JobWorker::quit);
            QObject::connect (carrier, &JobCarrier::finished, this, &JobDispatcher::jobWorkerFinished);
            this->jobCarriers[requestChannelNumber] = carrier;
            this->numJobCarriers++;
            carrier->start ();
        }
        carrier->squidRequestIn (requestUrl, requestProperty, requestCaseSensivity, requestPatternSyntax, requestTokens);
    } else {
        qFatal("Invalid procedure call!");
    }
}

JobDispatcher::JobDispatcher (QObject *parent) :
    QObject (parent),
    started (false),
    numJobCarriers (0) {
    QObject::connect (&this->stdinReader, &StdinReader::finished, this, &JobDispatcher::stdinReaderFinished);
    QObject::connect (&this->stdinReader, &StdinReader::writeAnswerLine, this, &JobDispatcher::writeAnswerLine);
    QObject::connect (&this->stdinReader, &StdinReader::squidRequest, this, &JobDispatcher::squidRequest);
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
        this->stdinReader.start (priority);
        this->started = true;
    }
}
