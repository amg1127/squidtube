#include "jobdispatcher.h"

jobDispatcher::jobDispatcher(QObject *parent) : QObject(parent) {

}

void jobDispatcher::squidRequest (const QStringList &tokens) {
#warning Nao implementado!
    // Expected format is [channel-ID] %URI property [-flag [-flag [...] ] ] criteria [criteria [criteria [...]]]
    if (tokens.isEmpty ()) {
        jobDispatcher::writeErrorAnswer ("", "an empty request was received");
    } else {
        QStringList copyOfTokens (tokens);
        // Test whether the Squid administrator has set 'concurrency=' argument
        bool ok;
        QString requestChannel (copyOfTokens.value(0));
        int requestChannelNumber = requestChannelToken.toInt (&ok, 10);
        if (ok) {
            if (requestChannelNumber < 0) {
                // This is a protocol error
                jobDispatcher::writeErrorAnswer (requestChannel, "protocol error");
                return;
            } else {
                copyOfTokens.removeFirst ();
            }
        } else {
            requestChannel = "";
        }
        if (copyOfTokens.isEmpty ()) {
            jobDispatcher::writeErrorAnswer (requestChannel, "an empty URL was received");
        } else {
            // Check URL syntax
            QUrl requestUri (copyOfTokens.takeFirst(), QUrl::StrictMode);
            if (requestUri.isValid ()) {
                if (copyOfTokens.isEmpty ()) {
                    jobDispatcher::writeErrorAnswer (requestChannel, "ACL specifies no property for comparison");
                } else {
                    QString requestProperty (copyOfTokens.takeFirst());
                    Qt::CaseSensitivity requestCaseSensivity(Qt::CaseSensitive);
                    QRegExp::PatternSyntax requestPatternSyntax(QRegExp::RegExp);
                    // Check the flags that may have specified by the administrator
                    for (QString requestFlag = copyOfTokens.takeFirst(); ! copyOfTokens.isEmpty(); requestFlag = copyOfTokens.takeFirst()) {
                        if (requestFlag.left(1) == "-") {
                            if (requestFlag == "-f" || requestFlag == "--fixed") {
                                requestPatternSyntax = QRegExp::FixedString;
                            } else if (requestFlag == "-w" || requestFlag == "--wildcard") {
                                requestPatternSyntax = QRegExp::WildcardUnix;
                            } else if (requestFlag == "-i" || requestFlag == "--ignorecase") {
                                requestCaseSensivity = Qt::CaseInsensitive;
                            } else {
                                jobDispatcher::writeErrorAnswer (requestChannel, QString("ACL specifies an invalid flag: ") + requestFlag);
                                return;
                            }
                        } else {
                            // No more flags to look for
                            copyOfTokens.prepend (requestFlag);
                            break;
                        }
                    }
#error Continue here
                }
            } else {
                jobDispatcher::writeErrorAnswer (requestChannel, "invalid URL was received");
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
    writeAnswerLine (msgpart1 + QString("BH message=%1 log=%1").arg(QUrl::toPercentEncoding (msg)));
    qWarning() << QString("An internal error ocurred while processing the request") + msgpart2 + ": " + msg;
}
