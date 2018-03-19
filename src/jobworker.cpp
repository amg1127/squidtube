#include "jobworker.h"

void JobWorker::squidResponseOut (const int requestId, const QString& msg, bool isError, bool isMatch) {
    AppSquidRequest squidRequest (this->runningRequests.take (requestId));
    if (squidRequest.helperName.isEmpty ()) {
        emit writeAnswerLine (this->requestChannel, msg, isError, isMatch);
    } else {
        emit writeAnswerLine (this->requestChannel, QString("[") + squidRequest.helperName + "] " + msg, isError, isMatch);
    }
    if (this->finishRequested && this->runningRequests.isEmpty () && this->incomingRequests.isEmpty ()) {
        qInfo() << QString("No pending jobs now. Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    }
}

void JobWorker::processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls) {
    if (helperInstance > 0 && helperInstance <= this->helperInstances.count() && appHelperSupportedUrls.isArray()) {
        AppHelperInfo* appHelperInfo (this->helperInstances[helperInstance - 1]);
        int length = appHelperSupportedUrls.property("length").toInt();
        if (length > 1) {
            qDebug() << QString("[%1] Received %2 patterns of supported URL's.").arg(appHelperInfo->name).arg(length);
        } else if (length == 1) {
            qDebug() << QString("[%1] Received a pattern of supported URL's.").arg(appHelperInfo->name);
        } else {
            qWarning() << QString("[%1] Helper declares no support for any URL pattern!").arg(appHelperInfo->name);
        }
        appHelperInfo->supportedURLs.clear ();
        for (int i = 0; i < length; i++) {
            QJSValue appHelperSupportedUrl (appHelperSupportedUrls.property (i));
            QRegExp regExpSupportedUrl;
            if (appHelperSupportedUrl.isString ()) {
                regExpSupportedUrl = QRegExp (appHelperSupportedUrl.toString(), Qt::CaseInsensitive, QRegExp::WildcardUnix);
            } else if (appHelperSupportedUrl.isRegExp ()) {
                regExpSupportedUrl = appHelperSupportedUrl.toVariant().toRegExp();
            }
            if (regExpSupportedUrl.isValid ()) {
                if (regExpSupportedUrl.isEmpty ()) {
                    qWarning() << QString("[%1] Discarding the empty regular expression #%2...").arg(appHelperInfo->name).arg(i);
                } else {
                    appHelperInfo->supportedURLs.append (regExpSupportedUrl);
                }
            } else {
                qWarning() << QString("[%1] Invalid item returned: '%2'").arg(appHelperInfo->name).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrl));
            }
        }
    } else {
        qWarning() << QString("Invalid returned data: count=%1 , instance=%2 , data='%3'").arg(this->helperInstances.count()).arg(helperInstance).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrls));
    }
}

