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
    if (JavascriptBridge::warnJsError ((*(this->jsEngine)), returnValue, "Uncaught exception received while calling method specified as either 'setTimeout();' or 'setInterval();' argument!")) {
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

bool JavascriptNetworkRequest::getPrivateData (const QString& key, QJSValue& value) {
    if (this->getPrivateDataCallback1.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), value = this->getPrivateDataCallback1.call (QJSValueList() << key), QString("[XHR#%1] Internal error while getting XMLHttpRequestPrivate.%2!").arg(this->networkRequestId).arg(key)));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::setPrivateData (const QString& key, const QJSValue& value) {
    if (this->setPrivateDataCallback1.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->setPrivateDataCallback1.call (QJSValueList() << key << value), QString("[XHR#%1] Internal error while setting XMLHttpRequestPrivate.%2!").arg(this->networkRequestId).arg(key)));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::getPrivateData (const QString& key, const QString& subKey, QJSValue& value) {
    if (this->getPrivateDataCallback2.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), value = this->getPrivateDataCallback2.call (QJSValueList() << key << subKey), QString("[XHR#%1] Internal error while getting XMLHttpRequestPrivate.%2.%3!").arg(this->networkRequestId).arg(key).arg(subKey)));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::setPrivateData (const QString& key, const QString& subKey, const QJSValue& value) {
    if (this->setPrivateDataCallback2.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->setPrivateDataCallback2.call (QJSValueList() << key << subKey << value), QString("[XHR#%1] Internal error while setting XMLHttpRequestPrivate.%2.%3!").arg(this->networkRequestId).arg(key).arg(subKey)));
    } else {
        return (false);
    }
}

void JavascriptNetworkRequest::cancelRequest (bool emitNetworkRequestFinished) {
    if (this->networkReply != Q_NULLPTR) {
        this->networkReply->disconnect (this);
        if (this->networkReply->isRunning()) {
            this->networkReply->abort ();
        }
        /*
         * http://doc.qt.io/qt-5/qnetworkaccessmanager.html#sendCustomRequest
         *
         * I have to make sure there is no QNetworkReply linked to 'this->requestBodyBuffer',
         * because I may want to destroy it later...
         * This statement satisfies that requirement because I just invoked QNetworkReply::abort().
         * It will emit QNetworkReply::finished() signal and free the link.
         *
         */
        QCoreApplication::processEvents ();
        this->networkReply->deleteLater ();
        this->networkReply = Q_NULLPTR;
    }
    if (emitNetworkRequestFinished) {
        QJSValue networkRequestId;
        if (this->getPrivateData ("requestId", networkRequestId)) {
            if (networkRequestId.toUInt() == this->networkRequestId) {
                this->setPrivateData ("requestId", QJSValue(QJSValue::NullValue));
            }
        }
        emit networkRequestFinished (this->networkRequestId);
    }
}

