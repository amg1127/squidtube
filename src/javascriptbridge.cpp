#include "javascriptbridge.h"

const int JavascriptMethod::getSupportedUrls = 0;
const int JavascriptMethod::getObjectFromUrl = 1;
const int JavascriptMethod::getPropertiesFromObject = 2;
const QStringList JavascriptMethod::requiredMethods (
    QStringList()
        << "getSupportedUrls"
        << "getObjectFromUrl"
        << "getPropertiesFromObject"
);

QJsonValue JavascriptBridge::QJS2QJsonValue (const QJSValue& value) {
    if (value.isBool ()) {
        return (QJsonValue (value.toBool ()));
    } else if (value.isDate ()) {
        return (QJsonValue (value.toDateTime().toString(Qt::ISODateWithMs)));
    } else if (value.isNull ()) {
        return (QJsonValue (QJsonValue::Null));
    } else if (value.isNumber ()) {
        return (QJsonValue (value.toNumber()));
    } else if (value.isString ()) {
        return (QJsonValue (value.toString()));
    } else if (value.isArray ()) {
        QJsonArray jsonArray;
        int length = value.property("length").toInt ();
        for (int i = 0; i < length; i++) {
            jsonArray.append (JavascriptBridge::QJS2QJsonValue (value.property (i)));
        }
        return (jsonArray);
    } else if (value.isObject ()) {
        QJsonObject jsonObject;
        QJSValueIterator i (value);
        while (i.hasNext ()) {
            i.next ();
            jsonObject.insert (i.name(), JavascriptBridge::QJS2QJsonValue (i.value ()));
        }
        return (jsonObject);
    } else {
        return (QJsonValue (QJsonValue::Undefined));
    }
}

QJSValue JavascriptBridge::QJson2QJS (QJSEngine& jsEngine, const QJsonValue& value) {
    if (value.isBool ()) {
        return (QJSValue (value.toBool ()));
    } else if (value.isDouble ()) {
        return (QJSValue (value.toDouble ()));
    } else if (value.isString ()) {
        return (QJSValue (value.toString ()));
    } else if (value.isNull ()) {
        return (QJSValue (QJSValue::NullValue));
    } else if (value.isArray ()) {
        QJsonArray jsonArray (value.toArray ());
        QJSValue jsValue (jsEngine.newArray (jsonArray.count()));
        quint32 pos = 0;
        for (QJsonArray::const_iterator i = jsonArray.constBegin(); i != jsonArray.constEnd(); i++) {
            jsValue.setProperty (pos++, JavascriptBridge::QJson2QJS (jsEngine, (*i)));
        }
        return (jsValue);
    } else if (value.isObject ()) {
        QJsonObject jsonObject (value.toObject ());
        QJSValue jsValue (jsEngine.newObject ());
        for (QJsonObject::const_iterator i = jsonObject.constBegin(); i != jsonObject.constEnd(); i++) {
            jsValue.setProperty (i.key(), JavascriptBridge::QJson2QJS (jsEngine, i.value ()));
        }
        return (jsValue);
    } else {
        return (QJSValue (QJSValue::UndefinedValue));
    }
}

JavascriptBridge::JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel) :
    QObject (Q_NULLPTR),
    myself (jsEngine.newQObject (this)),
    requestChannel (requestChannel),
    transactionId (1) {
}

QJSValue JavascriptBridge::QJson2QJS (QJSEngine& jsEngine, const QJsonDocument& value) {
    if (value.isNull ()) {
        return (QJSValue (QJSValue::NullValue));
    } else if (value.isEmpty ()) {
        return (QJSValue (QJSValue::UndefinedValue));
    } else if (value.isArray ()) {
        return (JavascriptBridge::QJson2QJS (jsEngine, value.array ()));
    } else if (value.isObject ()) {
        return (JavascriptBridge::QJson2QJS (jsEngine, value.object ()));
    } else {
        qFatal (QString("Unable to handle the received QJsonDocument: '%1'!").arg(QString::fromUtf8(value.toJson(QJsonDocument::Compact))).toLocal8Bit().constData());
    }
}