void JobWorker::processObjectFromUrl (int requestId, const QJSValue& appHelperObjectFromUrl) {
    if (this->runningRequests.contains (requestId)) {
        AppSquidRequest squidRequest (this->runningRequests[requestId]);
        QString objectClassName, objectId;
        if (appHelperObjectFromUrl.isObject ()) {
            objectClassName = appHelperObjectFromUrl.property("className").toString ();
            objectId = appHelperObjectFromUrl.property("id").toString ();
        } else if (appHelperObjectFromUrl.isString ()) {
            QJsonDocument jsonHelperObjectFromUrl (QJsonDocument::fromJson (appHelperObjectFromUrl.toString().toUtf8()));
            objectClassName = jsonHelperObjectFromUrl.object().value("className").toString();
            objectId = jsonHelperObjectFromUrl.object().value("id").toString();
        }
        if (objectClassName.isEmpty() || objectId.isEmpty ()) {
            this->squidResponseOut (requestId, "Unable to determine an object that matches the supplied URL.", false, false);
        } else {
            for (QList<AppHelperInfo*>::const_iterator appHelperInfo = this->helperInstances.constBegin(); appHelperInfo != this->helperInstances.constEnd(); appHelperInfo++) {
                if ((*appHelperInfo)->name == squidRequest.helperName) {
                    qDebug() << QString("[%1] Searching for information concerning 'className=%2, id=%3'...").arg(squidRequest.helperName).arg(objectClassName).arg(objectId);
                    QJsonDocument objectData;
                    qint64 objectTimestamp;
                    CacheStatus cacheStatus = (*appHelperInfo)->memoryCache->read (objectClassName, objectId, squidRequest.timestampNow - AppRuntime::registryTTLint, objectData, objectTimestamp);
                    if (cacheStatus == CacheStatus::CacheHit) {
                        qDebug() << QString("[%1] Information retrieved from the cache concerning 'className=%2, id=%3' is fresh. Now the matching test begins.").arg(squidRequest.helperName).arg(objectClassName).arg(objectId);
                        this->processCriteria (requestId, objectData);
                        return;
                    } else if (cacheStatus == CacheStatus::CacheOnProgress) {
                        if (objectTimestamp >= (this->currentTimestamp - AppConstants::AppHelperMaxWait)) {
                            qDebug() << QString("[%1] Another thread or process is currently fetching information concerning 'className=%2, id=%3'. I will try to wait for it...").arg(squidRequest.helperName).arg(objectClassName).arg(objectId);
                            this->incomingRequests.prepend (squidRequest);
                            this->runningRequests.remove (requestId);
                            if (this->rngdInitialized) {
                                this->retryTimer->start (AppConstants::AppHelperTimerTimeout + ((int) ((((double) qrand()) / ((double) RAND_MAX)) * ((double) AppConstants::AppHelperTimerTimeout))));
                            } else {
                                this->retryTimer->start (AppConstants::AppHelperTimerTimeout);
                            }
                            return;
                        }
                    }
                    /*
                     * If multiple threads or processes are waiting for an answer and the worker which took that job times out,
                     * a race condition begins. It will not crash the program, fortunately. However, the race condition will
                     * produce concurrent calls to helper's 'getPropertiesFromObject ();' function, so duplicated API calls will
                     * be issued.
                     *
                     * Such race condition may be triggered by a network outage. In this scenario, the helper returns error
                     * codes to the program and the race ends.
                     */
                    qDebug() << QString("[%1] Information concerning 'className=%2, id=%3' was not found in the cache. Invoking 'getPropertiesFromObject ();' - RequestID #%4").arg(squidRequest.helperName).arg(objectClassName).arg(objectId).arg(requestId);
                    QJSValue params = this->runtimeEnvironment->newArray (2);
                    params.setProperty (0, objectClassName);
                    params.setProperty (1, objectId);
                    (*appHelperInfo)->memoryCache->write (objectClassName, objectId, QJsonDocument::fromJson("{}"), this->currentTimestamp);
                    if (! this->javascriptBridge->invokeMethod ((*appHelperInfo)->entryPoint, requestId, JavascriptMethod::getPropertiesFromObject, params)) {
                        this->squidResponseOut (requestId, "'getPropertiesFromObject ();' function returned an error!", true, false);
                    }
                    return;
                }
            }
            qFatal("A helper name shall not disappear from the program memory! This is really bad...");
        }
    } else {
        qWarning() << QString("Invalid returned data: requestId=%1 , data='%2'").arg(requestId).arg(JavascriptBridge::QJS2QString(appHelperObjectFromUrl));
    }
}

void JobWorker::processCriteria (int requestId, const QJsonDocument& data) {
#warning I believe that I will need to create a recursive function.
#warning I do not believe the signature will be like this...
}

