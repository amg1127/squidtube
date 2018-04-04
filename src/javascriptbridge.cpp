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

JavascriptTimer::JavascriptTimer (QJSEngine& jsEngine, unsigned int timerId, bool repeat, int timeout, const QJSValue& callback) :
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

const short int JavascriptNetworkRequest::status_UNSENT = 0;
const short int JavascriptNetworkRequest::status_OPENED = 1;
const short int JavascriptNetworkRequest::status_HEADERS_RECEIVED = 2;
const short int JavascriptNetworkRequest::status_LOADING = 3;
const short int JavascriptNetworkRequest::status_DONE = 4;

bool JavascriptNetworkRequest::getPrivateData (QString key, QJSValue& value) {
    if (this->getPrivateDataCallback.isCallable ()) {
        return (! JavascriptBridge::warnJsError (value = this->getPrivateDataCallback.call (QJSValueList() << key), QString("[XHR#%1] Internal error while getting XMLHttpRequestPrivate.%2!").arg(this->networkRequestId).arg(key)));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::setPrivateData (QString key, const QJSValue& value) {
    if (this->setPrivateDataCallback.isCallable ()) {
        return (! JavascriptBridge::warnJsError (this->setPrivateDataCallback.call (QJSValueList() << key << value), QString("[XHR#%1] Internal error while setting XMLHttpRequestPrivate.%2!").arg(this->networkRequestId).arg(key)));
    } else {
        return (false);
    }
}

void JavascriptNetworkRequest::fulfillRequest (QNetworkAccessManager& networkManager) {
    if (this->networkReply != Q_NULLPTR) {
        this->networkReply->disconnect ();
        if (this->networkReply->isRunning()) {
            this->networkReply->abort ();
            /*
             * http://doc.qt.io/qt-5/qnetworkaccessmanager.html#sendCustomRequest
             *
             * I have to make sure there is no QNetworkReply linked to 'this->requestBodyStream',
             * because I must destroy it now! QDataStream::deleteLater() method does not exist...
             * This statement satisfies that requirement because I just invoked QNetworkReply::abort().
             * It will emit QNetworkReply::finished() signal and free the link.
             *
             */
            QCoreApplication::processEvents ();
        }
        this->networkReply->deleteLater ();
        this->networkReply = Q_NULLPTR;
    }
    QJSValue requestMethod;
    QJSValue requestUrl;
    QJSValue requestHeaders;
    QJSValue requestTimeout;
    QJSValue requestUsername;
    QJSValue requestPassword;
    QJSValue uploadCompleteFlag;
    QJSValue synchronousFlag;
    QJSValue requestBuffer;
    if (this->getPrivateData ("requestMethod", requestMethod) &&
        this->getPrivateData ("requestUrl", requestUrl) &&
        this->getPrivateData ("requestHeaders", requestHeaders) &&
        this->getPrivateData ("requestTimeout", requestTimeout) &&
        this->getPrivateData ("requestUsername", requestUsername) &&
        this->getPrivateData ("requestPassword", requestPassword) &&
        this->getPrivateData ("uploadCompleteFlag", uploadCompleteFlag) &&
        this->getPrivateData ("synchronousFlag", synchronousFlag) &&
        this->getPrivateData ("requestBuffer", requestBuffer)) {
        QUrl url (requestUrl.toString());
        QString username (requestUsername.toString());
        if (! username.isEmpty ()) {
            url.setUserName (username);
        }
        QString password (requestPassword.toString());
        if (! password.isEmpty ()) {
            url.setPassword (password);
        }
        QNetworkRequest networkRequest (url);
        QJSValueIterator requestHeader (requestHeaders);
        while (requestHeader.hasNext ()) {
            requestHeader.next ();
            QString requestHeaderName (requestHeader.name());
            QJSValue requestHeaderValue (requestHeader.value().property("join").call(QJSValueList() << ","));
            if (JavascriptBridge::warnJsError (requestHeaderValue, QString("[XHR#%1] Internal error while fetching request header '%2'!").arg(this->networkRequestId).arg(requestHeaderName))) {
                break;
            } else {
                networkRequest.setRawHeader (requestHeaderName.toLocal8Bit(), requestHeaderValue.toString().toLocal8Bit());
            }
        }
        if (! requestHeader.hasNext ()) {
            // Override the User Agent
            networkRequest.setRawHeader ("user-agent", QString("%1.%2/%3").arg(APP_owner_name).arg(APP_project_name).arg(APP_project_version).toLocal8Bit());
            if (this->requestBodyStream != Q_NULLPTR) {
                delete (this->requestBodyStream);
                this->requestBodyStream = Q_NULLPTR;
            }
            QString strRequestMethod (requestMethod.toString());
            bool uploadCompleteFlagBool = uploadCompleteFlag.toBool();
            int requestBodySize = this->requestBody.count();
            if (((! uploadCompleteFlagBool) || requestBodySize > 0) && strRequestMethod.compare("GET", Qt::CaseInsensitive) && strRequestMethod.compare("HEAD", Qt::CaseInsensitive)) {
                this->requestBody = requestBuffer.toVariant().toByteArray();
                networkRequest.setRawHeader ("content-length", QString::number(requestBodySize).toLocal8Bit());
                this->requestBodyStream = new QDataStream (this->requestBody);
            }
            bool synchronousFlagBool = synchronousFlag.toBool();
            if (! synchronousFlagBool) {
                this->fireProgressEvent (false, "onloadstart", 0, 0);
                if (! uploadCompleteFlagBool) {
                    this->fireProgressEvent (true, "onloadstart", 0, requestBodySize);
#warning I believe this statement should be moved to a function
                    QJSValue state, sendFlag;
                    bool fetchedState = (this->getPrivateData ("state", state) && this->getPrivateData ("sendFlag", sendFlag));
                    if ((! fetchedState) || state.toInt() != JavascriptNetworkRequest::status_OPENED || (! sendFlag.toBool())) {
                        emit networkRequestFinished (networkRequestId);
                        return;
                    }
                }
            }
            int msec = requestTimeout.toInt ();
            qDebug() << QString("[XHR#%1] Performing a HTTP %2 request against '%3'...").arg(this->networkRequestId).arg(strRequestMethod).str(url.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery));
            this->downloadStarted = false;
            this->networkReply = networkManager.sendCustomRequest (networkRequest, strRequestMethod.toLocal8Bit(), this->requestBodyStream);
            QObject::connect (&(this->timeoutTimer), &QTimer::timeout, this->networkReply, &QNetworkReply::abort);
            this->setTimerInterval (msec);
            if (synchronousFlagBool) {
                QEventLoop eventLoop;
                QObject::connect (this->networkReply, &QNetworkReply::finished, &eventLoop, &QEventLoop::exit);
                QObject::connect (this->networkReply, &QNetworkReply::finished, this, &JavascriptNetworkRequest::networkReplyFinished, Qt::DirectConnection);
                eventLoop.exec ();
            } else {
                QObject::connect (this->networkReply, &QNetworkReply::finished, this, &JavascriptNetworkRequest::networkReplyFinished);
                QObject::connect (this->networkReply, &QNetworkReply::downloadProgress, this, &JavascriptNetworkRequest::networkReplyDownloadProgress);
                if (! uploadCompleteFlagBool) {
                    QObject::connect (this->networkReply, &QNetworkReply::uploadProgress, this, &JavascriptNetworkRequest::networkReplyUploadProgress);
                }
            }
            return;
        }
    }
    emit networkRequestFinished (networkRequestId);
}

void JavascriptNetworkRequest::fireProgressEvent (bool isUpload, const QString& callback, const QJSValue& transmitted, const QJSValue& length) {
    if (callback.startsWith ("on")) {
        QJSValue jsCallback;
        if (isUpload) {
            jsCallback = this->xmlHttpRequestObject.property("upload").property(callback);
        } else {
            jsCallback = this->xmlHttpRequestObject.property(callback);
        }
        this->fireProgressEvent (jsCallback, transmitted, length);
    } else {
        qFatal ("Invalid procedure call: 'callback' must start with 'on'!");
    }
}

void JavascriptNetworkRequest::fireProgressEvent (const QJSValue& callback, const QJSValue& transmitted, const QJSValue& length) {
    if (callback.isCallable ()) {
        QJSValue progressEvent = this->jsEngine->newObject ();
        progressEvent.setProperty ("loaded", transmitted);
        progressEvent.setProperty ("length", length);
        progressEvent.setProperty ("lengthComputable", length.toBool ());
        JavascriptBridge::warnJsError (callback.callWithInstance (this->xmlHttpRequestObject, QJSValueList() << progressEvent), QString("[XHR#%1] Uncaught exception received while firing a ProgressEvent!").arg(this->networkRequestId));
    }
}

bool JavascriptNetworkRequest::isFinalAnswer () {
    int httpStatusCode = this->networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    return ((httpStatusCode >= 200 && httpStatusCode < 300) || (httpStatusCode >= 400 && httpStatusCode < 600));
}

void JavascriptNetworkRequest::networkReplyDownloadProgress (qint64 bytesReceived, qint64 bytesTotal) {
    if (this->isFinalAnswer ()) {
        if (! this->downloadStarted) {
#error Continue from here
            this->downloadStarted = true;
        }

#error Then, go here
    }
    qFatal("Not done yet.");
}

void JavascriptNetworkRequest::networkReplyFinished () {
    qFatal("Not done yet.");
}

void JavascriptNetworkRequest::networkReplyUploadProgress (qint64 bytesSent, qint64 bytesTotal) {
    if (bytesTotal >= 0) {
        this->fireProgressEvent (true, "onprogress", bytesSent, bytesTotal);
        if (bytesSent == bytesTotal) {
            this->setPrivateData ("uploadCompleteFlag", true);
            this->fireProgressEvent (true, "onload", bytesSent, bytesTotal);
            this->fireProgressEvent (true, "onloadend", bytesSent, bytesTotal);
        }
    } else {
        this->fireProgressEvent (true, "onprogress", bytesSent, 0);
    }
}

void JavascriptNetworkRequest::timeoutTimerTimeout () {
    this->setPrivateData ("timedOutFlag", true);
}

JavascriptNetworkRequest::JavascriptNetworkRequest (QObject *parent) :
    QObject (parent),
    jsEngine (qjsEngine (parent)),
    requestBodyStream (Q_NULLPTR),
    networkReply (Q_NULLPTR),
    downloadStarted (false) {
    if (this->jsEngine == Q_NULLPTR) {
        qFatal("JavascriptBridge object must be bound to a QJSEngine! Invoke 'QJSEngine::newQObject();'!");
    }
    this->timeoutTimer.setSingleShot (true);
    QObject::connect (&(this->timeoutTimer), &QTimer::timeout, this, &JavascriptNetworkRequest::timeoutTimerTimeout);
}

JavascriptNetworkRequest::~JavascriptNetworkRequest () {
    if (this->networkReply != Q_NULLPTR) {
        this->networkReply->disconnect ();
        if (this->networkReply->isRunning()) {
            this->networkReply->abort ();
            /*
             * http://doc.qt.io/qt-5/qnetworkaccessmanager.html#sendCustomRequest
             *
             * I have to make sure there is no QNetworkReply linked to 'this->requestBodyStream',
             * because I must destroy it now! QDataStream::deleteLater() method does not exist...
             * This statement satisfies that requirement because I just invoked QNetworkReply::abort().
             * It will emit QNetworkReply::finished() signal and free the link.
             *
             */
            QCoreApplication::processEvents ();
        }
        this->networkReply->deleteLater ();
    }
    if (this->requestBodyStream != Q_NULLPTR) {
        delete (this->requestBodyStream);
    }
}

void JavascriptNetworkRequest::setTimerInterval (int msec) {
    this->timeoutTimer.setInterval (msec);
    if (this->timeoutTimer.isActive()) {
        if (msec <= 0) {
            this->timeoutTimer.stop ();
        }
    } else {
        if (msec > 0) {
            this->timeoutTimer.start ();
        }
    }
}

void JavascriptNetworkRequest::start (QNetworkAccessManager& networkManager, unsigned int networkRequestId) {
    this->networkRequestId = networkRequestId;
    this->maxRedirects = 20;
    if (this->setPrivateData ("requestId", networkRequestId)) {
        this->fulfillRequest (networkManager);
    } else {
        emit networkRequestFinished (networkRequestId);
    }
}

void JavascriptNetworkRequest::abort () {
    qFatal("Not done yet.");
}

//////////////////////////////////////////////////////////////////

QJsonValue JavascriptBridge::QJS2QJsonValue (const QJSValue& value) {
    if (value.isBool ()) {
        return (QJsonValue (value.toBool ()));
    } else if (value.isDate ()) {
        // https://bugreports.qt.io/browse/QTBUG-59235
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        return (QJsonValue (value.toDateTime().toString(Qt::ISODateWithMs)));
#else
        return (QJsonValue (value.toDateTime().toString(Qt::ISODate)));
#endif
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

unsigned int JavascriptBridge::createTimer (const bool repeat, const QJSValue& callback, const int interval) {
    QJSEngine* jsEngine (qjsEngine (this));
    if (jsEngine != Q_NULLPTR) {
        unsigned int timerId (this->javascriptTimerId);
        this->javascriptTimerId += 4;
        if (repeat) {
            timerId += 2;
        }
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
    javascriptTimerId (1),
    networkManager (new QNetworkAccessManager (this)),
    networkRequestId (1) {
    // I will handle redirects at application nevel
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
    this->networkManager->setRedirectPolicy (QNetworkRequest::ManualRedirectPolicy);
#endif
    // "require();" function implementation
    jsEngine.globalObject().setProperty ("require", this->myself.property ("require"));
    // "set/clear Timeout/Interval" functions implementation
    jsEngine.globalObject().setProperty ("setTimeout", this->myself.property ("setTimeout"));
    jsEngine.globalObject().setProperty ("setInterval", this->myself.property ("setInterval"));
    jsEngine.globalObject().setProperty ("clearTimeout", this->myself.property ("clearTimeout"));
    jsEngine.globalObject().setProperty ("clearInterval", this->myself.property ("clearInterval"));
    // Console object implementation
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    jsEngine.installExtensions (QJSEngine::ConsoleExtension);
#else
    QJSValue consoleObj = jsEngine.newObject ();
    consoleObj.setProperty ("debug", this->myself.property ("console_log"));
    consoleObj.setProperty ("log", this->myself.property ("console_log"));
    consoleObj.setProperty ("error", this->myself.property ("console_error"));
    consoleObj.setProperty ("exception", this->myself.property ("console_error"));
    consoleObj.setProperty ("info", this->myself.property ("console_info"));
    consoleObj.setProperty ("warn", this->myself.property ("console_warn"));
    jsEngine.globalObject().setProperty ("console", consoleObj);
 #endif
    // Interface for data conversion to and from Unicode
    // My XMLHttpRequest implementation need it
    jsEngine.globalObject().setProperty ("textDecode", this->myself.property ("textDecode"));
    jsEngine.globalObject().setProperty ("textEncode", this->myself.property ("textEncode"));
    // XMLHttpRequest implementation
    QString xmlHttpCode = AppRuntime::readFileContents (":/xmlhttprequest.js");
    if (xmlHttpCode.isNull ()) {
        qFatal("Unable to read resource file ':/xmlhttprequest.js'!");
    } else {
        QJSValue xmlHttpRequestFunction = jsEngine.evaluate (xmlHttpCode, ":/xmlhttprequest.js");
        if (! JavascriptBridge::warnJsError (xmlHttpRequestFunction, "Unable to initialize XMLHttpRequest object within the QJSEngine!")) {
            if (xmlHttpRequestFunction.isCallable ()) {
                // Make sure C++ and Javascript agree with the XMLHttpRequest status codes.
                QJSValue xmlHttpRequest_statusCodes = jsEngine.newObject ();
                xmlHttpRequest_statusCodes.setProperty ("UNSENT", JavascriptNetworkRequest::status_UNSENT);
                xmlHttpRequest_statusCodes.setProperty ("OPENED", JavascriptNetworkRequest::status_OPENED);
                xmlHttpRequest_statusCodes.setProperty ("HEADERS_RECEIVED", JavascriptNetworkRequest::status_HEADERS_RECEIVED);
                xmlHttpRequest_statusCodes.setProperty ("LOADING", JavascriptNetworkRequest::status_LOADING);
                xmlHttpRequest_statusCodes.setProperty ("DONE", JavascriptNetworkRequest::status_DONE);
                QJSValue xmlHttpRequest = xmlHttpRequestFunction.call (QJSValueList()
                    << this->myself.property ("xmlHttpRequest_send")
                    << this->myself.property ("xmlHttpRequest_abort")
                    << this->myself.property ("xmlHttpRequest_setTimeout")
                    << xmlHttpRequest_statusCodes
                );
                if (! JavascriptBridge::warnJsError (xmlHttpRequest, "Unable to retrieve the XMLHttpRequest constructor from the code!")) {
                    jsEngine.globalObject().setProperty ("XMLHttpRequest", xmlHttpRequest);
                }
            } else {
                qCritical() << "XMLHttpRequest initialization procedure did not return a callable object!";
            }
        }
    }
}

JavascriptBridge::~JavascriptBridge () {
    for (QMap<unsigned int,JavascriptNetworkRequest*>::iterator request = this->pendingNetworkRequests.begin(); request != this->pendingNetworkRequests.end(); request++) {
        delete (*request);
    }
    this->pendingNetworkRequests.clear ();
    delete (this->networkManager);
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
        // https://bugreports.qt.io/browse/QTBUG-59235
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        return (value.toDateTime().toString(Qt::ISODateWithMs));
#else
        return (value.toDateTime().toString(Qt::ISODate));
#endif
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
 * QJSEngine will call back the C++ method before the code flow returns to the event loop.
 *
 * Following a call to Javascript::invokeMethod, there should be error handling statements only.
 *
 */

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, unsigned int context, int method, QJSValue args) {
    return (this->invokeMethod (entryPoint, context, JavascriptMethod::requiredMethods.value (method), args));
}

bool JavascriptBridge::invokeMethod (QJSValue& entryPoint, unsigned int context, const QString& method, QJSValue args) {
    if (entryPoint.isCallable ()) {
        unsigned int transactionId (this->transactionId);
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

void JavascriptBridge::receiveValue (unsigned int transactionId, const QString& method, const QJSValue& returnedValue) {
    QMap<unsigned int, unsigned int>::iterator transactionIdIterator = this->pendingTransactions.find (transactionId);
    if (transactionIdIterator != this->pendingTransactions.end()) {
        unsigned int context = (*transactionIdIterator);
        this->pendingTransactions.erase (transactionIdIterator);
        emit valueReturnedFromJavascript (context, method, returnedValue);
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

unsigned int JavascriptBridge::setTimeout (const QJSValue& callback, const int interval) {
    return (this->createTimer (false, callback, interval));
}

unsigned int JavascriptBridge::setInterval (const QJSValue& callback, const int interval) {
    return (this->createTimer (true, callback, interval));
}

void JavascriptBridge::clearTimeout (unsigned int timerId) {
    // This prevent misuse of clearTimeout() and clearInterval() interchangeably
    if (! (timerId & 3)) {
        this->timerFinished (timerId);
    }
}

void JavascriptBridge::clearInterval (unsigned int timerId) {
    // This prevent misuse of clearTimeout() and clearInterval() interchangeably
    if (timerId & 3) {
        this->timerFinished (timerId);
    }
}

void JavascriptBridge::xmlHttpRequest_send (QJSValue& object, QJSValue& getPrivateData, QJSValue& setPrivateData) {
    unsigned int networkRequestId = this->networkRequestId;
    this->networkRequestId += 2;
    JavascriptNetworkRequest* networkRequest = new JavascriptNetworkRequest (this);
    this->pendingNetworkRequests.insert (networkRequestId, networkRequest);
    QObject::connect (networkRequest, &JavascriptNetworkRequest::networkRequestFinished, this, &JavascriptBridge::networkRequestFinished);
    networkRequest->setXMLHttpRequestObject (object);
    networkRequest->setPrivateDataCallbacks (getPrivateData, setPrivateData);
    networkRequest->start (*(this->networkManager), networkRequestId);
}

void JavascriptBridge::xmlHttpRequest_abort (const unsigned int networkRequestId) {
    QMap<unsigned int, JavascriptNetworkRequest*>::iterator networkRequest = this->pendingNetworkRequests.find (networkRequestId);
    if (networkRequest != this->pendingNetworkRequests.end ()) {
        (*networkRequest)->abort ();
    }
}

void JavascriptBridge::xmlHttpRequest_setTimeout (const unsigned int networkRequestId, const int msec) {
    QMap<unsigned int, JavascriptNetworkRequest*>::iterator networkRequest = this->pendingNetworkRequests.find (networkRequestId);
    if (networkRequest != this->pendingNetworkRequests.end ()) {
        (*networkRequest)->setTimerInterval (msec);
    }
}

QString JavascriptBridge::textDecode (const QByteArray& bytes, const QString& fallbackCharset) {
    QString answer;
    QMutexLocker m_lck (&AppRuntime::textCoDecMutex);
    QTextCodec* textCodec = QTextCodec::codecForUtfText (bytes, QTextCodec::codecForName (fallbackCharset.toUtf8()));
    if (textCodec != Q_NULLPTR) {
        QTextDecoder textDecoder (textCodec, QTextCodec::ConvertInvalidToNull);
        answer = textDecoder.toUnicode (bytes);
    }
    return (answer);
}

QByteArray JavascriptBridge::textEncode (const QString& string, const QString& charset) {
    QByteArray answer;
    QMutexLocker m_lck (&AppRuntime::textCoDecMutex);
    QTextCodec* textCodec = QTextCodec::codecForName (charset.toUtf8());
    if (textCodec != Q_NULLPTR) {
        QTextEncoder textEncoder (textCodec, QTextCodec::ConvertInvalidToNull);
        answer = textEncoder.fromUnicode (string);
    }
    return (answer);
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
void JavascriptBridge::console_log (const QJSValue& msg) {
    qDebug() << QString("QJS: ") + msg.toString();
}

void JavascriptBridge::console_info (const QJSValue& msg) {
    qInfo() << QString("QJS: ") + msg.toString();
}

void JavascriptBridge::console_warn (const QJSValue& msg) {
    qWarning() << QString("QJS: ") + msg.toString();
}

void JavascriptBridge::console_error (const QJSValue& msg) {
    qCritical() << QString("QJS: ") + msg.toString();
}
#endif

void JavascriptBridge::timerFinished (unsigned int timerId) {
    QMap<unsigned int,JavascriptTimer*>::iterator timerIdIterator = this->javascriptTimers.find (timerId);
    if (timerIdIterator != this->javascriptTimers.end()) {
        JavascriptTimer* oldTimer = (*timerIdIterator);
        this->javascriptTimers.erase (timerIdIterator);
        oldTimer->deleteLater ();
    }
}

void JavascriptBridge::networkRequestFinished (unsigned int networkRequestId) {
    QMap<unsigned int,JavascriptNetworkRequest*>::iterator networkRequestIterator = this->pendingNetworkRequests.find (networkRequestId);
    if (networkRequestIterator != this->pendingNetworkRequests.end()) {
        JavascriptNetworkRequest* oldNetworkRequest = (*networkRequestIterator);
        this->pendingNetworkRequests.erase (networkRequestIterator);
        oldNetworkRequest->deleteLater ();
        qDebug() << QString("XMLHttpRequest #%1 has finished.").arg(networkRequestId);
    }
}
