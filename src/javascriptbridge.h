#ifndef JAVASCRIPTBRIDGE_H
#define JAVASCRIPTBRIDGE_H

#include "appruntime.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QEventLoop>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPair>
#include <QtDebug>
#include <QTextCodec>
#include <QTextEncoder>
#include <QTextDecoder>
#include <QTimer>

class JavascriptMethod {
public:
    static const int getSupportedUrls;
    static const int getObjectFromUrl;
    static const int getPropertiesFromObject;
    static const QStringList requiredMethods;
};

class JavascriptTimer : public QTimer {
    Q_OBJECT
private:
    unsigned int timerId;
    QJSEngine* jsEngine;
    QJSValue callback;
public:
    JavascriptTimer (QJSEngine& jsEngine, unsigned int timerId, bool repeat, int timeout, const QJSValue& callback);
private slots:
    void timerTimeout ();
signals:
    void timerFinished (unsigned int timerId);
};

class JavascriptNetworkRequest : public QObject {
    Q_OBJECT
private:
    QJSEngine* jsEngine;
    unsigned int networkRequestId;
    unsigned int maxRedirects;
    QJSValue xmlHttpRequestObject;
    QJSValue getPrivateDataCallback1;
    QJSValue getPrivateDataCallback2;
    QJSValue setPrivateDataCallback1;
    QJSValue setPrivateDataCallback2;
    QByteArray requestBody;
    QDataStream* requestBodyStream;
    QNetworkReply* networkReply;
    QTimer timeoutTimer;
    bool downloadStarted;
    int httpStatus;
    QString httpRequestMethod;
    bool getPrivateData (const QString& key, QJSValue& value);
    bool setPrivateData (const QString& key, const QJSValue& value);
    bool getPrivateData (const QString& key, const QString& subKey, QJSValue& value);
    bool setPrivateData (const QString& key, const QString& subKey, const QJSValue& value);
    void cancelRequest (bool emitNetworkRequestFinished = false);
    void fulfillRequest (QNetworkAccessManager& networkManager);
    void fireProgressEvent (bool isUpload, const QString& callback, qint64 transmitted, qint64 length);
    void fireProgressEvent (const QJSValue& callback, qint64 transmitted, qint64 length);
    inline bool isGetOrHeadRequest () {
        return (! (this->httpRequestMethod.compare("GET", Qt::CaseInsensitive) &&
                   this->httpRequestMethod.compare("HEAD", Qt::CaseInsensitive)));
    }
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    inline bool isRedirectWithMethodChange () {
        return (this->httpStatus == 302 ||
                this->httpStatus == 303);
    }
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    inline bool isRedirectWithoutMethodChange () {
        return ((this->httpStatus == 301 && this->isGetOrHeadRequest()) ||
                this->httpStatus == 307 ||
                this->httpStatus == 308);
    }
    inline bool isFinalAnswer () {
        return (this->maxRedirects <= 0 ||
                (! this->isRedirectWithMethodChange()) ||
                (! this->isRedirectWithoutMethodChange()));
    }
private slots:
    void networkReplyDownloadProgress (qint64 bytesReceived, qint64 bytesTotal);
    void networkReplyFinished ();
    void networkReplyUploadProgress (qint64 bytesSent, qint64 bytesTotal);
    void timeoutTimerTimeout ();
public:
    static const short int status_UNSENT;
    static const short int status_OPENED;
    static const short int status_HEADERS_RECEIVED;
    static const short int status_LOADING;
    static const short int status_DONE;
    JavascriptNetworkRequest (QObject* parent = Q_NULLPTR);
    ~JavascriptNetworkRequest ();
    void setTimerInterval (int msec);
    inline void setXMLHttpRequestObject (QJSValue& object) {
        this->xmlHttpRequestObject = object;
    }
    inline void setPrivateDataCallbacks (QJSValue& getPrivateData1, QJSValue& setPrivateData1, QJSValue& getPrivateData2, QJSValue& setPrivateData2) {
        this->getPrivateDataCallback1 = getPrivateData1;
        this->setPrivateDataCallback1 = setPrivateData1;
        this->getPrivateDataCallback2 = getPrivateData2;
        this->setPrivateDataCallback2 = setPrivateData2;
    }
    void start (QNetworkAccessManager& networkManager, unsigned int networkRequestId);
    void abort ();
signals:
    void networkRequestFinished (unsigned int requestId);
};

class JavascriptBridge : public QObject {
    Q_OBJECT
private:
    QJSValue myself;
    QString requestChannel;
    QMap<unsigned int,unsigned int> pendingTransactions;
    unsigned int transactionId;
    QStringList loadedLibraries;
    QMap<unsigned int,JavascriptTimer*> javascriptTimers;
    unsigned int javascriptTimerId;
    QNetworkAccessManager* networkManager;
    QMap<unsigned int, JavascriptNetworkRequest*> pendingNetworkRequests;
    unsigned int networkRequestId;
    static QJsonValue QJS2QJsonValue (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonValue& value);
    unsigned int createTimer (const bool repeat, const QJSValue& callback, const int interval);
public:
    JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel);
    ~JavascriptBridge ();
    static QJsonDocument QJS2QJsonDocument (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonDocument& value);
    static QString QJS2QString (const QJSValue& value);
    static bool warnJsError (const QJSValue& jsValue, const QString& msg = QString());
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, int method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, const QString& method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    // http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#conversion-between-qt-and-javascript-types
    Q_INVOKABLE void receiveValue (unsigned int transactionId, const QString& method, const QJSValue& returnedValue);
    Q_INVOKABLE void require (const QJSValue& library);
    Q_INVOKABLE unsigned int setTimeout (const QJSValue& callback, const int interval);
    Q_INVOKABLE unsigned int setInterval (const QJSValue& callback, const int interval);
    Q_INVOKABLE void clearTimeout (unsigned int timerId);
    Q_INVOKABLE void clearInterval (unsigned int timerId);
    Q_INVOKABLE void xmlHttpRequest_send (QJSValue& object, QJSValue& getPrivateData1, QJSValue& setPrivateData1, QJSValue& getPrivateData2, QJSValue& setPrivateData2);
    Q_INVOKABLE void xmlHttpRequest_abort (const unsigned int networkRequestId);
    Q_INVOKABLE void xmlHttpRequest_setTimeout (const unsigned int networkRequestId, const int msec);
    Q_INVOKABLE QString textDecode (const QByteArray& bytes, const QString& fallbackCharset);
    Q_INVOKABLE QByteArray textEncode (const QString& string, const QString& charset);
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    Q_INVOKABLE void console_log (const QJSValue& msg);
    Q_INVOKABLE void console_info (const QJSValue& msg);
    Q_INVOKABLE void console_warn (const QJSValue& msg);
    Q_INVOKABLE void console_error (const QJSValue& msg);
#endif
private slots:
    void timerFinished (unsigned int timerId);
    void networkRequestFinished (unsigned int networkRequestId);
signals:
    void valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue);
};

#endif // JAVASCRIPTBRIDGE_H
