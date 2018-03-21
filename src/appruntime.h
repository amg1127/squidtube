#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QLinkedList>
#include <QList>
#include <QMutex>
#include <QRegExp>
#include <QSemaphore>
#include <QString>
#include <QUrl>

class AppHelperObject;
class AppHelperClass;
class AppHelperObjectCache;

class AppRuntime {
public:
    // Settings read from [main] section
    static QString loglevel;
    static QString helpers;
    static QString positiveTTL;
    static QString negativeTTL;
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
    static QList<AppHelperObjectCache*> helperMemoryCache; // All functions in class QHash<Key,T> are reentrant
    // I don't need a QMutex protection to helperSources* because JobWorker::JobWorker() constructor runs in the main thread
    static QHash<QString,QString> helperSourcesByName;
    static QStringList helperNames;
    static QMutex commonSourcesMutex;
    static QHash<QString,QString> commonSources; // All functions in class QHash<Key,T> are reentrant
    // Database information
    static int dbInstance;
    static QStringList dbStartupQueries;
    static QMutex dbSettingsMutex;
    // Number versions of positiveTTL and negativeTTL
    static qint64 positiveTTLint;
    static qint64 negativeTTLint;
    // A static method for datetime retrieval, because the native class is reentrant
    static QDateTime currentDateTime();
    // JSON retrieved from the database must pass this test
    // inline static bool validateJsonData (const QJsonDocument& data) { return ((! data.isNull()) && (! data.isEmpty()) && (! data.isObject()) && data.isArray ()); }
    // A static method to check whether a JSON document is considered fresh
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
    AppHelperClass* objectClass;
};

class AppHelperClass {
public:
    AppHelperObjectCache* objectCache;
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

enum class PropertyMatchQuantity {
    MatchNone = 0,
    MatchAny = 1,
    MatchAll = 2
};

class AppSquidPropertyMatch {
public:
    QString matchName;
    int matchFrom;
    int matchTo;
    PropertyMatchQuantity matchQuantity;
};

enum class AppSquidMathMatchOperator {
    String = 1,
    LessThan = 1,
    LessThanOrEquals = 2,
    Equals = 3,
    NotEquals = 4,
    GreaterThanOrEquals = 5,
    GreaterThan = 6
};

class AppSquidRequest {
public:
    QUrl requestUrl;
    QLinkedList<AppSquidPropertyMatch> requestProperties;
    AppSquidMathMatchOperator requestMathOperator;
    Qt::CaseSensitivity requestCaseSensivity;
    QRegExp::PatternSyntax requestPatternSyntax;
    QStringList requestCriteria;
    QString requestHelperName;
    int requestHelperId;
    QString objectClassName;
    QString objectId;
    // Method that forces a deep copy of the object
    AppSquidRequest deepCopy () const;
};

#endif // APPRUNTIME_H
