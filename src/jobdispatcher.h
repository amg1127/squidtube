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

#ifndef JOBDISPATCHER_H
#define JOBDISPATCHER_H

#include "stdinreader.h"
#include "jobworker.h"

#include <iostream>

#include <QObject>
#include <QRegExp>
#include <QString>
#include <QStringList>
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
    JobDispatcher (QObject *parent = Q_NULLPTR);
    ~JobDispatcher ();
    bool start (QThread::Priority priority = QThread::InheritPriority);
signals:
    void finishWorkers ();
    void finished ();
};

#endif // JOBDISPATCHER_H
