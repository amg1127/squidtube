#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"
#include "javascriptbridge.h"
#include "objectcache.h"

#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QRegExp>
#include <QJsonDocument>
#include <QJSValue>
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
    QQmlEngine* runtimeEnvironment;
    JavascriptBridge* javascriptBridge;
    int requestId;
    QMap<int,AppSquidRequest> runningRequests;
    QTimer* retryTimer;
    QLinkedList<AppSquidRequest> incomingRequests;
    qint64 currentTimestamp;
    void squidResponseOut (const int requestId, const QString& msg, bool isError, bool isMatch);
    void processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls);
    void processObjectFromUrl (int helperInstance, const QJSValue& appHelperObjectFromUrl);
    void processCriteria (int requestId, const QJsonDocument& data);
public:
    JobWorker (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
private slots:
    void valueReturnedFromJavascript (int context, const QString& method, const QJSValue& returnedValue);
    void processIncomingRequest ();
    void setCurrentTimestamp ();
public slots:
    void squidRequestIn (const AppSquidRequest& squidRequest);
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
    inline JobWorker* worker () { return (this->workerObj); }
    void start (QThread::Priority priority = QThread::InheritPriority);
    inline bool wait (unsigned long time = ULONG_MAX) { return (this->threadObj->wait (time)); }
    void squidRequestIn (const AppSquidRequest& squidRequest);
signals:
    void squidRequestOut (const AppSquidRequest& squidRequest);
    void finished ();
};

#endif // JOBWORKER_H
