#include "javascriptbridge.h"

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
        QJSValueIterator i (value);
        while (i.hasNext ()) {
            i.next ();
            jsonArray.append (JavascriptBridge::QJS2QJsonValue (i.value ()));
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

JavascriptBridge::JavascriptBridge (QObject* parent) :
    QObject (parent) {

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
        return (QString("QRegExp: CaseSensitivity=%1 , PatternSyntax=%2 , Pattern=%3").arg(regExp.caseSensitivity()).arg(regExp.patternSyntax()).arg(regExp.pattern()));
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
