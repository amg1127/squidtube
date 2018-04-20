#ifndef STDINREADER_H
#define STDINREADER_H

#include "appruntime.h"

#include <iostream>
#include <string>

#include <QStringList>
#include <QThread>
#include <QUrl>

class StdinReader : public QThread {
    Q_OBJECT
public:
    StdinReader (QObject *parent = Q_NULLPTR);
protected:
    void run ();
signals:
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
    void squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData);
};

#endif // STDINREADER_H
