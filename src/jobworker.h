#ifndef JOBWORKER_H
#define JOBWORKER_H

#include "appruntime.h"

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
    bool isBusy;
    QList<QRegExp> supportedURLs;
    QQmlEngine environment;
};

class JobWorker : public QObject {
    Q_OBJECT
private:
    QString requestChannel;
    QList<AppHelperInfo*> helperInstances;
    static bool warnJsError (const QJSValue& jsValue, const QString& msg = QString());
public:
    JobWorker (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobWorker();
public slots:
    void squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
signals:
    void writeAnswerLine (const QString& channel, const QString& msg, bool isError, bool isMatch);
};

class JobCarrier : public QObject {
    Q_OBJECT
private:
    QThread* threadObj;
    JobWorker* workerObj;
public:
    JobCarrier (const QString& requestChannel, QObject* parent = Q_NULLPTR);
    ~JobCarrier ();
    inline JobWorker* worker () { return (this->workerObj); }
    inline void start (QThread::Priority priority = QThread::InheritPriority) { this->threadObj->start (priority); }
    void squidRequestIn (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
signals:
    void squidRequestOut (const QUrl& requestUrl, const QString& requestProperty, Qt::CaseSensitivity requestCaseSensivity, QRegExp::PatternSyntax requestPatternSyntax, const QStringList& requestData);
};

#endif // JOBWORKER_H
