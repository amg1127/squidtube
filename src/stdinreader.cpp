#include "stdinreader.h"

stdinReader::stdinReader (QObject *parent) : QThread (parent) {
}

void stdinReader::run () {
    qDebug() << "stdinReader thread started.";
    std::string stdinLine;
    while (! std::getline (std::cin, stdinLine).eof()) {
        // Expected format is [channel-ID] %URI property [-flag [-flag [...] ] ] criteria [criteria [criteria [...]]]
        QString tokens (QString::fromLocal8Bit(stdinLine.data(), (int) stdinLine.size()).trimmed ());
        qDebug() << QString("Squid has sent an ACL matching request: %1").arg(tokens);
        emit squidRequest (tokens.split(QRegExp("\\s+"), QString::SkipEmptyParts));
    }
    qDebug() << "stdinReader thread finished.";
}
