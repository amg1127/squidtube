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

class jobDispatcher : public QObject {
    Q_OBJECT
private:
    QList<jobWorker*> workers;
    void writeAnswerLine (const QString& msg);
    void writeErrorAnswer (const QString& channel, const QString& msg);
    void writeResultAnswer (const QString& channel, bool matched, const QString& msg);

public:
    explicit jobDispatcher (QObject *parent = 0);
    ~jobDispatcher ();

signals:

public slots:
    void squidRequest (const QStringList& tokens);
};

#endif // JOBDISPATCHER_H
