#ifndef JAVASCRIPTBRIDGE_H
#define JAVASCRIPTBRIDGE_H

#include <QDateTime>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QtDebug>

class JavascriptMethod {
public:
    static const int getSupportedUrls;
    static const int getObjectFromUrl;
    static const int getPropertiesFromObject;
    static const QStringList requiredMethods;
};

class JavascriptBridge : public QObject {
    Q_OBJECT
private:
    QJSValue myself;
    QString requestChannel;
    int transactionId;
    QList<int> pendingTransactions;
    QList<int> transactionContexts;
    static QJsonValue QJS2QJsonValue (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonValue& value);
public:
    JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel);
    static QJsonDocument QJS2QJsonDocument (const QJSValue& value);
    static QJSValue QJson2QJS (QJSEngine& jsEngine, const QJsonDocument& value);
    static QString QJS2QString (const QJSValue& value);
    static bool warnJsError (const QJSValue& jsValue, const QString& msg = QString());
    bool invokeMethod (QJSValue& entryPoint, int context, int method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    bool invokeMethod (QJSValue& entryPoint, int context, const QString& method, QJSValue args = QJSValue(QJSValue::UndefinedValue));
    Q_INVOKABLE void receiveValue (int context, const QString& method, const QJSValue& returnedValue);
signals:
    void valueReturnedFromJavascript (int context, const QString& method, const QJSValue& returnedValue);
};

#endif // JAVASCRIPTBRIDGE_H
