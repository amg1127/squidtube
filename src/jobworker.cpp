#include "jobworker.h"

JobWorker::JobWorker (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    requestChannel (requestChannel) {
    for (QList<AppHelper>::const_iterator appHelper = AppRuntime::helperObjects.constBegin(); appHelper != AppRuntime::helperObjects.constEnd(); appHelper++) {
        AppHelperInfo* appHelperInfo = new AppHelperInfo;
        appHelperInfo->name = appHelper->name;
        appHelperInfo->isAvailable = false;
        appHelperInfo->isBusy = false;
        for (QHash<QString,QString>::const_iterator globalVariable = appHelper->globalVariables.constBegin(); globalVariable != appHelper->globalVariables.constEnd(); globalVariable++) {
            appHelperInfo->environment.globalObject().setProperty (globalVariable.key(), globalVariable.value());
        }
#error This code must be refactored!
        if (! JobWorker::warnJsError (appHelperInfo->environment.evaluate (QString("function ") + appHelperInfo->name + "() {\n" + appHelper->code + "}\n", appHelperInfo->name + AppHelper::AppHelperExtension, 0), QString("A Javascript exception occurred while the helper '%1' was initializing. It will be disabled!").arg(appHelperInfo->name))) {
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
                    appHelperInfo->isAvailable = true;
                } else {
                    qWarning() << QString("'supportedUrls();' function returned invalid data. The helper '%1' will be disabled!").arg(appHelperInfo->name);
                }
            }
        }
        this->helperInstances.append (appHelperInfo);
    }
}

JobWorker::~JobWorker () {
    while (! this->helperInstances.isEmpty ()) {
        delete (this->helperInstances.takeLast ());
    }
}

bool JobWorker::warnJsError (const QJSValue& jsValue, const QString& msg) {
    if (jsValue.isError ()) {
        qInfo() << QString("Javascript exception at %1:%2: %3: %4")
            .arg(jsValue.property("fileName").toString())
            .arg(jsValue.property("lineNumber").toInt())
            .arg(jsValue.property("name").toString())
            .arg(jsValue.property("message").toString());
        QStringList stackLines (jsValue.property("stack").toString().split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts));
        for (QStringList::const_iterator stackLine = stackLines.constBegin(); stackLine != stackLines.constEnd(); stackLine++) {
            qDebug() << (*stackLine);
        }
        if (! msg.isEmpty ()) {
            qWarning() << msg;
        }
        return (true);
    } else {
        return (false);
    }
}

void JobWorker::squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData) {
#warning Testing
    qDebug() << requestUrl << requestProperty << requestCaseSensivity << requestPatternSyntax << requestData;
    emit writeAnswerLine (this->requestChannel, "Not implemented yet!", true, false);
}

//////////////////////////////////////////////////////////////////

JobCarrier::JobCarrier (const QString& requestChannel, QObject* parent) :
    QObject (parent),
    threadObj (new QThread (this)),
    workerObj (new JobWorker (requestChannel, Q_NULLPTR)) {
    this->workerObj->moveToThread (this->threadObj);
    QObject::connect (this->threadObj, &QThread::finished, this->workerObj, &QObject::deleteLater);
    QObject::connect (this, &JobCarrier::squidRequestOut, this->workerObj, &JobWorker::squidRequestIn);
}

JobCarrier::~JobCarrier () {
    this->threadObj->quit ();
    this->threadObj->wait ();
    delete (this->threadObj);
}

void JobCarrier::squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData) {
    emit squidRequestOut (requestUrl, requestProperty, requestCaseSensivity, requestPatternSyntax, requestData);
}