QJsonDocument JavascriptBridge::QJS2QJsonDocument (const QJSValue& value) {
    if (value.isArray ()) {
        return (QJsonDocument (JavascriptBridge::QJS2QJsonValue(value).toArray ()));
    } else if (value.isObject ()) {
        return (QJsonDocument (JavascriptBridge::QJS2QJsonValue(value).toObject ()));
    } else {
        return (QJsonDocument ());
    }
}

QString JavascriptBridge::QJS2QString (const QJSValue& value) {
    if (value.isBool ()) {
        if (value.toBool ()) {
            return ("true");
        } else {
            return ("false");
        }
    } else if (value.isCallable ()) {
        return ("function () { /* Native code */ }");
    } else if (value.isDate ()) {
        return (value.toDateTime().toString(Qt::ISODateWithMs));
    } else if (value.isNull ()) {
        return ("null");
    } else if (value.isNumber ()) {
        return (QString::number (value.toNumber()));
    } else if (value.isRegExp ()) {
        QRegExp regExp (value.toVariant().toRegExp());
        return (QString("QRegExp: CaseSensitivity=%1 , PatternSyntax=%2 , Pattern='%3'").arg(regExp.caseSensitivity()).arg(regExp.patternSyntax()).arg(regExp.pattern()));
    } else if (value.isString ()) {
        return (value.toString());
    } else if (value.isUndefined ()) {
        return ("undefined");
    } else {
        QJsonDocument jsonDocument (JavascriptBridge::QJS2QJsonDocument(value));
        if (jsonDocument.isNull ()) {
            return ("(conversion error)");
        } else {
            return (QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Compact)));
        }
    }
}

bool JavascriptBridge::warnJsError (const QJSValue& jsValue, const QString& msg) {
    if (jsValue.isError ()) {
        qInfo() << QString("Javascript exception at %1:%2: %3: %4")
            .arg(jsValue.property("fileName").toString())
            .arg(jsValue.property("lineNumber").toInt())
            .arg(jsValue.property("name").toString())
            .arg(jsValue.property("message").toString());
        qDebug() << jsValue.property("stack").toString().toLocal8Bit().constData();
        if (! msg.isEmpty ()) {
            qWarning() << msg;
        }
        return (true);
    } else {
        return (false);
    }
}

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, int context, int method, QJSValue args) {
    return (this->invokeMethod (entryPoint, context, JavascriptMethod::requiredMethods.value (method), args));
}

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, int context, const QString& method, QJSValue args) {
    if (entryPoint.isCallable ()) {
        int transactionId (this->transactionId);
        this->transactionId += 2;
        this->pendingTransactions.prepend (transactionId);
        this->transactionContexts.prepend (context);
        QJSValue callReturn (entryPoint.call (QJSValueList() << this->myself.property("receiveValue") << transactionId << method << args));
        if (JavascriptBridge::warnJsError (callReturn, QString("Uncaught exception received while calling method '%1' on channel #%2!").arg(method).arg(this->requestChannel))) {
            int pos = this->pendingTransactions.indexOf (transactionId);
            if (pos >= 0) {
                this->pendingTransactions.removeAt (pos);
                this->transactionContexts.removeAt (pos);
            }
        } else {
            if (! callReturn.isUndefined ()) {
                qWarning() << QString("Value '%1' returned from a call to method '%2' on channel #%3 will be discarded. Please, return values by using the following statement: 'arguments[0].call (returnValue);' !").arg(JavascriptBridge::QJS2QString (entryPoint)).arg(method).arg(this->requestChannel);
            }
            return (true);
        }
    } else {
        qWarning() << QString ("An attempt to call a non-function value '%1' within channel #%2 failed!").arg(JavascriptBridge::QJS2QString (entryPoint)).arg(this->requestChannel);
    }
    return (false);
}

void JavascriptBridge::receiveValue (int context, const QString& method, const QJSValue& returnedValue) {
    int pos = this->pendingTransactions.indexOf (context);
    if (pos >= 0) {
        this->pendingTransactions.removeAt (context);
        emit valueReturnedFromJavascript (this->transactionContexts.takeAt(pos), method, returnedValue);
    } else {
        qWarning() << QString("Unexpected data '%1' received by a invocation of method '%2' on channel #%3! It will be discarded.").arg(JavascriptBridge::QJS2QString (returnedValue)).arg(method).arg(this->requestChannel);
    }
}
