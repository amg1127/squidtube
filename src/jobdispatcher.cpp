#include "jobdispatcher.h"

jobDispatcher::jobDispatcher(QObject *parent) : QObject(parent) {
}

jobDispatcher::~jobDispatcher () {
    qDebug() << "Finishing all request workers...";
    for (QList<jobWorker*>::iterator job = this->workers.begin(); job != this->workers.end(); job++) {
        (*job)->quit ();
    }
    for (QList<jobWorker*>::iterator job = this->workers.begin(); job != this->workers.end(); job++) {
        (*job)->wait ();
    }
    for (QList<jobWorker*>::iterator job = this->workers.begin(); job != this->workers.end(); job++) {
        delete (*job);
    }
    qDebug() << "All request workers finished.";
}

void jobDispatcher::squidRequest (const QStringList &tokens) {
    // Expected format is [channel-ID] %URI property [-flag [-flag [...] ] ] criteria [criteria [criteria [...]]]
    if (tokens.isEmpty ()) {
        jobDispatcher::writeErrorAnswer ("", "an empty request was received");
    } else {
        jobWork request;
        request.requestTokens = tokens;
        // Test whether the Squid administrator has set 'concurrency=' argument
        bool ok;
        request.requestChannel = request.requestTokens.value(0);
        int requestChannelNumber = request.requestChannel.toInt (&ok, 10);
        if (ok) {
            if (requestChannelNumber < 0) {
                // This is a protocol error
                jobDispatcher::writeErrorAnswer (request.requestChannel, "protocol error");
                return;
            } else {
                request.requestTokens.removeFirst ();
            }
        } else {
            request.requestChannel = "";
        }
        if (request.requestTokens.isEmpty ()) {
            jobDispatcher::writeErrorAnswer (request.requestChannel, "an empty URL was received");
        } else {
            // Check URL syntax
            QUrl requestUri (request.requestTokens.takeFirst(), QUrl::StrictMode);
            if (requestUri.isValid ()) {
                if (request.requestTokens.isEmpty ()) {
                    jobDispatcher::writeErrorAnswer (request.requestChannel, "ACL specifies no property for comparison");
                } else {
                    request.requestProperty = request.requestTokens.takeFirst();
                    request.requestCaseSensivity = Qt::CaseSensitive;
                    request.requestPatternSyntax = QRegExp::RegExp;
                    // Check the flags that may have specified by the administrator
                    for (QString requestFlag = request.requestTokens.takeFirst(); ! request.requestTokens.isEmpty(); requestFlag = request.requestTokens.takeFirst()) {
                        if (requestFlag.left(1) == "-") {
                            if (requestFlag == "-f" || requestFlag == "--fixed") {
                                request.requestPatternSyntax = QRegExp::FixedString;
                            } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                                request.requestPatternSyntax = QRegExp::WildcardUnix;
                            } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                                request.requestCaseSensivity = Qt::CaseInsensitive;
                            } else {
                                jobDispatcher::writeErrorAnswer (request.requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag);
                                return;
                            }
                        } else {
                            // No more flags to look for
                            request.requestTokens.prepend (requestFlag);
                            break;
                        }
                    }
                    // Decode the comparation texts
                    for (QStringList::iterator token = request.requestTokens.begin(); token != request.requestTokens.end(); token++) {
                        (*token) = QUrl::fromPercentEncoding ((*token).toUtf8());
                    }
                    // Now, send the request to a worker
                    // Create if it does not exists yet
                    while (requestChannelNumber >= this->workers.count()) {
                        jobWorker* worker = new jobWorker ();
#error It is important to remember that a QThread instance lives in the old thread that instantiated it, not in the new thread that calls run(). This means that all of QThread's queued slots will execute in the old thread. Thus, a developer who wishes to invoke slots in the new thread must use the worker-object approach; new slots should not be implemented directly into a subclassed QThread.
#warning Connect signals to slots here
                        this->workers.append (worker);
                        worker->start ();
                    }
                    this->workers[requestChannelNumber]->addJob (request);
                }
            } else {
                jobDispatcher::writeErrorAnswer (request.requestChannel, "invalid URL was received");
            }
        }
    }
}

void jobDispatcher::writeAnswerLine (const QString &msg) {
    std::cout << msg.toLocal8Bit().constData() << std::endl;
}

void jobDispatcher::writeErrorAnswer (const QString& channel, const QString &msg) {
    QString msgpart1("");
    QString msgpart2("");
    if (! channel.isEmpty()) {
        msgpart1 += channel + " ";
        msgpart2 += QString("from channel #") + channel;
    }
    writeAnswerLine (msgpart1 + QString("BH message=%1 log=%1").arg(QString::fromUtf8(QUrl::toPercentEncoding (msg))));
    qWarning() << QString("An internal error ocurred while processing the request") + msgpart2 + ": " + msg;
}
