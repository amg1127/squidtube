#ifndef JAVASCRIPTBRIDGE_H
#define JAVASCRIPTBRIDGE_H

#include "appruntime.h"

#include <QDateTime>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QMutexLocker>
#include <QObject>
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

#warning Definir XMLHttpRequest no Javascript
#warning Definir o metodo XMLHttpRequest.send() dentro do C++, passando todo o XMLHttpRequest como um QJSValue
#warning Chamar os event handlers usando QJSValue.call()

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

class JavascriptBridge : public QObject {
    Q_OBJECT
private:
    QJSValue myself;
    QString requestChannel;
    unsigned int transactionId;
    QMap<unsigned int,unsigned int> pendingTransactions;
    QStringList loadedLibraries;
    QMap<unsigned int,JavascriptTimer*> javascriptTimers;
    QMap<unsigned int,QJSValue> JavascriptXMLHttpRequests;
    unsigned int javascriptTimerId;
    static QJsonValue QJS2QJsonValue (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonValue& value);
    QJSValue createTimer (const bool repeat, const QJSValue& callback, const int interval);
public:
    JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel);
    static QJsonDocument QJS2QJsonDocument (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonDocument& value);
    static QString QJS2QString (const QJSValue& value);
    static bool warnJsError (const QJSValue& jsValue, const QString& msg = QString());
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, int method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    bool invokeMethod (QJSValue& entryPoint, unsigned int context, const QString& method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    Q_INVOKABLE void receiveValue (unsigned int transactionId, const QString& method, const QJSValue& returnedValue);
    Q_INVOKABLE void require (const QJSValue& library);
    Q_INVOKABLE QJSValue setTimeout (const QJSValue& callback, const int interval);
    Q_INVOKABLE QJSValue setInterval (const QJSValue& callback, const int interval);
    Q_INVOKABLE void clearTimeout (unsigned int timerId);
    Q_INVOKABLE void clearInterval (unsigned int timerId);
    Q_INVOKABLE void xmlHttpRequest_abort (QJSValue& object, QJSValue& getPrivateData, QJSValue& setPrivateData);
    Q_INVOKABLE void xmlHttpRequest_send (QJSValue& object, QJSValue& requestBody, QJSValue& getPrivateData, QJSValue& setPrivateData);
    Q_INVOKABLE QJSValue TextDecode (const QJSValue& bytes, const QJSValue& fallbackCharset);
    Q_INVOKABLE QJSValue TextEncode (const QJSValue& string, const QJSValue& charset);
    Q_INVOKABLE void console_log (const QJSValue& msg);
    Q_INVOKABLE void console_info (const QJSValue& msg);
    Q_INVOKABLE void console_warn (const QJSValue& msg);
    Q_INVOKABLE void console_error (const QJSValue& msg);
private slots:
    void timerFinished (unsigned int timerId);
signals:
    void valueReturnedFromJavascript (unsigned int context, const QString& method, const QJSValue& returnedValue);
};

#endif // JAVASCRIPTBRIDGE_H
