#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"
#include "javascriptbridge.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QRegExp>
#include <QJsonDocument>
#include <QJSValue>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>

class AppHelperInfo {
public:
    QString name;
    bool isAvailable;
    QList<QRegExp> supportedURLs;
    QJSValue entryPoint;
};

class JobWorker : public QObject {
    Q_OBJECT
private:
    bool finishRequested;
    QString requestChannel;
    QList<AppHelperInfo*> helperInstances;
    QQmlEngine* runtimeEnvironment;
    JavascriptBridge* javascriptBridge;
    int requestId;
    QList<AppSquidRequest> pendingRequests;
    QList<int> pendingRequestIDs;
    int appendRequest (const AppSquidRequest& squidRequest, const QString& helperName);
    AppSquidRequest getRequest (int requestId);
    AppSquidRequest takeRequest (int requestId);
    void squidResponseOut (const int requestId, const QString& msg, bool isError, bool isMatch);
    void processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls);
    void processObjectFromUrl (int helperInstance, const QJSValue& appHelperObjectFromUrl);
public:
    JobWorker (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
private slots:
    void valueReturnedFromJavascript (int context, const QString& method, const QJSValue& returnedValue);
public slots:
    void quit ();
    void squidRequestIn (const AppSquidRequest& squidRequest);
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