JobWorker::JobWorker (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    finishRequested (false),
    rngdInitialized (false),
    requestChannel (requestChannel),
    runtimeEnvironment (new QQmlEngine (this)),
    javascriptBridge (new JavascriptBridge ((*runtimeEnvironment), requestChannel)),
    requestId (1),
    retryTimer (new QTimer (this)),
    currentTimestamp (0) {
    QObject::connect (this->javascriptBridge, &JavascriptBridge::valueReturnedFromJavascript, this, &JobWorker::valueReturnedFromJavascript);
    QObject::connect (this->retryTimer, &QTimer::timeout, this, &JobWorker::setCurrentTimestamp);
    QObject::connect (this->retryTimer, &QTimer::timeout, this, &JobWorker::processIncomingRequest);
    for (QHash<QString,QString>::const_iterator appHelper = AppRuntime::helperSources.constBegin(); appHelper != AppRuntime::helperSources.constEnd(); appHelper++) {
        AppHelperInfo* appHelperInfo = new AppHelperInfo;
        this->helperInstances.append (appHelperInfo);
        appHelperInfo->name = appHelper.key();
        appHelperInfo->databaseCache = new ObjectCacheDatabase (appHelperInfo->name, AppRuntime::dbTblPrefix);
        appHelperInfo->memoryCache = new ObjectCacheMemory (appHelperInfo->name, (*(appHelperInfo->databaseCache)));
        appHelperInfo->entryPoint = this->runtimeEnvironment->evaluate (appHelper.value(), appHelperInfo->name + AppConstants::AppHelperExtension);
        if (! JavascriptBridge::warnJsError (appHelperInfo->entryPoint, QString("A Javascript exception occurred while the helper '%1' was initializing. It will be disabled!").arg(appHelperInfo->name))) {
            this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, this->helperInstances.count(), JavascriptMethod::getSupportedUrls);
        }
    }
}

JobWorker::~JobWorker () {
    if ((! (this->runningRequests.isEmpty() && this->finishRequested && this->incomingRequests.isEmpty ())) || this->retryTimer->isActive ()) {
        qFatal("Program tried to destruct a JobWorker unexpectedly!");
    }
    while (! this->helperInstances.isEmpty ()) {
        AppHelperInfo* appHelperInfo (this->helperInstances.takeLast ());
        delete (appHelperInfo->memoryCache);
        delete (appHelperInfo->databaseCache);
        delete (appHelperInfo);
    }
    delete (this->retryTimer);
    delete (this->runtimeEnvironment);
}

void JobWorker::valueReturnedFromJavascript (int context, const QString& method, const QJSValue& returnedValue) {
    if (method == "getSupportedUrls") {
        this->processSupportedUrls (context, returnedValue);
    } else if (method == "getObjectFromUrl") {
        this->processObjectFromUrl (context, returnedValue);
    } else if (method == "getPropertiesFromObject") {
#warning TODO
#warning Remember not to process objects without properties from the helper
#warning It is used internally...
        this->squidResponseOut (context, "Answer to 'getPropertiesFromObject();' is not ready yet!", true, false);
        qCritical() << JavascriptBridge::QJS2QString (returnedValue);
        qCritical() << "Answer to 'getPropertiesFromObject();' is not ready yet!";
    } else {
        qCritical() << QString("Javascript returned value from an unexpected method invocation: context=%1 , method='%2' , returnedValue='%3'").arg(context).arg(method).arg(JavascriptBridge::QJS2QString (returnedValue));
    }
}

