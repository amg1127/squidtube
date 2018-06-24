/*
 * squidtube - An external Squid ACL class helper that provides control over access to videos
 * Copyright (C) 2018  Anderson M. Gomes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "stdinreader.h"

StdinReader::StdinReader (QObject *parent) : QThread (parent) {
}

void StdinReader::run () {
    qInfo ("StdinReader thread started.");
    std::string stdinLine;
    while (! std::getline (std::cin, stdinLine).eof()) {
        // Expected format is [channel-ID] %URI property [-flag [-flag [...] ] ] criteria [criteria [criteria [...]]]
        QString tokens (QString::fromLocal8Bit(stdinLine.data(), static_cast<int> (stdinLine.size())).trimmed ());
        qInfo ("Received an ACL matching request: '%s'", tokens.toLatin1().constData());
        if (tokens.isEmpty ()) {
            emit writeAnswerLine (QStringLiteral(""), QStringLiteral("an empty request was received"), true, false);
        } else {
            QStringList requestTokens(tokens.split(QRegExp(QStringLiteral("\\s+")), QString::SkipEmptyParts));
            // Test whether the Squid administrator has set 'concurrency=' argument
            bool ok;
            QString requestChannel = requestTokens.value(0);
            int requestChannelNumber = requestChannel.toInt (&ok, 10);
            if (ok) {
                if (requestChannelNumber < 0) {
                    // This is a protocol error
                    emit writeAnswerLine (requestChannel, QStringLiteral("invalid channel number received"), true, false);
                    continue;
                } else {
                    requestTokens.removeFirst ();
                }
            } else {
                requestChannel = QStringLiteral("");
            }
            if (requestTokens.isEmpty ()) {
                emit writeAnswerLine (requestChannel, QStringLiteral("an empty URL was received"), true, false);
            } else {
                // Check URL syntax
                QUrl requestUrl (requestTokens.takeFirst(), QUrl::StrictMode);
                if (requestUrl.isValid ()) {
                    if (requestTokens.isEmpty ()) {
                        emit writeAnswerLine (requestChannel, QStringLiteral("ACL specifies no property for comparison"), true, false);
                    } else {
                        emit squidRequest (requestChannelNumber, requestChannel, requestUrl, requestTokens);
                    }
                } else {
                    emit writeAnswerLine (requestChannel, QStringLiteral("invalid URL was received: %1").arg(requestUrl.errorString ()), true, false);
                }
            }
        }
    }
    qInfo ("StdinReader thread finished.");
}
