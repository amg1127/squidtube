#include "stdinreader.h"

StdinReader::StdinReader (QObject *parent) : QThread (parent) {
}

void StdinReader::run () {
    qInfo ("StdinReader thread started.");
    std::string stdinLine;
    while (! std::getline (std::cin, stdinLine).eof()) {
        // Expected format is [channel-ID] %URI property [-flag [-flag [...] ] ] criteria [criteria [criteria [...]]]
        QString tokens (QString::fromLocal8Bit(stdinLine.data(), (int) stdinLine.size()).trimmed ());
        qInfo ("Received an ACL matching request: '%s'", tokens.toLatin1().constData());
        if (tokens.isEmpty ()) {
            emit writeAnswerLine ("", "an empty request was received", true, false);
        } else {
            QStringList requestTokens(tokens.split(QRegExp("\\s+"), QString::SkipEmptyParts));
            // Test whether the Squid administrator has set 'concurrency=' argument
            bool ok;
            QString requestChannel = requestTokens.value(0);
            int requestChannelNumber = requestChannel.toInt (&ok, 10);
            if (ok) {
                if (requestChannelNumber < 0) {
                    // This is a protocol error
                    emit writeAnswerLine (requestChannel, "invalid channel number received", true, false);
                    continue;
                } else {
                    requestTokens.removeFirst ();
                }
            } else {
                requestChannel = "";
            }
            if (requestTokens.isEmpty ()) {
                emit writeAnswerLine (requestChannel, "an empty URL was received", true, false);
            } else {
                // Check URL syntax
                QUrl requestUrl (requestTokens.takeFirst(), QUrl::StrictMode);
                if (requestUrl.isValid ()) {
                    if (requestTokens.isEmpty ()) {
                        emit writeAnswerLine (requestChannel, "ACL specifies no property for comparison", true, false);
                    } else {
                        emit squidRequest (requestChannelNumber, requestChannel, requestUrl, requestTokens);
                    }
                } else {
                    emit writeAnswerLine (requestChannel, QString("invalid URL was received: %1").arg(requestUrl.errorString ()), true, false);
                }
            }
        }
    }
    qInfo ("StdinReader thread finished.");
}