void JavascriptNetworkRequest::fulfillRequest (QNetworkAccessManager& networkManager) {
    this->cancelRequest ();
    QJSValue requestMethod;
    QJSValue requestUrl;
    QJSValue requestHeaders;
    QJSValue requestTimeout;
    QJSValue requestUsername;
    QJSValue requestPassword;
    QJSValue uploadCompleteFlag;
    QJSValue synchronousFlag;
    QJSValue requestBuffer;
    this->setPrivateData ("responseBuffer", QJSValue(QJSValue::NullValue));
    if (this->getPrivateData ("requestMethod", requestMethod) &&
        this->getPrivateData ("requestUrl", requestUrl) &&
        this->getPrivateData ("requestHeaders", requestHeaders) &&
        this->getPrivateData ("requestTimeout", requestTimeout) &&
        this->getPrivateData ("requestUsername", requestUsername) &&
        this->getPrivateData ("requestPassword", requestPassword) &&
        this->getPrivateData ("uploadCompleteFlag", uploadCompleteFlag) &&
        this->getPrivateData ("synchronousFlag", synchronousFlag) &&
        this->getPrivateData ("requestBuffer", requestBuffer)) {
        QUrl url;
        if (! JavascriptBridge::valueIsEmpty (requestUrl)) {
            url.setUrl (requestUrl.toString());
        }
        if (! JavascriptBridge::valueIsEmpty(requestUsername)) {
            QString username (requestUsername.toString());
            if (! username.isEmpty()) {
                url.setUserName (username);
            }
        }
        if (! JavascriptBridge::valueIsEmpty(requestPassword)) {
            QString password (requestPassword.toString());
            if (! password.isEmpty()) {
                url.setPassword (password);
            }
        }
        QNetworkRequest networkRequest (url);
        QJSValueIterator requestHeader (requestHeaders);
        QString requestHeaderName;
        QJSValue requestHeaderValueList;
        uint requestHeaderValueListLength;
        uint requestHeaderValueListPos;
        QStringList requestHeaderValues;
        QJSValue requestHeaderValue;
        while (requestHeader.hasNext ()) {
            requestHeader.next ();
            requestHeaderName = requestHeader.name();
            requestHeaderValueList = requestHeader.value();
            if (requestHeaderValueList.isArray()) {
                requestHeaderValueListLength = requestHeaderValueList.property("length").toUInt();
                for (requestHeaderValueListPos = 0; requestHeaderValueListPos < requestHeaderValueListLength; requestHeaderValueListPos++) {
                    requestHeaderValue = requestHeaderValueList.property(requestHeaderValueListPos);
                    if (! JavascriptBridge::valueIsEmpty (requestHeaderValue)) {
                        requestHeaderValues.append (requestHeaderValue.toString());
                    }
                }
                networkRequest.setRawHeader (requestHeaderName.toLocal8Bit(), requestHeaderValues.join(',').toLocal8Bit());
                requestHeaderValues.clear ();
            }
        }
        // Override the User Agent
        networkRequest.setRawHeader ("user-agent", QString("%1.%2/%3").arg(APP_owner_name).arg(APP_project_name).arg(APP_project_version).toLocal8Bit());
        if (! JavascriptBridge::valueIsEmpty (requestMethod)) {
            this->synchronousFlag = synchronousFlag.toBool();
            this->requestMethod = requestMethod.toString();
            if (! this->requestMethod.isEmpty()) {
                this->requestBodyBuffer.setData (JavascriptBridge::ArrayBuffer2QByteArray ((*(this->jsEngine)), requestBuffer));
                if (! this->requestBodyBuffer.open (QIODevice::ReadOnly)) {
                    qCritical() << QString("[XHR#%1] Internal error while allocating request body buffer: %2").arg(this->networkRequestId).arg(this->requestBodyBuffer.errorString());
                }
                bool uploadCompleteFlagBool = uploadCompleteFlag.toBool();
                qint64 requestBodySize = this->requestBodyBuffer.size();
                if (! this->isGetOrHeadRequest ()) {
                    if ((! uploadCompleteFlagBool) || requestBodySize > 0) {
                        networkRequest.setRawHeader ("content-length", QString::number(requestBodySize).toLocal8Bit());
                    }
                }
                this->fireProgressEvent (false, "onloadstart", 0, 0);
                if (! uploadCompleteFlagBool) {
                    this->fireProgressEvent (true, "onloadstart", 0, requestBodySize);
                    QJSValue state, sendFlag;
                    bool fetchedState = (this->getPrivateData ("state", state) && this->getPrivateData ("sendFlag", sendFlag));
                    if ((! fetchedState) || state.toInt() != JavascriptNetworkRequest::status_OPENED || (! sendFlag.toBool())) {
                        this->cancelRequest (true);
                        return;
                    }
                }
                int msec = requestTimeout.toInt ();
                qDebug() << QString("[XHR#%1] Performing a HTTP %2 request against '%3'...").arg(this->networkRequestId).arg(this->requestMethod).arg(url.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery));
                this->downloadStarted = false;
                this->responseStatus = 0;
                this->networkReply = networkManager.sendCustomRequest (networkRequest, this->requestMethod.toLocal8Bit(), &(this->requestBodyBuffer));
                QObject::connect (&(this->timeoutTimer), &QTimer::timeout, this->networkReply, &QNetworkReply::abort);
                this->setTimerInterval (msec);
                QObject::connect (this->networkReply, &QNetworkReply::finished, this, &JavascriptNetworkRequest::networkReplyFinished);
                QObject::connect (this->networkReply, &QNetworkReply::downloadProgress, this, &JavascriptNetworkRequest::networkReplyDownloadProgress);
                if (! uploadCompleteFlagBool) {
                    QObject::connect (this->networkReply, &QNetworkReply::uploadProgress, this, &JavascriptNetworkRequest::networkReplyUploadProgress);
                }
                if (this->synchronousFlag) {
                    QEventLoop eventLoop;
                    QObject::connect (this->networkReply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
                    eventLoop.exec ();
                }
                return;
            }
        }
    }
    this->cancelRequest (true);
}

