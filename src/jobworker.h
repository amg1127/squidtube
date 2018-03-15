#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"
#include "javascriptbridge.h"

#include <QList>
#include <QNetworkAccessManager>
#include <QQmlEngine>
#include <QQmlNetworkAccessManagerFactory>
#include <QRegExp>
#include <QJSValue>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>

class AppHelperInfo {
public:
    QString name;
    bool isAvailable;
    QList<QRegExp> supportedURLs;
    QJSValue entryPoint;
};

class JobWorker : public QObject {
    Q_OBJECT
private:
    unsigned int numPendingJobs;
    bool finishRequested;
    QString requestChannel;
    QList<AppHelperInfo*> helperInstances;
    QQmlEngine* runtimeEnvironment;
    void squidResponseOut (const QString& msg, bool isError, bool isMatch);
    static bool warnJsError (const QJSValue& jsValue, const QString& msg = QString());
public:
    JobWorker (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
public slots:
    void quit ();
    void squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
signals:
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
    void finished();
};

class JobCarrier : public QObject {
    Q_OBJECT
private:
    bool started;
    QThread* threadObj;
    JobWorker* workerObj;
public:
    JobCarrier (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobCarrier ();
    inline JobWorker* worker () { return (this->workerObj); }
    void start (QThread::Priority priority = QThread::InheritPriority);
    inline bool wait (unsigned long time = ULONG_MAX) { return (this->threadObj->wait (time)); }
    void squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
signals:
    void squidRequestOut (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
    void finished ();
};

#endif // JOBWORKER_H
