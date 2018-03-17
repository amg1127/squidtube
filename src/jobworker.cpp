#include "jobworker.h"

int JobWorker::appendRequest (const AppSquidRequest& squidRequest, const QString& helperName) {
    int requestId (this->requestId);
    this->requestId += 2;
    this->pendingRequestIDs.append (requestId);
    this->pendingRequests.append (squidRequest);
    this->pendingRequests.last().helperName = helperName;
    return (requestId);
}

AppSquidRequest JobWorker::getRequest (int requestId) {
    int requestPos = this->pendingRequestIDs.indexOf (requestId);
    if (requestPos >= 0) {
        return (this->pendingRequests.at(requestPos));
    } else {
        return (AppSquidRequest ());
    }
}

AppSquidRequest JobWorker::takeRequest (int requestId) {
    AppSquidRequest squidRequest;
    int requestPos = this->pendingRequestIDs.indexOf (requestId);
    if (requestPos >= 0) {
        this->pendingRequestIDs.removeAt (requestPos);
        squidRequest = this->pendingRequests.takeAt (requestPos);
    }
    return (squidRequest);
}

void JobWorker::squidResponseOut (const int requestId, const QString& msg, bool isError, bool isMatch) {
    AppSquidRequest squidRequest (this->takeRequest (requestId));
    if (squidRequest.helperName.isEmpty ()) {
        emit writeAnswerLine (this->requestChannel, msg, isError, isMatch);
    } else {
        emit writeAnswerLine (this->requestChannel, QString("[") + squidRequest.helperName + "] " + msg, isError, isMatch);
    }
    if (this->finishRequested && this->pendingRequestIDs.isEmpty ()) {
        qInfo() << QString("No pending jobs now. Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    }
}

void JobWorker::processSupportedUrls (int helperInstance, const QJSValue& appHelperSupportedUrls) {
    if (helperInstance > 0 && helperInstance <= this->helperInstances.count() && appHelperSupportedUrls.isArray()) {
        AppHelperInfo* appHelperInfo (this->helperInstances[helperInstance - 1]);
        int length = appHelperSupportedUrls.property("length").toInt();
        qDebug() << QString("Received %1 patterns of supported URL's from helper '%2'.").arg(length).arg(appHelperInfo->name);
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
                    qWarning() << QString("Discarding the empty regular expression #%1 returned by helper '%2'...").arg(i).arg(appHelperInfo->name);
                } else {
                    appHelperInfo->supportedURLs.append (regExpSupportedUrl);
                }
            } else {
                qWarning() << QString("Helper '%1' returned an invalid item: '%2'").arg(appHelperInfo->name).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrl));
            }
        }
    } else {
        qWarning() << QString("Invalid returned data: count=%1 , instance=%2 , data='%3'").arg(this->helperInstances.count()).arg(helperInstance).arg(JavascriptBridge::QJS2QString(appHelperSupportedUrls));
    }
}

void JobWorker::processObjectFromUrl (int requestId, const QJSValue& appHelperObjectFromUrl) {
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
        qDebug() << QString("[%1] Searching for information concerning 'className=%2' , 'id=%3'...").arg(this->getRequest(requestId).helperName).arg(objectClassName).arg(objectId);
#warning Now, the time to program cache management comes.
    }
}

JobWorker::JobWorker (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    finishRequested (false),
    requestChannel (requestChannel),
    runtimeEnvironment (new QQmlEngine (this)),
    javascriptBridge (new JavascriptBridge ((*runtimeEnvironment), requestChannel)),
    requestId (1) {
    QObject::connect (this->javascriptBridge, &JavascriptBridge::valueReturnedFromJavascript, this, &JobWorker::valueReturnedFromJavascript);
    for (QHash<QString,QString>::const_iterator appHelper = AppRuntime::helperSources.constBegin(); appHelper != AppRuntime::helperSources.constEnd(); appHelper++) {
        AppHelperInfo* appHelperInfo = new AppHelperInfo;
        this->helperInstances.append (appHelperInfo);
        appHelperInfo->name = appHelper.key();
        appHelperInfo->isAvailable = false;
        appHelperInfo->entryPoint = this->runtimeEnvironment->evaluate (appHelper.value(), appHelperInfo->name + AppHelper::AppHelperExtension);
        if (! JavascriptBridge::warnJsError (appHelperInfo->entryPoint, QString("A Javascript exception occurred while the helper '%1' was initializing. It will be disabled!").arg(appHelperInfo->name))) {
            if (this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, this->helperInstances.count(), JavascriptMethod::getSupportedUrls)) {
                appHelperInfo->isAvailable = true;
            }
        }
    }
}

JobWorker::~JobWorker () {
    if (! (this->pendingRequestIDs.isEmpty () && this->finishRequested)) {
        qFatal("Program tried to destruct a JobWorker unexpectedly!");
    }
    while (! this->helperInstances.isEmpty ()) {
        delete (this->helperInstances.takeLast ());
    }
    delete (this->runtimeEnvironment);
}

void JobWorker::valueReturnedFromJavascript (int context, const QString& method, const QJSValue& returnedValue) {
    if (method == "getSupportedUrls") {
        this->processSupportedUrls (context, returnedValue);
    } else if (method == "getObjectFromUrl") {
        this->processObjectFromUrl (context, returnedValue);
    } else if (method == "getPropertiesFromObject") {
#warning TODO
        qCritical() << "Answer to 'getPropertiesFromObject();' is not ready yet!";
    } else {
        qCritical() << QString("Javascript returned value from an unexpected method invocation: context=%1 , method='%2' , returnedValue='%3'").arg(context).arg(method).arg(JavascriptBridge::QJS2QString (returnedValue));
    }
}

void JobWorker::squidRequestIn (const AppSquidRequest& squidRequest) {
    qDebug() << QString("JobDispatcher has sent an ACL matching request to channel #%1.").arg(this->requestChannel);
    int numHelpers = this->helperInstances.count ();
    QString urlString (squidRequest.url.toString ());
    for (int helperPos = 0; helperPos < numHelpers; helperPos++) {
        AppHelperInfo* appHelperInfo (this->helperInstances[helperPos]);
        int numSupportedUrls = appHelperInfo->supportedURLs.count ();
        for (int supportedUrlPos = 0; supportedUrlPos < numSupportedUrls; supportedUrlPos++) {
            if (urlString.indexOf (appHelperInfo->supportedURLs[supportedUrlPos]) >= 0) {
                int requestId (this->appendRequest (squidRequest, appHelperInfo->name));
                if (! this->javascriptBridge->invokeMethod (appHelperInfo->entryPoint, requestId, JavascriptMethod::getObjectFromUrl, urlString)) {
                    this->squidResponseOut (requestId, "'getObjectFromUrl ();' function returned an error!", true, false);
                }
                return;
            }
        }
    }
    this->squidResponseOut (0, "Unable to find a helper that handles the requested URL.", false, false);
}

void JobWorker::quit () {
    this->finishRequested = true;
    int numPendingRequests (this->pendingRequestIDs.count ());
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
    emit squidRequestOut (squidRequest);
}
