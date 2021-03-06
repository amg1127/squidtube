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

#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"
#include "javascriptbridge.h"
#include "objectcache.h"

#include <QJSEngine>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QJSValue>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QThread>
#include <QUrl>

class AppHelperInfo {
public:
    QString name;
    QList<QRegExp> supportedURLs;
    QJSValue entryPoint;
    ObjectCacheMemory* memoryCache;
    ObjectCacheDatabase* databaseCache;
};

class JobWorker : public QObject {
    Q_OBJECT
private:
    bool finishRequested;
    bool rngdInitialized;
    QString requestChannel;
    QList<AppHelperInfo*> helperInstances;
    QJSEngine* runtimeEnvironment;
    JavascriptBridge* javascriptBridge;
    unsigned int requestId;
    QMap<unsigned int,AppJobRequest*> runningRequests;
    QTimer* retryTimer;
    QLinkedList<AppJobRequest*> incomingRequests;
    qint64 currentTimestamp;
    void startRetryTimer ();
    void tryNextHelper (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator);
    void squidResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromSquid* squidRequest, const QString& msg, bool isMatch);
    inline void scriptResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromHelper* helperRequest, QJSValue::SpecialValue returnValue) {
        this->scriptResponseOut (requestIdIterator, helperRequest, QJSValue(returnValue));
    }
    void scriptResponseOut (QMap<unsigned int,AppJobRequest*>::iterator& requestIdIterator, AppJobRequestFromHelper* helperRequest, const QJSValue& returnValue);
    void processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls);
    void processObjectFromUrl (unsigned int _requestId, const QJSValue& appHelperObjectFromUrl);
    void processPropertiesFromObject (unsigned int _requestId, const QJSValue& appHelperPropertiesFromObject);
    static const char* jsonType (const QJsonValue& jsonValue);
    static bool processCriteria (
        const QString& requestHelperName,
        const unsigned int _requestId,
        const int level,
        const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesIterator,
        const AppSquidMathMatchOperator& requestMathMatchOperator,
        const Qt::CaseSensitivity& requestCaseSensitivity,
        const QRegExp::PatternSyntax& requestPatternSyntax,
        const QStringList& requestCriteria,
        const QJsonDocument& jsonDocumentInformation);
    static bool processCriteria (
        const QString& requestHelperName,
        const unsigned int _requestId,
        const int level,
        const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesIterator,
        const AppSquidMathMatchOperator& requestMathMatchOperator,
        const Qt::CaseSensitivity& requestCaseSensitivity,
        const QRegExp::PatternSyntax& requestPatternSyntax,
        const QStringList& requestCriteria,
        const QJsonValue& jsonValueInformation);
    static bool regexMatches (const QRegExp& regularExpression, const QString& testString);
public:
    JobWorker (const QString& _requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
private slots:
    void valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue);
    void processIncomingRequest ();
    void setCurrentTimestamp ();
public slots:
    void squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow);
    void quit ();
signals:
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
    void finished();
};

class JobCarrier : public QObject {
    Q_OBJECT
private:
    bool started;
    QThread* threadObj;
    JobWorker* workerObj;
public:
    JobCarrier (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobCarrier ();
    inline JobWorker* worker () {
        return (this->workerObj);
    }
    bool start (QThread::Priority priority = QThread::InheritPriority);
    inline bool wait (unsigned long time = ULONG_MAX) {
        return (this->threadObj->wait (time));
    }
    void squidRequestIn (AppJobRequestFromSquid* request, const qint64 timestampNow);
signals:
    void squidRequestOut (AppJobRequestFromSquid* request, const qint64 timestampNow);
    void finished ();
};

#endif // JOBWORKER_H
