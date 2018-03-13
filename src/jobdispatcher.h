#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "jobworker.h"

#include <iostream>

#include <QObject>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QtDebug>
#include <QUrl>

class JobDispatcher : public QObject {
    Q_OBJECT
private:
    QList<JobCarrier*> jobCarriers;

public:
    explicit JobDispatcher (QObject *parent = 0);
    ~JobDispatcher ();

public slots:
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
    void squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData);
};

#endif // JOBDISPATCHER_H
