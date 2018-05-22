#include "javascriptbridge.h"

const int JavascriptMethod::getSupportedUrls = 0;
const int JavascriptMethod::getObjectFromUrl = 1;
const int JavascriptMethod::getPropertiesFromObject = 2;
const QStringList JavascriptMethod::requiredMethods (
    QStringList()
        << QStringLiteral("getSupportedUrls")
        << QStringLiteral("getObjectFromUrl")
        << QStringLiteral("getPropertiesFromObject")
);

//////////////////////////////////////////////////////////////////

JavascriptTimer::JavascriptTimer (QJSEngine& _jsEngine, unsigned int _timerId, bool repeat, int timeout, const QJSValue& _callback) :
    QTimer (&_jsEngine),
    timerId (_timerId),
    jsEngine (&_jsEngine),
    callback (_callback) {
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
        qWarning ("JavascriptTimer triggered, but callback value is not a function nor a string.");
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
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), value = this->getPrivateDataCallback1.call (QJSValueList() << key), QByteArray("[XHR#") + QByteArray::number (this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Internal error while getting XMLHttpRequestPrivate." + key.toUtf8() + "!"));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::setPrivateData (const QString& key, const QJSValue& value) {
    if (this->setPrivateDataCallback1.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->setPrivateDataCallback1.call (QJSValueList() << key << value), QByteArray("[XHR#") + QByteArray::number (this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Internal error while setting XMLHttpRequestPrivate." + key.toUtf8() + "!"));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::getPrivateData (const QString& key, const QString& subKey, QJSValue& value) {
    if (this->getPrivateDataCallback2.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), value = this->getPrivateDataCallback2.call (QJSValueList() << key << subKey), QByteArray("[XHR#") + QByteArray::number (this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Internal error while getting XMLHttpRequestPrivate." + key.toUtf8() + "." + subKey.toUtf8() + "!"));
    } else {
        return (false);
    }
}

bool JavascriptNetworkRequest::setPrivateData (const QString& key, const QString& subKey, const QJSValue& value) {
    if (this->setPrivateDataCallback2.isCallable ()) {
        return (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->setPrivateDataCallback2.call (QJSValueList() << key << subKey << value), QByteArray("[XHR#") + QByteArray::number (this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Internal error while setting XMLHttpRequestPrivate." + key.toUtf8() + "." + subKey.toUtf8() + "!"));
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
        this->networkReply->close ();
        this->networkReply->deleteLater ();
        this->networkReply = Q_NULLPTR;
    }
    if (emitNetworkRequestFinished) {
        QJSValue _networkRequestId;
        if (this->getPrivateData (QStringLiteral("requestId"), _networkRequestId)) {
            if (_networkRequestId.toUInt() == this->networkRequestId) {
                this->setPrivateData (QStringLiteral("requestId"), QJSValue(QJSValue::NullValue));
            }
        }
        emit networkRequestFinished (this->networkRequestId);
    }
}

void JavascriptNetworkRequest::fulfillRequest (QNetworkAccessManager& networkManager, bool invokeLoadStart) {
    this->cancelRequest ();
    QJSValue _requestMethod;
    QJSValue _requestUrl;
    QJSValue _requestHeaders;
    QJSValue _requestTimeout;
    QJSValue _requestUsername;
    QJSValue _requestPassword;
    QJSValue _uploadCompleteFlag;
    QJSValue _synchronousFlag;
    QJSValue _requestBuffer;
    this->setPrivateData (QStringLiteral("responseBuffer"), QJSValue(QJSValue::NullValue));
    if (this->getPrivateData (QStringLiteral("requestMethod"), _requestMethod) &&
        this->getPrivateData (QStringLiteral("requestUrl"), _requestUrl) &&
        this->getPrivateData (QStringLiteral("requestHeaders"), _requestHeaders) &&
        this->getPrivateData (QStringLiteral("requestTimeout"), _requestTimeout) &&
        this->getPrivateData (QStringLiteral("requestUsername"), _requestUsername) &&
        this->getPrivateData (QStringLiteral("requestPassword"), _requestPassword) &&
        this->getPrivateData (QStringLiteral("uploadCompleteFlag"), _uploadCompleteFlag) &&
        this->getPrivateData (QStringLiteral("synchronousFlag"), _synchronousFlag) &&
        this->getPrivateData (QStringLiteral("requestBuffer"), _requestBuffer)) {
        QUrl url;
        if (! JavascriptBridge::valueIsEmpty (_requestUrl)) {
            url.setUrl (_requestUrl.toString());
        }
        if (! JavascriptBridge::valueIsEmpty(_requestUsername)) {
            QString username (_requestUsername.toString());
            if (! username.isEmpty()) {
                url.setUserName (username);
            }
        }
        if (! JavascriptBridge::valueIsEmpty(_requestPassword)) {
            QString password (_requestPassword.toString());
            if (! password.isEmpty()) {
                url.setPassword (password);
            }
        }
        QNetworkRequest networkRequest (url);
        QJSValueIterator requestHeader (_requestHeaders);
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
                requestHeaderValueListLength = requestHeaderValueList.property(QStringLiteral("length")).toUInt();
                for (requestHeaderValueListPos = 0; requestHeaderValueListPos < requestHeaderValueListLength; requestHeaderValueListPos++) {
                    requestHeaderValue = requestHeaderValueList.property(requestHeaderValueListPos);
                    if (! JavascriptBridge::valueIsEmpty (requestHeaderValue)) {
                        requestHeaderValues.append (requestHeaderValue.toString());
                    }
                }
                networkRequest.setRawHeader (requestHeaderName.toLocal8Bit(), requestHeaderValues.join(QStringLiteral(",")).toLocal8Bit());
                requestHeaderValues.clear ();
            }
        }
        // Override the User Agent
        networkRequest.setRawHeader ("user-agent", APP_owner_name "." APP_project_name "/" APP_project_version);
        if (! JavascriptBridge::valueIsEmpty (_requestMethod)) {
            this->synchronousFlag = _synchronousFlag.toBool();
            this->requestMethod = _requestMethod.toString();
            if (! this->requestMethod.isEmpty()) {
                this->requestBodyBuffer.setData (JavascriptBridge::ArrayBuffer2QByteArray ((*(this->jsEngine)), _requestBuffer));
                if (! this->requestBodyBuffer.open (QIODevice::ReadOnly)) {
                    qCritical ("[XHR#%u.%u] Internal error while allocating request body buffer: %s", this->networkRequestId, this->maxRedirects, this->requestBodyBuffer.errorString().toLatin1().constData());
                }
                if (! this->responseBodyBuffer.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
                    qCritical ("[XHR#%u.%u] Internal error while allocating response body buffer: %s", this->networkRequestId, this->maxRedirects, this->responseBodyBuffer.errorString().toLatin1().constData());
                }
                bool uploadCompleteFlagBool = _uploadCompleteFlag.toBool();
                qint64 requestBodySize = this->requestBodyBuffer.size();
                if (! this->isGetOrHeadRequest ()) {
                    if ((! uploadCompleteFlagBool) || requestBodySize > 0) {
                        networkRequest.setRawHeader ("content-length", QString::number(requestBodySize).toLocal8Bit());
                    }
                }
                if (invokeLoadStart) {
                    this->fireProgressEvent (false, QStringLiteral("onloadstart"), 0, 0);
                    if (! uploadCompleteFlagBool) {
                        this->fireProgressEvent (true, QStringLiteral("onloadstart"), 0, requestBodySize);
                        QJSValue state, sendFlag;
                        bool fetchedState = (this->getPrivateData (QStringLiteral("state"), state) && this->getPrivateData (QStringLiteral("sendFlag"), sendFlag));
                        if ((! fetchedState) || state.toInt() != JavascriptNetworkRequest::status_OPENED || (! sendFlag.toBool())) {
                            this->cancelRequest (true);
                            return;
                        }
                    }
                }
                int msec = _requestTimeout.toInt ();
                qDebug ("[XHR#%u.%u] Performing a HTTP %s request against '%s'...", this->networkRequestId, this->maxRedirects, this->requestMethod.toLatin1().constData(), url.toDisplayString(QUrl::PrettyDecoded | QUrl::RemoveUserInfo | QUrl::RemoveQuery).toLatin1().constData());
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
        if (callback.startsWith (QStringLiteral("on"))) {
            QJSValue jsCallback;
            if (isUpload) {
                jsCallback = this->xmlHttpRequestObject.property(QStringLiteral("upload")).property(callback);
            } else {
                jsCallback = this->xmlHttpRequestObject.property(callback);
            }
            this->fireProgressEvent (jsCallback, transmitted, length);
        } else {
            qFatal ("Invalid procedure call: 'callback' must start with 'on'!");
        }
    }
}

void JavascriptNetworkRequest::fireProgressEvent (QJSValue& callback, qint64 transmitted, qint64 length) {
    if ((! this->synchronousFlag) && callback.isCallable ()) {
        QJSValue _networkRequestId;
        if (this->getPrivateData (QStringLiteral("requestId"), _networkRequestId)) {
            if (_networkRequestId.toUInt() == this->networkRequestId) {
                QJSValue progressEvent = this->jsEngine->newObject ();
                // I will lose precision here...
                uint total = static_cast<uint> ((length > 0) ? length : 0);
                progressEvent.setProperty (QStringLiteral("loaded"), static_cast<uint> (transmitted));
                progressEvent.setProperty (QStringLiteral("total"), total);
                progressEvent.setProperty (QStringLiteral("lengthComputable"), static_cast<bool> (total));
                JavascriptBridge::warnJsError ((*(this->jsEngine)), callback.callWithInstance (this->xmlHttpRequestObject, QJSValueList() << progressEvent), QByteArray("[XHR#") + QByteArray::number (this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Uncaught exception received while firing a ProgressEvent!");
            }
        }
    }
}

void JavascriptNetworkRequest::fireEvent (const QString& callback) {
    if (! this->synchronousFlag) {
        if (callback.startsWith (QStringLiteral("on"))) {
            QJSValue _networkRequestId;
            if (this->getPrivateData (QStringLiteral("requestId"), _networkRequestId)) {
                if (_networkRequestId.toUInt() == this->networkRequestId) {
                    JavascriptBridge::warnJsError ((*(this->jsEngine)), this->xmlHttpRequestObject.property(callback).callWithInstance (this->xmlHttpRequestObject), QByteArray("[XHR#") + QByteArray::number(this->networkRequestId) + "." + QByteArray::number(this->maxRedirects) + "] Uncaught exception received while firing the '" + callback.toUtf8() + "' event!");
                }
            }
        } else {
            qFatal ("Invalid procedure call: 'callback' must start with 'on'!");
        }
    }
}

void JavascriptNetworkRequest::networkReplyUploadProgress (qint64 bytesSent, qint64 bytesTotal) {
    if (this->httpResponseOk ()) {
        this->fireProgressEvent (true, QStringLiteral("onprogress"), bytesSent, bytesTotal);
        if (bytesTotal >= 0 && bytesSent == bytesTotal) {
            this->setPrivateData (QStringLiteral("uploadCompleteFlag"), true);
            this->fireProgressEvent (true, QStringLiteral("onload"), bytesSent, bytesTotal);
            this->fireProgressEvent (true, QStringLiteral("onloadend"), bytesSent, bytesTotal);
        }
    }
}

void JavascriptNetworkRequest::networkReplyDownloadProgress (qint64 bytesReceived, qint64 bytesTotal) {
    if (! this->responseStatus) {
        this->responseStatus = this->networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
    if (this->httpResponseOk () && this->isFinalAnswer ()) {
        if (! this->downloadStarted) {
            this->downloadStarted = true;
            QJSValue state (JavascriptNetworkRequest::status_HEADERS_RECEIVED);
            this->setPrivateData (QStringLiteral("state"), state);
            this->setPrivateData (QStringLiteral("status"), this->responseStatus);
            this->setPrivateData (QStringLiteral("statusText"), this->networkReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
            this->setPrivateData (QStringLiteral("responseURL"), this->networkReply->url().toString());
            QJSValue responseHeadersObject = this->jsEngine->newObject ();
            QList<QNetworkReply::RawHeaderPair> responseHeaders (this->networkReply->rawHeaderPairs ());
            for (QList<QNetworkReply::RawHeaderPair>::const_iterator headerPair = responseHeaders.constBegin(); headerPair != responseHeaders.constEnd(); headerPair++) {
                QString headerName (QString::fromUtf8(headerPair->first).toLower());
                QString headerValue (QString::fromUtf8(headerPair->second));
                if (! responseHeadersObject.hasProperty (headerName)) {
                    responseHeadersObject.setProperty (headerName, this->jsEngine->newArray ());
                }
                responseHeadersObject.property(headerName).setProperty (responseHeadersObject.property(headerName).property(QStringLiteral("length")).toUInt(), headerValue);
            }
            this->setPrivateData (QStringLiteral("responseHeaders"), responseHeadersObject);
            this->fireEvent (QStringLiteral("onreadystatechange"));
            if (this->getPrivateData (QStringLiteral("state"), state)) {
                if (state.toInt() != JavascriptNetworkRequest::status_HEADERS_RECEIVED) {
                    this->cancelRequest (true);
                    return;
                }
            }
            if (bytesReceived > 0 || bytesTotal > 0) {
                this->setPrivateData (QStringLiteral("state"), JavascriptNetworkRequest::status_LOADING);
            }
        }
        this->responseBodyBuffer.write (this->networkReply->readAll ());
        if (bytesReceived > 0 || bytesTotal > 0) {
            this->fireEvent (QStringLiteral("onreadystatechange"));
            this->fireProgressEvent (false, QStringLiteral("onprogress"), bytesReceived, bytesTotal);
        }
        if (bytesReceived == bytesTotal) {
            this->setPrivateData (QStringLiteral("responseBuffer"), JavascriptBridge::QByteArray2ArrayBuffer ((*(this->jsEngine)), this->responseBuffer ()));
            this->setPrivateData (QStringLiteral("state"), JavascriptNetworkRequest::status_DONE);
            this->setPrivateData (QStringLiteral("sendFlag"), false);
            this->setPrivateData (QStringLiteral("responseObject"), QStringLiteral("type"), QStringLiteral("object")); // The XMLHttpRequest specification did not rule this...
            this->fireEvent (QStringLiteral("onreadystatechange"));
            this->fireProgressEvent (false, QStringLiteral("onload"), bytesReceived, bytesTotal);
            this->fireProgressEvent (false, QStringLiteral("onloadend"), bytesReceived, bytesTotal);
        }
    }
}

void JavascriptNetworkRequest::networkReplyFinished () {
    this->requestBodyBuffer.close ();
    bool hasHttpError = (! this->httpResponseOk ());
    if (this->isFinalAnswer() || hasHttpError) {
        if (hasHttpError) {
            this->responseBodyBuffer.write (this->networkReply->readAll ());
            this->setPrivateData (QStringLiteral("responseBuffer"), JavascriptBridge::QByteArray2ArrayBuffer ((*(this->jsEngine)), this->responseBuffer ()));
        }
        this->responseBodyBuffer.close ();
        if (hasHttpError) {
            QNetworkReply::NetworkError networkError = this->networkReply->error ();
            qInfo ("[XHR#%u.%u] Network request finished unexpectedly: %d: %s", this->networkRequestId, this->maxRedirects, networkError, this->networkReply->errorString().toLatin1().constData());
            QJSValue sendFlag;
            if (this->getPrivateData (QStringLiteral("sendFlag"), sendFlag)) {
                if (! sendFlag.toBool ()) {
                    this->cancelRequest (true);
                    return;
                }
            }
            this->setPrivateData (QStringLiteral("state"), JavascriptNetworkRequest::status_DONE);
            this->setPrivateData (QStringLiteral("sendFlag"), false);
            this->setPrivateData (QStringLiteral("responseObject"), QStringLiteral("type"), QStringLiteral("failure"));
            if (networkError == QNetworkReply::RemoteHostClosedError && this->downloadStarted) {
                if (this->responseStatus >= 200 && this->responseStatus < 300) {
                    this->setPrivateData (QStringLiteral("status"), 0);
                    this->setPrivateData (QStringLiteral("statusText"), QStringLiteral("NetworkError: %1").arg(this->networkReply->errorString ()));
                }
                this->setPrivateData (QStringLiteral("responseObject"), QStringLiteral("value"), QStringLiteral("NetworkError"));
            } else {
                QString exception (QStringLiteral("NetworkError"));
                QString eventHandler (QStringLiteral("onerror"));
                if (networkError == QNetworkReply::OperationCanceledError) {
                    // Either the request did not finish within the configured timeout or XMLHttpRequest.prototype.abort() was called
                    // QNetworkReply::abort() is called in both cases.
                    QJSValue timedOutFlag, abortedFlag;
                    if (this->getPrivateData (QStringLiteral("timedOutFlag"), timedOutFlag) && this->getPrivateData (QStringLiteral("abortedFlag"), abortedFlag)) {
                        if (timedOutFlag.toBool()) {
                            exception = QStringLiteral("TimeoutError");
                            eventHandler = QStringLiteral("ontimeout");
                        } else if (abortedFlag.toBool()) {
                            exception = QStringLiteral("AbortError");
                            eventHandler = QStringLiteral("onabort");
                        }
                    }
                }
                QString errorString (QStringLiteral("%1: %2").arg(exception, this->networkReply->errorString ()));
                this->setPrivateData (QStringLiteral("status"), 0);
                this->setPrivateData (QStringLiteral("statusText"), errorString);
                this->setPrivateData (QStringLiteral("responseObject"), QStringLiteral("value"), errorString);
                this->fireEvent (QStringLiteral("onreadystatechange"));
                QJSValue uploadCompleteFlag;
                if (this->getPrivateData (QStringLiteral("uploadCompleteFlag"), uploadCompleteFlag)) {
                    if (! uploadCompleteFlag.toBool()) {
                        this->setPrivateData (QStringLiteral("uploadCompleteFlag"), true);
                        this->fireProgressEvent (true, eventHandler, 0, 0);
                        this->fireProgressEvent (true, QStringLiteral("onloadend"), 0, 0);
                    }
                }
                this->fireProgressEvent (false, eventHandler, 0, 0);
                this->fireProgressEvent (false, QStringLiteral("onloadend"), 0, 0);
            }
        }
        this->cancelRequest (true);
    } else {
        this->responseBodyBuffer.close ();
        // Handle the URL redirect
        if (this->isRedirectWithMethodChange()) {
            if (! this->isGetOrHeadRequest ()) {
                this->setPrivateData (QStringLiteral("requestMethod"), QStringLiteral("GET"));
            }
            this->setPrivateData (QStringLiteral("requestBuffer"), QJSValue(QJSValue::NullValue));
            this->setPrivateData (QStringLiteral("requestUsername"), QJSValue(QJSValue::NullValue));
            this->setPrivateData (QStringLiteral("requestPassword"), QJSValue(QJSValue::NullValue));
        }
        QJSValue requestUrl;
        if (this->getPrivateData (QStringLiteral("requestUrl"), requestUrl)) {
            QJSValue requestHeaderReferer (this->jsEngine->newArray (1));
            requestHeaderReferer.setProperty (0, requestUrl);
            this->setPrivateData (QStringLiteral("requestHeaders"), QStringLiteral("referer"), requestHeaderReferer);
        }
        QString location (QString::fromUtf8 (this->networkReply->rawHeader ("location")));
        if (location.isEmpty()) {
            qInfo ("[XHR#%u.%u] Server replied HTTP status '%d %s' and did not send a 'Location:' header. I will fetch the same URL again!", this->networkRequestId, this->maxRedirects, this->responseStatus, this->networkReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray().constData());
        } else {
            this->setPrivateData (QStringLiteral("requestUrl"), this->networkReply->url().resolved(location).toString());
        }
        this->maxRedirects--;
        this->fulfillRequest ((*(this->networkReply->manager ())), false);
    }
}

void JavascriptNetworkRequest::timeoutTimerTimeout () {
    this->setPrivateData (QStringLiteral("timedOutFlag"), true);
}

JavascriptNetworkRequest::JavascriptNetworkRequest (QJSEngine& _jsEngine, QObject *_parent) :
    QObject (_parent),
    jsEngine (&_jsEngine),
    networkReply (Q_NULLPTR) {
    if (this->jsEngine == Q_NULLPTR) {
        qFatal ("JavascriptBridge object must be bound to a QJSEngine! Invoke 'QJSEngine::newQObject();'!");
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

void JavascriptNetworkRequest::start (QNetworkAccessManager& networkManager, unsigned int _networkRequestId) {
    this->networkRequestId = _networkRequestId;
    this->maxRedirects = 20;
    this->responseStatus = 0;
    this->downloadStarted = false;
    if (this->setPrivateData (QStringLiteral("requestId"), _networkRequestId)) {
        this->fulfillRequest (networkManager, true);
    } else {
        this->cancelRequest (true);
    }
}

void JavascriptNetworkRequest::abort () {
    this->setPrivateData (QStringLiteral("abortedFlag"), true);
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
        unsigned int length = value.property(QStringLiteral("length")).toUInt ();
        for (unsigned int i = 0; i < length; i++) {
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
        QJSValue jsValue (jsEngine.newArray (static_cast<unsigned> (jsonArray.count())));
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

JavascriptBridge::JavascriptBridge (QJSEngine& _jsEngine, const QString& _requestChannel, QObject* parent) :
    QObject (parent),
    jsEngine (&_jsEngine),
    myself (_jsEngine.newQObject (this)),
    requestChannel (_requestChannel),
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
    QString requireCode (QStringLiteral (
        "(function (callback) {\n"
        "    return (function (library) {\n"
        "        if (! callback (library)) {\n"
        "            throw new ReferenceError (\"Unable to load library '\" + library + \" into the runtime environment!\");\n"
        "        }\n"
        "    });\n"
        "});\n"
    ));
    QJSValue requireEntryPoint = _jsEngine.evaluate (requireCode, QStringLiteral(__FILE__), scriptLine);
    if (! JavascriptBridge::warnJsError (_jsEngine, requireEntryPoint, "Unable to initialize 'require' method within the QJSEngine!")) {
        if (requireEntryPoint.isCallable ()) {
            requireEntryPoint = requireEntryPoint.call (QJSValueList() << this->myself.property (QStringLiteral("require")));
            if (! JavascriptBridge::warnJsError (_jsEngine, requireEntryPoint, "Unable to retrieve 'require' method implementation from the QJSEngine environment!")) {
                 _jsEngine.globalObject().setProperty (QStringLiteral("require"), requireEntryPoint);
            }
        } else {
            qFatal ("'require' method initialization procedure did not return a callable object!");
        }
    }
    // "set/clear Timeout/Interval" functions implementation
    _jsEngine.globalObject().setProperty (QStringLiteral("setTimeout"), this->myself.property (QStringLiteral("setTimeout")));
    _jsEngine.globalObject().setProperty (QStringLiteral("setInterval"), this->myself.property (QStringLiteral("setInterval")));
    _jsEngine.globalObject().setProperty (QStringLiteral("clearTimeout"), this->myself.property (QStringLiteral("clearTimeout")));
    _jsEngine.globalObject().setProperty (QStringLiteral("clearInterval"), this->myself.property (QStringLiteral("clearInterval")));
    // Console object implementation
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    _jsEngine.installExtensions (QJSEngine::ConsoleExtension);
#else
    QJSValue consoleObj = _jsEngine.newObject ();
    consoleObj.setProperty (QStringLiteral("debug"), this->myself.property (QStringLiteral("console_log")));
    consoleObj.setProperty (QStringLiteral("log"), this->myself.property (QStringLiteral("console_log")));
    consoleObj.setProperty (QStringLiteral("error"), this->myself.property (QStringLiteral("console_error")));
    consoleObj.setProperty (QStringLiteral("exception"), this->myself.property (QStringLiteral("console_error")));
    consoleObj.setProperty (QStringLiteral("info"), this->myself.property (QStringLiteral("console_info")));
    consoleObj.setProperty (QStringLiteral("warn"), this->myself.property (QStringLiteral("console_warn")));
    _jsEngine.globalObject().setProperty (QStringLiteral("console"), consoleObj);
 #endif
    // Interface for data conversion to and from Unicode
    // My XMLHttpRequest implementation need it
    _jsEngine.globalObject().setProperty (QStringLiteral("textDecode"), this->myself.property (QStringLiteral("textDecode")));
    _jsEngine.globalObject().setProperty (QStringLiteral("textEncode"), this->myself.property (QStringLiteral("textEncode")));
    // XMLHttpRequest implementation
    QString xmlHttpCode = AppRuntime::readFileContents (QStringLiteral(":/xmlhttprequest.js"));
    if (xmlHttpCode.isNull ()) {
        qFatal ("Unable to read resource file ':/xmlhttprequest.js'!");
    } else {
        QJSValue xmlHttpRequestFunction = _jsEngine.evaluate (xmlHttpCode, QStringLiteral(":/xmlhttprequest.js"));
        if (! JavascriptBridge::warnJsError (_jsEngine, xmlHttpRequestFunction, "Unable to initialize 'XMLHttpRequest' object within the QJSEngine!")) {
            if (xmlHttpRequestFunction.isCallable ()) {
                // Make sure C++ and Javascript agree with the XMLHttpRequest status codes.
                QJSValue xmlHttpRequest_statusCodes = _jsEngine.newObject ();
                xmlHttpRequest_statusCodes.setProperty (QStringLiteral("UNSENT"), JavascriptNetworkRequest::status_UNSENT);
                xmlHttpRequest_statusCodes.setProperty (QStringLiteral("OPENED"), JavascriptNetworkRequest::status_OPENED);
                xmlHttpRequest_statusCodes.setProperty (QStringLiteral("HEADERS_RECEIVED"), JavascriptNetworkRequest::status_HEADERS_RECEIVED);
                xmlHttpRequest_statusCodes.setProperty (QStringLiteral("LOADING"), JavascriptNetworkRequest::status_LOADING);
                xmlHttpRequest_statusCodes.setProperty (QStringLiteral("DONE"), JavascriptNetworkRequest::status_DONE);
                QJSValue xmlHttpRequest = xmlHttpRequestFunction.call (QJSValueList()
                    << this->myself.property (QStringLiteral("xmlHttpRequest_send"))
                    << this->myself.property (QStringLiteral("xmlHttpRequest_abort"))
                    << this->myself.property (QStringLiteral("xmlHttpRequest_setTimeout"))
                    << this->myself.property (QStringLiteral("xmlHttpRequest_getResponseBuffer"))
                    << xmlHttpRequest_statusCodes
                );
                if (! JavascriptBridge::warnJsError (_jsEngine, xmlHttpRequest, "Unable to retrieve the 'XMLHttpRequest' object constructor from the QJSEngine environment!")) {
                    _jsEngine.globalObject().setProperty (QStringLiteral("XMLHttpRequest"), xmlHttpRequest);
                }
            } else {
                qFatal ("'XMLHttpRequest' object initialization procedure did not return a callable object!");
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
        unsigned int _transactionId (this->transactionId);
        this->transactionId += 2;
        this->pendingTransactions[_transactionId] = context;
        QJSValue callReturn (entryPoint.call (QJSValueList() << this->myself.property(QStringLiteral("receiveValue")) << _transactionId << method << args));
        if (JavascriptBridge::warnJsError ((*(this->jsEngine)), callReturn, QByteArray("Uncaught exception received while calling method '") + method.toLatin1() + "' on channel #" + this->requestChannel.toLatin1() + "!")) {
            this->pendingTransactions.remove (_transactionId);
        } else {
            if (! callReturn.isUndefined ()) {
                qWarning ("Value '%s' returned from a call to method '%s' on channel #%s will be discarded. Please return values by using the following statement: 'arguments[0].call (returnValue);' !", JavascriptBridge::QJS2QString(entryPoint).toLatin1().constData(), method.toLatin1().constData(), this->requestChannel.toLatin1().constData());
            }
            return (true);
        }
    } else {
        qWarning ("An attempt to call a non-function value '%s' within channel #%s failed!", JavascriptBridge::QJS2QString(entryPoint).toLatin1().constData(), this->requestChannel.toLatin1().constData());
    }
    return (false);
}

QJSValue JavascriptBridge::makeEntryPoint (int index) {
    QString helperName (AppRuntime::helperNames.at(index));
    QJSValue evaluatedFunction = this->jsEngine->evaluate (AppRuntime::helperSourcesByName[helperName], AppConstants::AppHelperSubDir + QStringLiteral("/") + helperName + AppConstants::AppHelperExtension, AppRuntime::helperSourcesStartLine);
    if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), evaluatedFunction, QByteArray("A Javascript exception occurred while the helper '") + helperName.toLatin1() + "' was being constructed. It will be disabled!")) {
        if (evaluatedFunction.isCallable ()) {
            QJSValue entryPoint (evaluatedFunction.call (QJSValueList() << index << this->myself.property(QStringLiteral("getPropertiesFromObjectCache"))));
            if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), entryPoint, QByteArray("A Javascript exception occurred while the helper '") + helperName.toLatin1() + "' was initializing. It will be disabled!")) {
                return (entryPoint);
            }
        } else {
            qFatal ("The value generated from code evaluation of the helper '%s' is not a function!", helperName.toLatin1().constData());
        }
    }
    return (QJSValue(QJSValue::NullValue));
}

void JavascriptBridge::receiveValue (unsigned int _transactionId, const QString& method, const QJSValue& returnedValue) {
    QMap<unsigned int, unsigned int>::iterator transactionIdIterator = this->pendingTransactions.find (_transactionId);
    if (transactionIdIterator != this->pendingTransactions.end()) {
        unsigned int context = (*transactionIdIterator);
        this->pendingTransactions.erase (transactionIdIterator);
        emit valueReturnedFromJavascript (context, method, returnedValue);
    } else {
        qWarning ("Unexpected data '%s' received by a invocation of method '%s' on channel #%s! It will be discarded.", JavascriptBridge::QJS2QString(returnedValue).toLatin1().constData(), method.toLatin1().constData(), this->requestChannel.toLatin1().constData());
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
                    libraryCode = QStringLiteral("\"use strict\";\n%1").arg(librarySource.value());
                }
            }
            if (libraryCode.isNull ()) {
                qInfo ("Library '%s' requested by a helper on channel #%s was not found!", libraryName.toLatin1().constData(), this->requestChannel.toLatin1().constData());
            } else {
                qDebug ("Loading library '%s' requested by a helper on channel #%s...", libraryName.toLatin1().constData(), this->requestChannel.toLatin1().constData());
                if (! JavascriptBridge::warnJsError ((*(this->jsEngine)), this->jsEngine->evaluate(libraryCode, AppConstants::AppCommonSubDir + QStringLiteral("/") + libraryName + AppConstants::AppHelperExtension, 0), QByteArray("Uncaught exception found while library '") + libraryName.toLatin1() + "' was being loaded on channel #" + this->requestChannel.toLatin1() + "...")) {
                    this->loadedLibraries.append (libraryName);
                    return (true);
                }
            }
        }
    } else {
        qInfo ("Unable to load a library requested by a helper on channel #%s because of an invalid specification!", this->requestChannel.toLatin1().constData());
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
    unsigned int _networkRequestId = this->networkRequestId;
    this->networkRequestId += 2;
    JavascriptNetworkRequest* networkRequest = new JavascriptNetworkRequest ((*(this->jsEngine)), this);
    this->pendingNetworkRequests.insert (_networkRequestId, networkRequest);
    QObject::connect (networkRequest, &JavascriptNetworkRequest::networkRequestFinished, this, &JavascriptBridge::networkRequestFinished);
    networkRequest->setXMLHttpRequestObject (object);
    networkRequest->setPrivateDataCallbacks (getPrivateData1, setPrivateData1, getPrivateData2, setPrivateData2);
    networkRequest->start (*(this->networkManager), _networkRequestId);
}

void JavascriptBridge::xmlHttpRequest_abort (const unsigned int _networkRequestId) {
    QMap<unsigned int, JavascriptNetworkRequest*>::iterator networkRequest = this->pendingNetworkRequests.find (_networkRequestId);
    if (networkRequest != this->pendingNetworkRequests.end ()) {
        (*networkRequest)->abort ();
    }
}

void JavascriptBridge::xmlHttpRequest_setTimeout (const unsigned int _networkRequestId, const int msec) {
    QMap<unsigned int, JavascriptNetworkRequest*>::iterator networkRequest = this->pendingNetworkRequests.find (_networkRequestId);
    if (networkRequest != this->pendingNetworkRequests.end ()) {
        (*networkRequest)->setTimerInterval (msec);
    }
}

QJSValue JavascriptBridge::xmlHttpRequest_getResponseBuffer (const unsigned int _networkRequestId) {
    QMap<unsigned int, JavascriptNetworkRequest*>::iterator networkRequest = this->pendingNetworkRequests.find (_networkRequestId);
    if (networkRequest != this->pendingNetworkRequests.end ()) {
        return (JavascriptBridge::QByteArray2ArrayBuffer ((*(this->jsEngine)), (*networkRequest)->responseBuffer ()));
    } else {
        qCritical ("Invalid procedure call: unknown networkRequestId #%u!", _networkRequestId);
        return (QJSValue::NullValue);
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
    emit valueReturnedFromJavascript (context, QStringLiteral("getPropertiesFromObjectCache"), returnValue);
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 6, 0))
void JavascriptBridge::console_log (const QJSValue& msg) {
    qDebug ("[QJS] %s", msg.toString().toLatin1().constData());
}

void JavascriptBridge::console_info (const QJSValue& msg) {
    qInfo ("[QJS] %s", msg.toString().toLatin1().constData());
}

void JavascriptBridge::console_warn (const QJSValue& msg) {
    qWarning ("[QJS] %s", msg.toString().toLatin1().constData());
}

void JavascriptBridge::console_error (const QJSValue& msg) {
    qCritical ("[QJS] %s", msg.toString().toLatin1().constData());
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
        qFatal ("Unable to handle the received QJsonDocument: '%s'!", value.toJson(QJsonDocument::Compact).constData());
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
            return (QStringLiteral("true"));
        } else {
            return (QStringLiteral("false"));
        }
    } else if (value.isCallable ()) {
        return (QStringLiteral("function () { /* Native code */ }"));
    } else if (value.isDate ()) {
        // https://bugreports.qt.io/browse/QTBUG-59235
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        return (value.toDateTime().toString(Qt::ISODateWithMs));
#else
        return (value.toDateTime().toString(Qt::ISODate));
#endif
    } else if (value.isNull ()) {
        return (QStringLiteral("null"));
    } else if (value.isNumber ()) {
        return (QString::number (value.toNumber()));
    } else if (value.isRegExp ()) {
        QRegExp regExp (JavascriptBridge::RegExp2QRegExp (value));
        return (QStringLiteral("QRegExp: (CaseSensitivity=%1, PatternSyntax=%2, Pattern='%3')").arg(QString::number (regExp.caseSensitivity()), QString::number (regExp.patternSyntax()), regExp.pattern()));
    } else if (value.isString ()) {
        return (value.toString());
    } else if (value.isUndefined ()) {
        return (QStringLiteral("undefined"));
    } else {
        QJsonDocument jsonDocument (JavascriptBridge::QJS2QJsonDocument(value));
        if (jsonDocument.isNull ()) {
            return (QStringLiteral("(conversion error)"));
        } else {
            return (QString::fromUtf8(jsonDocument.toJson(QJsonDocument::Compact)));
        }
    }
}

QJSValue JavascriptBridge::QByteArray2ArrayBuffer (QJSEngine& jsEngine, const QByteArray& value) {
    uint length = static_cast <uint> (value.size ());
    if (length) {
        QJSValue answer = jsEngine.toScriptValue<QByteArray> (value);
        if (answer.property(QStringLiteral("byteLength")).toUInt() != length) {
            QJSValue uInt8Array (jsEngine.globalObject().property(QStringLiteral("Uint8Array")).callAsConstructor(QJSValueList() << length));
            if (! JavascriptBridge::warnJsError (jsEngine, uInt8Array, "Uncaught exception while creating 'Uint8Array' object!")) {
                if (uInt8Array.isObject ()) {
                    for (uint pos = 0; pos < length; pos++) {
                        uInt8Array.setProperty (pos, static_cast<unsigned> (value[pos]));
                    }
                    answer = uInt8Array.property(QStringLiteral("buffer"));
                }
            }
        }
        return (answer);
    } else {
        return (jsEngine.globalObject().property(QStringLiteral("ArrayBuffer")).callAsConstructor (QJSValueList() << 0));
    }
}

QByteArray JavascriptBridge::ArrayBuffer2QByteArray (QJSEngine&, const QJSValue& value) {
    return (value.toVariant().toByteArray());
}

QRegExp JavascriptBridge::RegExp2QRegExp (const QJSValue& jsValue) {
    QRegExp regExp;
    if (jsValue.isRegExp ()) {
        regExp = jsValue.toVariant().toRegExp();
        if (! regExp.isValid ()) {
            QJSValue regExpSource (jsValue.property(QStringLiteral("source")));
            QJSValue regExpIgnoreCase (jsValue.property(QStringLiteral("ignoreCase")));
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

bool JavascriptBridge::warnJsError (QJSEngine& jsEngine, const QJSValue& jsValue, const QByteArray& msg) {
    bool isError = jsValue.isError ();
    // Ugly workaround: https://stackoverflow.com/a/31564520/7184009
    // To this bug: https://bugreports.qt.io/browse/QTBUG-63953
    if (! isError) {
        QJSValue objectObject = jsEngine.globalObject().property(QStringLiteral("Object"));
        QJSValue proto = jsValue.property(QStringLiteral("constructor")).property(QStringLiteral("prototype"));
        while (proto.isObject()) {
            if (! proto.property(QStringLiteral("constructor")).property(QStringLiteral("name")).toString().compare(QStringLiteral("Error"))) {
                isError = true;
                break;
            }
            proto = objectObject.property(QStringLiteral("getPrototypeOf")).call(QJSValueList() << proto);
        }
    }
    if (isError) {
        qInfo ("Javascript exception at %s:%d: %s: %s",
            jsValue.property(QStringLiteral("fileName")).toString().toLatin1().constData(),
            jsValue.property(QStringLiteral("lineNumber")).toInt(),
            jsValue.property(QStringLiteral("name")).toString().toLatin1().constData(),
            jsValue.property(QStringLiteral("message")).toString().toLatin1().constData());
        qDebug ("%s", jsValue.property(QStringLiteral("stack")).toString().toLatin1().constData());
        if (! msg.isEmpty ()) {
            qWarning ("%s", msg.constData());
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

void JavascriptBridge::networkRequestFinished (unsigned int _networkRequestId) {
    QMap<unsigned int,JavascriptNetworkRequest*>::iterator networkRequestIterator = this->pendingNetworkRequests.find (_networkRequestId);
    if (networkRequestIterator != this->pendingNetworkRequests.end()) {
        JavascriptNetworkRequest* oldNetworkRequest = (*networkRequestIterator);
        this->pendingNetworkRequests.erase (networkRequestIterator);
        oldNetworkRequest->deleteLater ();
        qDebug ("XMLHttpRequest #%u has finished.", _networkRequestId);
    }
}