void JobWorker::processIncomingRequest () {
    AppSquidRequest squidRequest (this->incomingRequests.takeLast());
    if (this->incomingRequests.isEmpty ()) {
        this->retryTimer->stop ();
    }
    QString urlString (squidRequest.url.toString ());
    AppHelperInfo* appHelperInfo = Q_NULLPTR;
    int numHelpers = this->helperInstances.count ();
    if (squidRequest.helperName.isEmpty ()) {
        for (int helperPos = 0; helperPos < numHelpers; helperPos++) {
            appHelperInfo = this->helperInstances[helperPos];
            if (appHelperInfo->entryPoint.isCallable ()) {
                int numSupportedUrls = appHelperInfo->supportedURLs.count ();
                for (int supportedUrlPos = 0; supportedUrlPos < numSupportedUrls; supportedUrlPos++) {
                    if (urlString.indexOf (appHelperInfo->supportedURLs[supportedUrlPos]) >= 0) {
                        squidRequest.helperName = appHelperInfo->name;
                        supportedUrlPos = numSupportedUrls;
                        helperPos = numHelpers;
                    }
                }
            }
        }
    } else {
        for (int helperPos = 0; helperPos < numHelpers; helperPos++) {
            appHelperInfo = this->helperInstances[helperPos];
            if (appHelperInfo->name == squidRequest.helperName) {
                helperPos = numHelpers;
            }
        }
    }
    if (squidRequest.helperName.isEmpty ()) {
        this->squidResponseOut (0, "Unable to find a helper that handles the requested URL.", false, false);
    } else {
        int requestId (this->requestId);
        this->requestId += 2;
        this->runningRequests[requestId] = squidRequest;
        qDebug() << QString("[%1] Invoking 'getObjectFromUrl ();' - RequestID #%2").arg(appHelperInfo->name).arg(requestId);
        if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, requestId, JavascriptMethod::getObjectFromUrl, urlString)) {
            this->squidResponseOut (requestId, "'getObjectFromUrl ();' function returned an error!", true, false);
        }
    }
}

void JobWorker::setCurrentTimestamp () {
    // I need to update this->currentTimestamp manually if the JobWorker stops receiving jobs from JobDispatcher
    // probably because of an EOF at STDIN or an absence of requests from Squid.
    qint64 currentTimestamp (AppRuntime::currentDateTime().toMSecsSinceEpoch());
    if (! this->rngdInitialized) {
        qsrand ((uint) currentTimestamp);
        QThread::msleep (1);
        this->rngdInitialized = true;
    }
    this->currentTimestamp = (currentTimestamp / 1000);
}

void JobWorker::squidRequestIn (const AppSquidRequest& squidRequest) {
    qDebug() << QString("JobDispatcher has sent an ACL matching request to channel #%1.").arg(this->requestChannel);
    if (this->currentTimestamp < squidRequest.timestampNow) {
        this->currentTimestamp = squidRequest.timestampNow;
    }
    this->incomingRequests.prepend (squidRequest);
    this->processIncomingRequest ();
}

void JobWorker::quit () {
    this->finishRequested = true;
    int numPendingRequests (this->runningRequests.count () + this->incomingRequests.count());
    if (numPendingRequests == 0) {
        qInfo() << QString("Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    } else if (numPendingRequests == 1) {
        qInfo() << QString("Finish request received by handler for channel #%1, but there is a pending answer.").arg(this->requestChannel);
    } else {
        qInfo() << QString("Finish request received by handler for channel #%1, but there are %2 pending answers.").arg(this->requestChannel).arg(numPendingRequests);
    }
}

//////////////////////////////////////////////////////////////////

JobCarrier::JobCarrier (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    started (false),
    threadObj (new QThread (this)),
    workerObj (new JobWorker (requestChannel, Q_NULLPTR)) {
    this->workerObj->moveToThread (this->threadObj);
    QObject::connect (this->workerObj, &JobWorker::finished, this->threadObj, &QThread::quit);
    QObject::connect (this->threadObj, &QThread::finished, this->workerObj, &QObject::deleteLater);
    QObject::connect (this->threadObj, &QThread::finished, this, &JobCarrier::finished);
    QObject::connect (this, &JobCarrier::squidRequestOut, this->workerObj, &JobWorker::squidRequestIn, Qt::QueuedConnection);
}

JobCarrier::~JobCarrier () {
    if (this->threadObj->isRunning ()) {
        qFatal("Program tried to destruct a JobCarrier unexpectedly!");
    }
    delete (this->threadObj);
}

void JobCarrier::start (QThread::Priority priority) {
    if (this->started) {
        qFatal ("Invalid procedure call! This method must be called only once!");
    } else {
        this->threadObj->start (priority);
        this->started = true;
    }
}

void JobCarrier::squidRequestIn (const AppSquidRequest& squidRequest) {
    emit squidRequestOut (squidRequest.deepCopy ());
}