void JavascriptNetworkRequest::fireProgressEvent (bool isUpload, const QString& callback, qint64 transmitted, qint64 length) {
    if (! this->synchronousFlag) {
        if (callback.startsWith ("on")) {
            QJSValue jsCallback;
            if (isUpload) {
                jsCallback = this->xmlHttpRequestObject.property("upload").property(callback);
            } else {
                jsCallback = this->xmlHttpRequestObject.property(callback);
            }
            this->fireProgressEvent (jsCallback, transmitted, length);
        } else {
            qFatal("Invalid procedure call: 'callback' must start with 'on'!");
        }
    }
}

void JavascriptNetworkRequest::fireProgressEvent (QJSValue& callback, qint64 transmitted, qint64 length) {
    if ((! this->synchronousFlag) && callback.isCallable ()) {
        QJSValue networkRequestId;
        if (this->getPrivateData ("requestId", networkRequestId)) {
            if (networkRequestId.toUInt() == this->networkRequestId) {
                QJSValue progressEvent = this->jsEngine->newObject ();
                // I will lose precision here...
                uint total = (uint) ((length > 0) ? length : 0);
                progressEvent.setProperty ("loaded", (uint) transmitted);
                progressEvent.setProperty ("total", total);
                progressEvent.setProperty ("lengthComputable", (bool) total);
                JavascriptBridge::warnJsError ((*(this->jsEngine)), callback.callWithInstance (this->xmlHttpRequestObject, QJSValueList() << progressEvent), QString("[XHR#%1] Uncaught exception received while firing a ProgressEvent!").arg(this->networkRequestId));
            }
        }
    }
}

void JavascriptNetworkRequest::fireEvent (const QString& callback) {
    if (! this->synchronousFlag) {
        if (callback.startsWith ("on")) {
            QJSValue networkRequestId;
            if (this->getPrivateData ("requestId", networkRequestId)) {
                if (networkRequestId.toUInt() == this->networkRequestId) {
                    JavascriptBridge::warnJsError ((*(this->jsEngine)), this->xmlHttpRequestObject.property(callback).callWithInstance (this->xmlHttpRequestObject), QString("[XHR#%1] Uncaught exception received while firing the '%2' event!").arg(this->networkRequestId).arg(callback));
                }
            }
        } else {
            qFatal("Invalid procedure call: 'callback' must start with 'on'!");
        }
    }
}

void JavascriptNetworkRequest::appendResponseBuffer () {
    if (this->networkReply->isOpen ()) {
        QJSValue responseBuffer;
        if (this->getPrivateData ("responseBuffer", responseBuffer)) {
            this->setPrivateData ("responseBuffer", JavascriptBridge::QByteArray2ArrayBuffer ((*(this->jsEngine)), JavascriptBridge::ArrayBuffer2QByteArray ((*(this->jsEngine)), responseBuffer) + this->networkReply->readAll ()));

            this->getPrivateData ("responseBuffer", responseBuffer);
        }
    }
}

