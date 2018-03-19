#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QMutex>
#include <QRegExp>
#include <QSemaphore>
#include <QString>
#include <QUrl>

class AppHelperObjectCache;

class AppRuntime {
public:
    // Settings read from [main] section
    static QString loglevel;
    static QString helpers;
    static QString registryTTL;
    // Settings read from [db] section
    static QString dbDriver;
    static QString dbHost;
    static QString dbPort;
    static QString dbUser;
    static QString dbPassword;
    static QString dbOptions;
    static QString dbName;
    static QString dbStartupQuery;
    static QString dbTblPrefix;

    // Runtime configuration
    // Helpers information
    static QMutex helperMemoryCacheMutex;
    static QHash<QString,AppHelperObjectCache*> helperMemoryCache; // All functions in class QHash<Key,T> are reentrant
    static QHash<QString,QString> helperSources; // I don't need a QMutex protector to this QHash because JobWorker::JobWorker() constructor runs in the main thread
    static QMutex commonSourcesMutex;
    static QHash<QString,QString> commonSources; // All functions in class QHash<Key,T> are reentrant
    // Database information
    static int dbInstance;
    static QStringList dbStartupQueries;
    static QMutex dbSettingsMutex;
    // Number version of registryTTL
    static qint64 registryTTLint;
    // A static method for datetime retrieval, because the native class is reentrant
    static QDateTime currentDateTime();
};

class AppConstants {
public:
    static const int AppHelperMutexTimeout;
    static const int AppHelperCacheMaxSize;
    static const int AppHelperCacheVacuumSize;
    static const int AppHelperTimerTimeout;
    static const int AppHelperMaxWait;
    static const QString AppCommonSubDir;
    static const QString AppHelperSubDir;
    static const QString AppHelperExtension;
    static const QString AppHelperCodeHeader;
    static const QString AppHelperCodeFooter;
};

class AppHelperObject {
public:
    QJsonDocument data;
    qint64 timestampCreated;
};

class AppHelperClass {
public:
    QHash<QString,AppHelperObject*> ids;
    ~AppHelperClass ();
};

class AppHelperObjectCache {
public:
    QHash<QString,AppHelperClass*> classNames;
    QMutex mutex;
    ~AppHelperObjectCache ();
};

// QHash<QString,AppHelperObjectCache> helperMemoryCache
// qDebug() << helperMemoryCache["youtube"]->className["video"]->id["dQw4w9WgXcQ"]->data;

class AppSquidRequest {
public:
    QUrl url;
    QString property;
    Qt::CaseSensitivity caseSensivity;
    QRegExp::PatternSyntax patternSyntax;
    QStringList criteria;
    qint64 timestampNow;
    QString helperName;
    // Method that forces a deep copy of the object
    AppSquidRequest deepCopy () const;
};

#endif // APPRUNTIME_H
