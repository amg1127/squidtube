#include "jobworker.h"

bool JobWorker::warnJsError (const QJSValue& jsValue, const QString& msg) {
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

void JobWorker::squidResponseOut (const QString& msg, bool isError, bool isMatch) {
    emit writeAnswerLine (this->requestChannel, msg, isError, isMatch);
    this->numPendingJobs--;
    if (this->finishRequested && this->numPendingJobs == 0) {
        qInfo() << QString("No pending jobs now. Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    }
}

JobWorker::JobWorker (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    numPendingJobs (0),
    finishRequested (false),
    requestChannel (requestChannel),
    runtimeEnvironment (new QQmlEngine (this)) {
    for (QList<AppHelper>::const_iterator appHelper = AppRuntime::helperObjects.constBegin(); appHelper != AppRuntime::helperObjects.constEnd(); appHelper++) {
        AppHelperInfo* appHelperInfo = new AppHelperInfo;
        appHelperInfo->name = appHelper->name;
        appHelperInfo->isAvailable = false;
        appHelperInfo->entryPoint = this->runtimeEnvironment->evaluate (appHelper->code, appHelperInfo->name + AppHelper::AppHelperExtension), QString("A Javascript exception occurred while the helper '%1' was initializing. It will be disabled!").arg(appHelperInfo->name);
        if (! JobWorker::warnJsError (appHelperInfo->entryPoint)) {
            if (appHelperInfo->entryPoint.isCallable ()) {
                appHelperInfo->isAvailable = true;
                /*
                // The helper script must implement the supportedUrls() function and
                // must return an array of either string or regular expression elements
                QJSValue appHelperSupportedUrls (appHelperInfo->environment.evaluate ("supportedUrls();"));
                if (! JobWorker::warnJsError (appHelperSupportedUrls, QString("A Javascript exception occurred while collecting information from the helper '%1'. It will be disabled!").arg(appHelperInfo->name))) {
                    if (appHelperSupportedUrls.isArray()) {
                        quint32 appHelperSupportedUrlsLength = appHelperSupportedUrls.property("length").toUInt();
                        for (quint32 pos = 0; pos < appHelperSupportedUrlsLength; pos++) {
                            QJSValue appHelperSupportedUrl (appHelperSupportedUrls.property (pos));
                            QRegExp regExpSupportedUrl;
                            if (appHelperSupportedUrl.isString ()) {
                                regExpSupportedUrl = QRegExp (appHelperSupportedUrl.toString(), Qt::CaseInsensitive, QRegExp::WildcardUnix);
                            } else if (appHelperSupportedUrl.isRegExp ()) {
                                regExpSupportedUrl = appHelperSupportedUrl.toVariant().toRegExp();
                            }
                            if (regExpSupportedUrl.isValid ()) {
                                appHelperInfo->supportedURLs.append (regExpSupportedUrl);
                            } else {
                                qWarning() << QString("'supportedUrls();' function from helper %1 returned an invalid item at position #%2!").arg(appHelperInfo->name).arg(pos);
                            }
                        }
                        // appHelperInfo->isAvailable = true;
                    } else {
                        qWarning() << QString("'supportedUrls();' function returned invalid data. The helper '%1' will be disabled!").arg(appHelperInfo->name);
                    }
                }
                */
            } else {
                qWarning() << QString("Initialization returned '%1' instead of a function. The helper '%2' will be disabled!").arg(JavascriptBridge::QJS2QString(appHelperInfo->entryPoint)).arg(appHelperInfo->name);
            }
        }
        this->helperInstances.append (appHelperInfo);
    }
}

JobWorker::~JobWorker () {
    if (! (this->numPendingJobs == 0 && this->finishRequested)) {
        qFatal("Program tried to destruct a JobWorker unexpectedly!");
    }
    while (! this->helperInstances.isEmpty ()) {
        delete (this->helperInstances.takeLast ());
    }
    delete (this->runtimeEnvironment);
}

void JobWorker::squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData) {
    qDebug() << QString("JobDispatcher has sent an ACL matching request to channel #%1.").arg(this->requestChannel);
    this->numPendingJobs++;

#warning Testing
    qDebug() << requestUrl << requestProperty << requestCaseSensivity << requestPatternSyntax << requestData;
    this->squidResponseOut ("Not implemented yet!", true, false);
}

void JobWorker::quit () {
    this->finishRequested = true;
    if (this->numPendingJobs == 0) {
        qInfo() << QString("Finished handler for channel #%1.").arg(this->requestChannel);
        emit finished ();
    } else if (this->numPendingJobs == 1) {
        qInfo() << QString("Finish request received by handler for channel #%1, but there is a pending answer.").arg(this->requestChannel);
    } else {
        qInfo() << QString("Finish request received by handler for channel #%1, but there are %2 pending answers.").arg(this->requestChannel).arg(this->numPendingJobs);
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
    QObject::connect (this, &JobCarrier::squidRequestOut, this->workerObj, &JobWorker::squidRequestIn);
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

void JobCarrier::squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData) {
    emit squidRequestOut (requestUrl, requestProperty, requestCaseSensivity, requestPatternSyntax, requestData);
}