void JavascriptNetworkRequest::networkReplyUploadProgress (qint64 bytesSent, qint64 bytesTotal) {
    if (this->networkReply->error() == QNetworkReply::NoError) {
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
}

void JavascriptNetworkRequest::networkReplyDownloadProgress (qint64 bytesReceived, qint64 bytesTotal) {
    if (! this->responseStatus) {
        this->responseStatus = this->networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    if (this->networkReply->error() == QNetworkReply::NoError && this->isFinalAnswer ()) {
        if (! this->downloadStarted) {
            this->downloadStarted = true;
            QJSValue state (JavascriptNetworkRequest::status_HEADERS_RECEIVED);
            this->setPrivateData ("state", state);
            this->setPrivateData ("status", this->responseStatus);
            this->setPrivateData ("statusText", this->networkReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
            this->setPrivateData ("responseURL", this->networkReply->url().toString());
            QJSValue responseHeadersObject = this->jsEngine->newObject ();
            QList<QNetworkReply::RawHeaderPair> responseHeaders (this->networkReply->rawHeaderPairs ());
            for (QList<QNetworkReply::RawHeaderPair>::const_iterator headerPair = responseHeaders.constBegin(); headerPair != responseHeaders.constEnd(); headerPair++) {
                QString headerName (QString::fromUtf8(headerPair->first).toLower());
                QString headerValue (QString::fromUtf8(headerPair->second));
                if (! responseHeadersObject.hasProperty (headerName)) {
                    responseHeadersObject.setProperty (headerName, this->jsEngine->newArray ());
                }
                responseHeadersObject.property(headerName).setProperty (responseHeadersObject.property(headerName).property("length").toUInt(), headerValue);
            }
            this->setPrivateData ("responseHeaders", responseHeadersObject);
            this->fireEvent ("onreadystatechange");
            if (this->getPrivateData ("state", state)) {
                if (state.toInt() != JavascriptNetworkRequest::status_HEADERS_RECEIVED) {
                    this->cancelRequest (true);
                    return;
                }
            }
            if (bytesReceived > 0 || bytesTotal > 0) {
                this->setPrivateData ("state", JavascriptNetworkRequest::status_LOADING);
            }
        }
        this->appendResponseBuffer ();
        if (bytesReceived != bytesTotal) {
            this->fireEvent ("onreadystatechange");
            this->fireProgressEvent (false, "onprogress", bytesReceived, bytesTotal);
        } else {
            this->fireProgressEvent (false, "onprogress", bytesReceived, bytesTotal);
            this->setPrivateData ("state", JavascriptNetworkRequest::status_DONE);
            this->setPrivateData ("sendFlag", false);
            this->setPrivateData ("responseObject", "type", "object"); // The XMLHttpRequest specification did not rule this...
            this->fireEvent ("onreadystatechange");
            this->fireProgressEvent (false, "onload", bytesReceived, bytesTotal);
            this->fireProgressEvent (false, "onloadend", bytesReceived, bytesTotal);
        }
    }
}

void JavascriptNetworkRequest::networkReplyFinished () {
    this->requestBodyBuffer.close ();
    QNetworkReply::NetworkError networkError = this->networkReply->error ();
    if (this->isFinalAnswer() || networkError != QNetworkReply::NoError) {
        this->appendResponseBuffer ();
        if (networkError != QNetworkReply::NoError) {
            qInfo() << QString("[XHR#%1] Network request finished unexpectedly: %2: %3").arg(this->networkRequestId).arg(networkError).arg(this->networkReply->errorString());
            QJSValue sendFlag;
            if (this->getPrivateData ("sendFlag", sendFlag)) {
                if (! sendFlag.toBool ()) {
                    this->cancelRequest (true);
                    return;
                }
            }
            this->setPrivateData ("state", JavascriptNetworkRequest::status_DONE);
            this->setPrivateData ("sendFlag", false);
            this->setPrivateData ("responseObject", "type", "failure");
            if (networkError == QNetworkReply::RemoteHostClosedError && this->downloadStarted) {
                if (this->responseStatus >= 200 && this->responseStatus < 300) {
                    this->setPrivateData ("status", 0);
                    this->setPrivateData ("statusText", QString("NetworkError: %1").arg(this->networkReply->errorString ()));
                }
                this->setPrivateData ("responseObject", "value", "NetworkError");
            } else {
                QString exception ("NetworkError");
                QString eventHandler ("onerror");
                if (networkError == QNetworkReply::OperationCanceledError) {
                    // Either the request did not finish within the configured timeout or XMLHttpRequest.prototype.abort() was called
                    // QNetworkReply::abort() is called in both cases.
                    QJSValue timedOutFlag, abortedFlag;
                    if (this->getPrivateData ("timedOutFlag", timedOutFlag) && this->getPrivateData ("abortedFlag", abortedFlag)) {
                        if (timedOutFlag.toBool()) {
                            exception = "TimeoutError";
                            eventHandler = "ontimeout";
                        } else if (abortedFlag.toBool()) {
                            exception = "AbortError";
                            eventHandler = "onabort";
                        }
                    }
                }
                QString errorString (QString("%1: %2").arg(exception).arg(this->networkReply->errorString ()));
                this->setPrivateData ("status", 0);
                this->setPrivateData ("statusText", errorString);
                this->setPrivateData ("responseObject", "value", errorString);
                this->fireEvent ("onreadystatechange");
                QJSValue uploadCompleteFlag;
                if (this->getPrivateData ("uploadCompleteFlag", uploadCompleteFlag)) {
                    if (! uploadCompleteFlag.toBool()) {
                        this->setPrivateData ("uploadCompleteFlag", true);
                        this->fireProgressEvent (true, eventHandler, 0, 0);
                        this->fireProgressEvent (true, "onloadend", 0, 0);
                    }
                }
                this->fireProgressEvent (false, eventHandler, 0, 0);
                this->fireProgressEvent (false, "onloadend", 0, 0);
            }
        }
        this->cancelRequest (true);
    } else {
        // Handle the URL redirect
        if (this->isRedirectWithMethodChange()) {
            if (! this->isGetOrHeadRequest ()) {
                this->setPrivateData ("requestMethod", "GET");
            }
            this->setPrivateData ("requestBuffer", QJSValue(QJSValue::NullValue));
            this->setPrivateData ("requestUsername", QJSValue(QJSValue::NullValue));
            this->setPrivateData ("requestPassword", QJSValue(QJSValue::NullValue));
        }
        QJSValue requestUrl;
        if (this->getPrivateData ("requestUrl", requestUrl)) {
            QJSValue requestHeaderReferer (this->jsEngine->newArray (1));
            requestHeaderReferer.setProperty (0, requestUrl);
            this->setPrivateData ("requestHeaders", "referer", requestHeaderReferer);
        }
        QString location (QString::fromUtf8 (this->networkReply->rawHeader ("location")));
        if (location.isEmpty()) {
            qInfo() << QString("[XHR#%1] Server replied HTTP status '%2 %3' and did not send a 'Location:' header. I will fetch the same URL again!").arg(this->networkRequestId).arg(this->networkReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()).arg(this->responseStatus);
        } else {
            this->setPrivateData ("requestUrl", this->networkReply->url().resolved(location).toString());
        }
        this->maxRedirects--;
        this->fulfillRequest (*(this->networkReply->manager ()));
    }
}

void JavascriptNetworkRequest::timeoutTimerTimeout () {
    this->setPrivateData ("timedOutFlag", true);
}

JavascriptNetworkRequest::JavascriptNetworkRequest (QJSEngine& jsEngine, QObject *parent) :
    QObject (parent),
    jsEngine (&jsEngine),
    networkReply (Q_NULLPTR) {
    if (this->jsEngine == Q_NULLPTR) {
        qFatal("JavascriptBridge object must be bound to a QJSEngine! Invoke 'QJSEngine::newQObject();'!");
    }
    this->timeoutTimer.setSingleShot (true);
    QObject::connect (&(this->timeoutTimer), &QTimer::timeout, this, &JavascriptNetworkRequest::timeoutTimerTimeout);
}

JavascriptNetworkRequest::~JavascriptNetworkRequest () {
    this->cancelRequest ();
}

void JavascriptNetworkRequest::setTimerInterval (int msec) {
    if (msec > 0) {
        if (this->timeoutTimer.interval() != msec) {
            this->timeoutTimer.setInterval (msec);
        }
        if (! this->timeoutTimer.isActive()) {
            this->timeoutTimer.start();
        }
    } else {
        if (this->timeoutTimer.isActive()) {
            this->timeoutTimer.stop();
        }
    }
}

void JavascriptNetworkRequest::start (QNetworkAccessManager& networkManager, unsigned int networkRequestId) {
    this->networkRequestId = networkRequestId;
    this->maxRedirects = 20;
    this->responseStatus = 0;
    this->downloadStarted = false;
    if (this->setPrivateData ("requestId", networkRequestId)) {
        this->fulfillRequest (networkManager);
    } else {
        this->cancelRequest (true);
    }
}

void JavascriptNetworkRequest::abort () {
    this->setPrivateData ("abortedFlag", true);
    if (this->networkReply != Q_NULLPTR) {
        if (this->networkReply->isRunning ()) {
            this->networkReply->abort ();
            // XMLHttpRequest.prototype.send() calls JavascriptNetworkRequest::abort() internally.
            // Therefore, QNetworkReply::finished() must be processed before the execution flow returns to Javascript.
            QCoreApplication::processEvents ();
        }
    }
}

//////////////////////////////////////////////////////////////////

unsigned int JavascriptBridge::createTimer (const bool repeat, const QJSValue& callback, const int interval) {
    unsigned int timerId (this->javascriptTimerId);
    this->javascriptTimerId += 4;
    if (repeat) {
        timerId += 2;
    }
    JavascriptTimer* newTimer = new JavascriptTimer ((*(this->jsEngine)), timerId, repeat, ((interval > 1) ? interval : 1), callback);
    this->javascriptTimers.insert (timerId, newTimer);
    QObject::connect (newTimer, &JavascriptTimer::timerFinished, this, &JavascriptBridge::timerFinished);
    newTimer->start ();
    return (timerId);
}

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

JavascriptBridge::JavascriptBridge (QJSEngine& jsEngine, const QString& requestChannel, QObject* parent) :
    QObject (parent),
    jsEngine (&jsEngine),
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
    int scriptLine = __LINE__ + 2;
    QString requireCode (
        "(function (callback) {\n"
        "    return (function (library) {\n"
        "        if (! callback (library)) {\n"
        "            throw new ReferenceError (\"Unable to load library '\" + library + \" into the runtime environment!\");\n"
        "        }\n"
        "    });\n"
        "});\n"
    );
    QJSValue requireEntryPoint = jsEngine.evaluate (requireCode, __FILE__, scriptLine);
    if (! JavascriptBridge::warnJsError (jsEngine, requireEntryPoint, "Unable to initialize 'require' method within the QJSEngine!")) {
        if (requireEntryPoint.isCallable ()) {
            requireEntryPoint = requireEntryPoint.call (QJSValueList() << this->myself.property ("require"));
            if (! JavascriptBridge::warnJsError (jsEngine, requireEntryPoint, "Unable to retrieve 'require' method implementation from the QJSEngine environment!")) {
                 jsEngine.globalObject().setProperty ("require", requireEntryPoint);
            }
        } else {
            qCritical() << "'require' method initialization procedure did not return a callable object!";
        }
    }
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
        if (! JavascriptBridge::warnJsError (jsEngine, xmlHttpRequestFunction, "Unable to initialize 'XMLHttpRequest' object within the QJSEngine!")) {
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
                if (! JavascriptBridge::warnJsError (jsEngine, xmlHttpRequest, "Unable to retrieve the 'XMLHttpRequest' object constructor from the QJSEngine environment!")) {
                    jsEngine.globalObject().setProperty ("XMLHttpRequest", xmlHttpRequest);
                }
            } else {
                qCritical() << "'XMLHttpRequest' object initialization procedure did not return a callable object!";
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
        if (JavascriptBridge::warnJsError ((*(this->jsEngine)), callReturn, QString("Uncaught exception received while calling method '%1' on channel #%2!").arg(method).arg(this->requestChannel))) {
            this->pendingTransactions.remove (transactionId);
        } else {
            if (! callReturn.isUndefined ()) {
                qWarning() << QString("Value '%1' returned from a call to method '%2' on channel #%3 will be discarded. Please return values by using the following statement: 'arguments[0].call (returnValue);' !").arg(JavascriptBridge::QJS2QString (entryPoint)).arg(method).arg(this->requestChannel);
            }
            return (true);
        }
    } else {
        qWarning() << QString ("An attempt to call a non-function value '%1' within channel #%2 failed!").arg(JavascriptBridge::QJS2QString (entryPoint)).arg(this->requestChannel);
    }
    return (false);
}

QJSValue JavascriptBridge::makeEntryPoint (int index) {
    QString helperName (AppRuntime::helperNames.at(index));
    QJSValue evaluatedFunction = this->jsEngine->evaluate (AppRuntime::helperSourcesByName[helperName], AppConstants::AppHelperSubDir + "/" + helperName + AppConstants::AppHelperExtension, AppRuntime::helperSourcesStartLine);
    if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), evaluatedFunction, QString("A Javascript exception occurred while the helper '%1' was being constructed. It will be disabled!").arg(helperName))) {
        if (evaluatedFunction.isCallable ()) {
            QJSValue entryPoint (evaluatedFunction.call (QJSValueList() << index << this->myself.property("getPropertiesFromObjectCache")));
            if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), entryPoint, QString("A Javascript exception occurred while the helper '%1' was initializing. It will be disabled!").arg(helperName))) {
                return (entryPoint);
            }
        } else {
            qCritical() << QString("The value generated from code evaluation of the helper '%1' is not a function. This is an internal error which will render the helper disabled!").arg(helperName);
        }
    }
    return (QJSValue(QJSValue::NullValue));
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

