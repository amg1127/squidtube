#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"
#include "javascriptbridge.h"
#include "objectcache.h"

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
#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
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
    QQmlEngine* runtimeEnvironment;
    JavascriptBridge* javascriptBridge;
    unsigned int requestId;
    QMap<unsigned int,AppSquidRequest> runningRequests;
    QTimer* retryTimer;
    QLinkedList<AppSquidRequest> incomingRequests;
    qint64 currentTimestamp;
    void squidResponseOut (const unsigned int requestId, const QString& msg, bool isError, bool isMatch);
    void processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls);
    void processObjectFromUrl (unsigned int requestId, const QJSValue& appHelperObjectFromUrl);
    void processPropertiesFromObject (unsigned int requestId, const QJSValue& appHelperPropertiesFromObject);
    static bool processCriteria (const QLinkedList<AppSquidPropertyMatch>& requestProperties, const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesItem, const AppSquidMathMatchOperator& requestMathOperator, const Qt::CaseSensitivity& requestCaseSensitivity, const QRegExp::PatternSyntax& requestPatternSyntax, const QStringList& requestCriteria, const QJsonObject& jsonObjectInformation);
    static bool processCriteria (const QLinkedList<AppSquidPropertyMatch>& requestProperties, const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesItem, const AppSquidMathMatchOperator& requestMathOperator, const Qt::CaseSensitivity& requestCaseSensitivity, const QRegExp::PatternSyntax& requestPatternSyntax, const QStringList& requestCriteria, const QJsonArray& jsonArrayInformation);
    static bool processCriteria (const QLinkedList<AppSquidPropertyMatch>& requestProperties, const QLinkedList<AppSquidPropertyMatch>::const_iterator& requestPropertiesItem, const AppSquidMathMatchOperator& requestMathOperator, const Qt::CaseSensitivity& requestCaseSensitivity, const QRegExp::PatternSyntax& requestPatternSyntax, const QStringList& requestCriteria, const QJsonValue& jsonValueInformation);
public:
    JobWorker (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
private slots:
    void valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue);
    void processIncomingRequest ();
    void setCurrentTimestamp ();
public slots:
    void squidRequestIn (const AppSquidRequest& squidRequest, const qint64 timestampNow);
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
    void squidRequestIn (const AppSquidRequest& squidRequest, const qint64 timestampNow);
signals:
    void squidRequestOut (const AppSquidRequest& squidRequest, const qint64 timestampNow);
    void finished ();
};

#endif // JOBWORKER_H
