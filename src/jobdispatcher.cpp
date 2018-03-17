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
        squidRequest.url = requestUrl;
        squidRequest.criteria = requestData;
        squidRequest.property = squidRequest.criteria.takeFirst();
        squidRequest.caseSensivity = Qt::CaseSensitive;
        squidRequest.patternSyntax = QRegExp::RegExp;
        // Decode the tokens
        for (QStringList::iterator token = squidRequest.criteria.begin(); token != squidRequest.criteria.end(); token++) {
            (*token) = QUrl::fromPercentEncoding (token->toUtf8());
        }
        // Check the flags that may have specified by the administrator
        while (! squidRequest.criteria.isEmpty ()) {
            QString requestFlag (squidRequest.criteria.takeFirst());
            if (requestFlag.left(1) == "-") {
                if (requestFlag == "-f" || requestFlag == "--fixed") {
                    squidRequest.patternSyntax = QRegExp::FixedString;
                } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                    squidRequest.patternSyntax = QRegExp::WildcardUnix;
                } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                    squidRequest.caseSensivity = Qt::CaseInsensitive;
                } else {
                    this->writeAnswerLine (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag, true, false);
                    return;
                }
            } else {
                // No more flags to look for
                squidRequest.criteria.prepend (requestFlag);
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
        squidRequest.timestamp = (AppRuntime::currentDateTime().toMSecsSinceEpoch() / 1000);
        carrier->squidRequestIn (squidRequest);
    } else {
        qFatal("Invalid procedure call!");
    }
}

JobDispatcher::JobDispatcher (QObject *parent) :
    QObject (parent),
    started (false),
    numJobCarriers (0) {
    QObject::connect (&this->stdinReader, &StdinReader::finished, this, &JobDispatcher::stdinReaderFinished);
    QObject::connect (&this->stdinReader, &StdinReader::writeAnswerLine, this, &JobDispatcher::writeAnswerLine, Qt::QueuedConnection);
    QObject::connect (&this->stdinReader, &StdinReader::squidRequest, this, &JobDispatcher::squidRequest, Qt::QueuedConnection);
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