bool JavascriptBridge::require (const QJSValue& library) {
    if (library.isString ()) {
        QString libraryName (library.toString ());
        QString libraryCode;
        if (this->loadedLibraries.contains (libraryName)) {
            return (true);
        } else {
            {
                QMutexLocker m_lck (&AppRuntime::commonSourcesMutex);
                QHash<QString,QString>::const_iterator librarySource (AppRuntime::commonSources.find(libraryName));
                if (librarySource != AppRuntime::commonSources.constEnd()) {
                    // Also force a deep copy of the library source code
                    // http://doc.qt.io/qt-5/implicit-sharing.html
                    libraryCode = QString("\"use strict\";\n%1").arg(librarySource.value());
                }
            }
            if (libraryCode.isNull ()) {
                qInfo() << QString("Library '%1' requested by a helper on channel #%2 was not found!").arg(libraryName).arg(this->requestChannel);
            } else {
                qDebug() << QString("Loading library '%1' requested by a helper on channel #%2...").arg(libraryName).arg(this->requestChannel);
                if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->jsEngine->evaluate(libraryCode, AppConstants::AppCommonSubDir + "/" + libraryName + AppConstants::AppHelperExtension, 0), QString("Uncaught exception found while library '%1' was being loaded on channel #%2...").arg(libraryName).arg(this->requestChannel))) {
                    this->loadedLibraries.append (libraryName);
                    return (true);
                }
            }
        }
    } else {
        qInfo() << QString("Unable to load a library requested by a helper on channel #%1 because an invalid specification!").arg(this->requestChannel);
    }
    return (false);
}

