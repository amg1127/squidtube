#include "jobdispatcher.h"

JobDispatcher::JobDispatcher (QObject *parent) : QObject(parent) {
}

JobDispatcher::~JobDispatcher () {
    qDebug() << "Finishing all request workers...";
    JobCarrier* jobCarrier;
    while (! this->jobCarriers.isEmpty ()) {
        jobCarrier = this->jobCarriers.takeLast ();
        if (jobCarrier != Q_NULLPTR) {
            delete (jobCarrier);
        }
    }
    qDebug() << "All request workers finished.";
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
            qDebug() << QString("Creating a new handler for the channel #%1...").arg(requestChannelNumber);
            carrier = new JobCarrier (requestChannel, this);
            QObject::connect (carrier->worker(), &JobWorker::writeAnswerLine, this, &JobDispatcher::writeAnswerLine);
            carrier->start ();
            this->jobCarriers[requestChannelNumber] = carrier;
        }
        carrier->squidRequestIn (requestUrl, requestProperty, requestCaseSensivity, requestPatternSyntax, requestTokens);
    } else {
        qFatal("Invalid procedure call!");
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
    }
}
