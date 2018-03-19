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

//////////////////////////////////////////////////////////////////

JavascriptTimer::JavascriptTimer (QJSEngine& jsEngine, int timerId, bool repeat, int timeout, const QJSValue& callback) :
    QTimer (&jsEngine),
    timerId (timerId),
    jsEngine (&jsEngine),
    callback (callback) {
    this->setSingleShot (! repeat);
    QObject::connect (this, &QTimer::timeout, this, &JavascriptTimer::timerTimeout);
    this->setInterval (timeout);
}

void JavascriptTimer::timerTimeout () {
    bool finishTimer = false;
    QJSValue returnValue;
    if (this->callback.isCallable ()) {
        returnValue = this->callback.call ();
    } else if (this->callback.isString ()) {
        returnValue = this->jsEngine->evaluate (this->callback.toString ());
    } else {
        qWarning() << "JavascriptTimer triggered, but callback value is not a function nor a string.";
        finishTimer = true;
    }
    if (JavascriptBridge::warnJsError (returnValue, "Uncaught exception received while calling method specified as either 'setTimeout();' or 'setInterval();' argument!")) {
        finishTimer = true;
    }
    if (this->isSingleShot () || finishTimer) {
        this->stop ();
        emit timerFinished (timerId);
    }
}

//////////////////////////////////////////////////////////////////

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

QJSValue JavascriptBridge::createTimer (const bool repeat, const QJSValue& callback, const int interval) {
    QJSEngine* jsEngine (qjsEngine (this));
    if (jsEngine != Q_NULLPTR) {
        int timerId (this->javascriptTimerId);
        this->javascriptTimerId += 2;
        JavascriptTimer* newTimer = new JavascriptTimer ((*jsEngine), timerId, repeat, ((interval > 1) ? interval : 1), callback);
        this->javascriptTimers.insert (timerId, newTimer);
        QObject::connect (newTimer, &JavascriptTimer::timerFinished, this, &JavascriptBridge::timerFinished);
        newTimer->start ();
        return (timerId);
    } else {
        qFatal("JavascriptBridge object must be bound to a QJSEngine! Invoke 'QJSEngine::newQObject();'!");
        return (0);
    }
}

JavascriptBridge::JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel) :
    QObject (Q_NULLPTR),
    myself (jsEngine.newQObject (this)),
    requestChannel (requestChannel),
    transactionId (1),
    javascriptTimerId (1) {
    // "require();" function implementation
    jsEngine.globalObject().setProperty ("require", this->myself.property ("require"));
    // "set/clear Timeout/Interval" functions implementation
    jsEngine.globalObject().setProperty ("setTimeout", this->myself.property ("setTimeout"));
    jsEngine.globalObject().setProperty ("setInterval", this->myself.property ("setInterval"));
    jsEngine.globalObject().setProperty ("clearTimeout", this->myself.property ("clearTimeout"));
    jsEngine.globalObject().setProperty ("clearInterval", this->myself.property ("clearInterval"));
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

/*
 * A reminder: Javascript::invokeMethod must be the last call of every calling function.
 * C++ methods will run out of the expected order if Javascript method returns values immediately, because
 * QQmlEngine will call back the C++ method before the code flow returns to the event loop.
 *
 * Following a call to Javascript::invokeMethod, there should be error handling statements only.
 */

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, int context, int method, QJSValue args) {
    return (this->invokeMethod (entryPoint, context, JavascriptMethod::requiredMethods.value (method), args));
}

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, int context, const QString& method, QJSValue args) {
    if (entryPoint.isCallable ()) {
        int transactionId (this->transactionId);
        this->transactionId += 2;
        this->pendingTransactions[transactionId] = context;
        QJSValue callReturn (entryPoint.call (QJSValueList() << this->myself.property("receiveValue") << transactionId << method << args));
        if (JavascriptBridge::warnJsError (callReturn, QString("Uncaught exception received while calling method '%1' on channel #%2!").arg(method).arg(this->requestChannel))) {
            this->pendingTransactions.remove (transactionId);
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

void JavascriptBridge::receiveValue (int transactionId, const QString& method, const QJSValue& returnedValue) {
    if (this->pendingTransactions.contains (transactionId)) {
        emit valueReturnedFromJavascript (this->pendingTransactions.take(transactionId), method, returnedValue);
    } else {
        qWarning() << QString("Unexpected data '%1' received by a invocation of method '%2' on channel #%3! It will be discarded.").arg(JavascriptBridge::QJS2QString (returnedValue)).arg(method).arg(this->requestChannel);
    }
}

void JavascriptBridge::require (const QJSValue& library) {
    QJSEngine* jsEngine (qjsEngine (this));
    if (jsEngine != Q_NULLPTR) {
        if (library.isString ()) {
            QString libraryName (library.toString ());
            QString libraryCode;
            if (! this->loadedLibraries.contains (libraryName)) {
                {
                    QMutexLocker m_lck (&AppRuntime::commonSourcesMutex);
                    QHash<QString,QString>::const_iterator librarySource (AppRuntime::commonSources.find(libraryName));
                    if (librarySource != AppRuntime::commonSources.constEnd()) {
                        // Force a deep copy of the library source code
                        // http://doc.qt.io/qt-5/implicit-sharing.html
                        libraryCode = QString("%1").arg(librarySource.value());
                    }
                }
                if (libraryCode.isNull ()) {
                    qInfo() << QString("Library '%1' requested by a helper on channel #%2 was not found!").arg(libraryName).arg(this->requestChannel);
                } else {
                    qDebug() << QString("Loading library '%1' requested by a helper on channel #%2...").arg(libraryName).arg(this->requestChannel);
                    this->loadedLibraries.append (libraryName);
                    JavascriptBridge::warnJsError (jsEngine->evaluate(libraryCode, AppConstants::AppCommonSubDir + "/" + libraryName + AppConstants::AppHelperExtension), QString("Uncaught exception found while library '%1' was being loaded on channel #%2...").arg(libraryName).arg(this->requestChannel));
                }
            }
        } else {
            qInfo() << QString("Unable to load a library requested by a helper on channel #%1 because an invalid specification!").arg(this->requestChannel);
        }
    } else {
        qFatal("JavascriptBridge object must be bound to a QJSEngine! Invoke 'QJSEngine::newQObject();'!");
    }
}

QJSValue JavascriptBridge::setTimeout (const QJSValue& callback, const int interval) {
    return (this->createTimer (false, callback, interval));
}

QJSValue JavascriptBridge::setInterval (const QJSValue& callback, const int interval) {
    return (this->createTimer (true, callback, interval));
}

void JavascriptBridge::clearTimeout (int timerId) {
    this->timerFinished (timerId);
}

void JavascriptBridge::clearInterval (int timerId) {
    this->timerFinished (timerId);
}

void JavascriptBridge::timerFinished (int timerId) {
    if (this->javascriptTimers.contains (timerId)) {
        JavascriptTimer* oldTimer = this->javascriptTimers.take (timerId);
        oldTimer->deleteLater ();
    }
}