unsigned int JavascriptBridge::setTimeout (const QJSValue& callback, const int interval) {
    return (this->createTimer (false, callback, interval));
}

unsigned int JavascriptBridge::setInterval (const QJSValue& callback, const int interval) {
    return (this->createTimer (true, callback, interval));
}

void JavascriptBridge::clearTimeout (unsigned int timerId) {
    // This prevent misuse of clearTimeout() and clearInterval() interchangeably
    if (! (timerId & 2)) {
        this->timerFinished (timerId);
    }
}

void JavascriptBridge::clearInterval (unsigned int timerId) {
    // This prevent misuse of clearTimeout() and clearInterval() interchangeably
    if (timerId & 2) {
        this->timerFinished (timerId);
    }
}

void JavascriptBridge::xmlHttpRequest_send (const QJSValue& object, const QJSValue& getPrivateData1, const QJSValue& setPrivateData1, const QJSValue& getPrivateData2, const QJSValue& setPrivateData2) {
    unsigned int networkRequestId = this->networkRequestId;
    this->networkRequestId += 2;
    JavascriptNetworkRequest* networkRequest = new JavascriptNetworkRequest ((*(this->jsEngine)), this);
    this->pendingNetworkRequests.insert (networkRequestId, networkRequest);
    QObject::connect (networkRequest, &JavascriptNetworkRequest::networkRequestFinished, this, &JavascriptBridge::networkRequestFinished);
    networkRequest->setXMLHttpRequestObject (object);
    networkRequest->setPrivateDataCallbacks (getPrivateData1, setPrivateData1, getPrivateData2, setPrivateData2);
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

QString JavascriptBridge::textDecode (const QJSValue& bytes, const QString& fallbackCharset) {
    QString answer;
    QByteArray inputBuffer (JavascriptBridge::ArrayBuffer2QByteArray ((*(this->jsEngine)), bytes));
    QMutexLocker m_lck (&AppRuntime::textCoDecMutex);
    QTextCodec* textCodec = QTextCodec::codecForUtfText (inputBuffer, QTextCodec::codecForName (fallbackCharset.toUtf8()));
    if (textCodec != Q_NULLPTR) {
        QTextDecoder textDecoder (textCodec, QTextCodec::ConvertInvalidToNull);
        answer = textDecoder.toUnicode (inputBuffer);
    }
    return (answer);
}

QJSValue JavascriptBridge::textEncode (const QString& string, const QString& charset) {
    QByteArray answer;
    QMutexLocker m_lck (&AppRuntime::textCoDecMutex);
    QTextCodec* textCodec = QTextCodec::codecForName (charset.toUtf8());
    if (textCodec != Q_NULLPTR) {
        QTextEncoder textEncoder (textCodec, QTextCodec::ConvertInvalidToNull | QTextCodec::IgnoreHeader);
        answer = textEncoder.fromUnicode (string);
    }
    return (JavascriptBridge::QByteArray2ArrayBuffer ((*(this->jsEngine)), answer));
}

void JavascriptBridge::getPropertiesFromObjectCache (unsigned int context, const QJSValue& returnValue) {
    emit valueReturnedFromJavascript (context, "getPropertiesFromObjectCache", returnValue);
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
void JavascriptBridge::console_log (const QJSValue& msg) {
    qDebug() << QString("[QJS] ") + msg.toString();
}

void JavascriptBridge::console_info (const QJSValue& msg) {
    qInfo() << QString("[QJS] ") + msg.toString();
}

void JavascriptBridge::console_warn (const QJSValue& msg) {
    qWarning() << QString("[QJS] ") + msg.toString();
}

void JavascriptBridge::console_error (const QJSValue& msg) {
    qCritical() << QString("[QJS] ") + msg.toString();
}
#endif

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
        qFatal(QString("Unable to handle the received QJsonDocument: '%1'!").arg(QString::fromUtf8(value.toJson(QJsonDocument::Compact))).toLocal8Bit().constData());
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
        QRegExp regExp (JavascriptBridge::RegExp2QRegExp (value));
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

QJSValue JavascriptBridge::QByteArray2ArrayBuffer (QJSEngine& jsEngine, const QByteArray& value) {
    uint length = value.size ();
    QJSValue answer = QVariant(value).value<QJSValue>();
    if (answer.property("byteLength").toUInt() != length) {
        QJSValue uInt8Array (jsEngine.globalObject().property("Uint8Array").callAsConstructor(QJSValueList() << length));
        if (! JavascriptBridge::warnJsError (jsEngine, uInt8Array, "Uncaught exception while creating 'Uint8Array' object!")) {
            if (uInt8Array.isObject ()) {
                for (uint pos = 0; pos < length; pos++) {
                    uInt8Array.setProperty (pos, (unsigned) value[pos]);
                }
                answer = uInt8Array.property("buffer");
            }
        }
    }
    return (answer);
}

QByteArray JavascriptBridge::ArrayBuffer2QByteArray (QJSEngine&, const QJSValue& value) {
    return (value.toVariant().toByteArray());
}

QRegExp JavascriptBridge::RegExp2QRegExp (const QJSValue& jsValue) {
    QRegExp regExp;
    if (jsValue.isRegExp ()) {
        regExp = jsValue.toVariant().toRegExp();
        if (! regExp.isValid ()) {
            QJSValue regExpSource (jsValue.property("source"));
            QJSValue regExpIgnoreCase (jsValue.property("ignoreCase"));
            if (regExpSource.isString() && regExpIgnoreCase.isBool()) {
                regExp = QRegExp (
                    regExpSource.toString(),
                    ((regExpIgnoreCase.toBool()) ? Qt::CaseInsensitive : Qt::CaseSensitive)
                );
            }
        }
    }
    return (regExp);
}

bool JavascriptBridge::warnJsError (QJSEngine& jsEngine, const QJSValue& jsValue, const QString& msg) {
    bool isError = jsValue.isError ();
    // Ugly workaround: https://stackoverflow.com/a/31564520/7184009
    // To this bug: https://bugreports.qt.io/browse/QTBUG-63953
    if (! isError) {
        QJSValue objectObject = jsEngine.globalObject().property("Object");
        QJSValue proto = jsValue.property("constructor").property("prototype");
        while (proto.isObject()) {
            if (! proto.property("constructor").property("name").toString().compare("Error")) {
                isError = true;
                break;
            }
            proto = objectObject.property("getPrototypeOf").call(QJSValueList() << proto);
        }
    }
    if (isError) {
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
