#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "stdinreader.h"
#include "jobworker.h"

#include <iostream>

#include <QObject>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QtDebug>
#include <QTimer>
#include <QUrl>

class JobDispatcher : public QObject {
    Q_OBJECT
private:
    bool started;
    unsigned int numJobCarriers;
    StdinReader stdinReader;
    QList<JobCarrier*> jobCarriers;
    QTimer* clockTimer;
    qint64 currentTimestamp;
private slots:
    void jobWorkerFinished ();
    void stdinReaderFinished ();
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
    void squidRequest (const int requestChannelNumber, const QString& requestChannel, const QUrl& requestUrl, const QStringList& requestData);
    void setCurrentTimestamp ();
public:
    JobDispatcher (QObject *parent = 0);
    ~JobDispatcher ();
    void start (QThread::Priority priority = QThread::InheritPriority);
signals:
    void finishWorkers ();
    void finished ();
};

#endif // JOBDISPATCHER_H
