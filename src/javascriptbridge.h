#ifndef JAVASCRIPTBRIDGE_H
#define JAVASCRIPTBRIDGE_H

#include "appruntime.h"

#include <QBuffer>
#include <QCoreApplication>
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
    JavascriptTimer (QJSEngine& _jsEngine, unsigned int _timerId, bool repeat, int timeout, const QJSValue& _callback);
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
    QString requestMethod;
    QBuffer requestBodyBuffer;
    bool synchronousFlag;
    int responseStatus;
    QNetworkReply* networkReply;
    QTimer timeoutTimer;
    bool downloadStarted;
    QBuffer responseBodyBuffer;
    bool getPrivateData (const QString& key, QJSValue& value);
    bool setPrivateData (const QString& key, const QJSValue& value);
    bool getPrivateData (const QString& key, const QString& subKey, QJSValue& value);
    bool setPrivateData (const QString& key, const QString& subKey, const QJSValue& value);
    void cancelRequest (bool emitNetworkRequestFinished = false);
    void fulfillRequest (QNetworkAccessManager& networkManager, bool invokeLoadStart);
    void fireProgressEvent (bool isUpload, const QString& callback, qint64 transmitted, qint64 length);
    void fireProgressEvent (QJSValue& callback, qint64 transmitted, qint64 length);
    void fireEvent (const QString& callback);
    inline bool isGetOrHeadRequest () {
        return (! (this->requestMethod.compare(QStringLiteral("GET"), Qt::CaseInsensitive) &&
                   this->requestMethod.compare(QStringLiteral("HEAD"), Qt::CaseInsensitive)));
    }
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    inline bool isRedirectWithMethodChange () {
        return (this->responseStatus == 302 ||
                this->responseStatus == 303);
    }
    // https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#3xx_Redirection
    inline bool isRedirectWithoutMethodChange () {
        return ((this->responseStatus == 301 && this->isGetOrHeadRequest()) ||
                this->responseStatus == 307 ||
                this->responseStatus == 308);
    }
    inline bool isFinalAnswer () {
        return (! (this->maxRedirects && (this->isRedirectWithMethodChange() || this->isRedirectWithoutMethodChange())));
    }
    // HTTP errors are expected to be handled by XMLHttpRequest users, but QNetworkAccessManager handles some errors by itself...
    inline bool httpResponseOk () {
        return (this->networkReply->error() == QNetworkReply::NoError || (this->responseStatus >= 400 && this->responseStatus < 600));
    }
private slots:
    void networkReplyUploadProgress (qint64 bytesSent, qint64 bytesTotal);
    void networkReplyDownloadProgress (qint64 bytesReceived, qint64 bytesTotal);
    void networkReplyFinished ();
    void timeoutTimerTimeout ();
public:
    static const short int status_UNSENT;
    static const short int status_OPENED;
    static const short int status_HEADERS_RECEIVED;
    static const short int status_LOADING;
    static const short int status_DONE;
    JavascriptNetworkRequest (QJSEngine& _jsEngine, QObject *_parent = Q_NULLPTR);
    ~JavascriptNetworkRequest ();
    void setTimerInterval (int msec);
    inline void setXMLHttpRequestObject (const QJSValue& object) {
        this->xmlHttpRequestObject = object;
    }
    inline void setPrivateDataCallbacks (const QJSValue& getPrivateData1, const QJSValue& setPrivateData1, const QJSValue& getPrivateData2, const QJSValue& setPrivateData2) {
        this->getPrivateDataCallback1 = getPrivateData1;
        this->setPrivateDataCallback1 = setPrivateData1;
        this->getPrivateDataCallback2 = getPrivateData2;
        this->setPrivateDataCallback2 = setPrivateData2;
    }
    void start (QNetworkAccessManager& networkManager, unsigned int _networkRequestId);
    void abort ();
    inline const QByteArray responseBuffer () {
        return (this->responseBodyBuffer.data ());
    }
signals:
    void networkRequestFinished (unsigned int requestId);
};

class JavascriptBridge : public QObject {
    Q_OBJECT
private:
    QJSEngine* jsEngine;
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
    unsigned int createTimer (const bool repeat, const QJSValue& callback, const int interval);
    static QJsonValue QJS2QJsonValue (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonValue& value);
public:
    JavascriptBridge (QJSEngine& _jsEngine, const QString& _requestChannel, QObject* parent = Q_NULLPTR);
    ~JavascriptBridge ();
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, int method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, const QString& method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    QJSValue makeEntryPoint (int index);
    // http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#conversion-between-qt-and-javascript-types
    Q_INVOKABLE void receiveValue (unsigned int _transactionId, const QString& method, const QJSValue& returnedValue);
    Q_INVOKABLE bool require (const QJSValue& library);
    Q_INVOKABLE unsigned int setTimeout (const QJSValue& callback, const int interval);
    Q_INVOKABLE unsigned int setInterval (const QJSValue& callback, const int interval);
    Q_INVOKABLE void clearTimeout (unsigned int timerId);
    Q_INVOKABLE void clearInterval (unsigned int timerId);
    Q_INVOKABLE void xmlHttpRequest_send (const QJSValue& object, const QJSValue& getPrivateData1, const QJSValue& setPrivateData1, const QJSValue& getPrivateData2, const QJSValue& setPrivateData2);
    Q_INVOKABLE void xmlHttpRequest_abort (const unsigned int _networkRequestId);
    Q_INVOKABLE void xmlHttpRequest_setTimeout (const unsigned int _networkRequestId, const int msec);
    Q_INVOKABLE QJSValue xmlHttpRequest_getResponseBuffer (const unsigned int _networkRequestId);
    Q_INVOKABLE QString textDecode (const QJSValue& bytes, const QString& fallbackCharset);
    Q_INVOKABLE QJSValue textEncode (const QString& string, const QString& charset);
    Q_INVOKABLE void getPropertiesFromObjectCache (unsigned int context, const QJSValue& returnValue);
#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
    Q_INVOKABLE void console_log (const QJSValue& msg);
    Q_INVOKABLE void console_info (const QJSValue& msg);
    Q_INVOKABLE void console_warn (const QJSValue& msg);
    Q_INVOKABLE void console_error (const QJSValue& msg);
#endif
    static QJsonDocument QJS2QJsonDocument (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonDocument& value);
    static QString QJS2QString (const QJSValue& value);
    static QJSValue QByteArray2ArrayBuffer (QJSEngine& jsEngine, const QByteArray& value);
    static QByteArray ArrayBuffer2QByteArray (QJSEngine&, const QJSValue& value);
    static QRegExp RegExp2QRegExp (const QJSValue& jsValue);
    static bool warnJsError (QJSEngine& jsEngine, const QJSValue& jsValue, const QByteArray& msg = QByteArray());
    inline static bool valueIsEmpty (const QJSValue& value) {
        return (value.isUndefined() || value.isNull());
    }
private slots:
    void timerFinished (unsigned int timerId);
    void networkRequestFinished (unsigned int _networkRequestId);
signals:
    void valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue);
};

#endif // JAVASCRIPTBRIDGE_H
