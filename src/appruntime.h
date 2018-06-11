#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJSValue>
#include <QLinkedList>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QRegExp>
#include <QSemaphore>
#include <QString>
#include <QTextStream>
#include <QUrl>

#ifdef QT_NO_DEBUG
#define downcast static_cast
#else
#define downcast dynamic_cast
#endif

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
    static QString squidProtocol;
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
    static int helperSourcesStartLine;
    static QMutex commonSourcesMutex;
    static QHash<QString,QString> commonSources; // All functions in class QHash<Key,T> are reentrant
    // Database information
    static unsigned int dbInstance;
    static QStringList dbStartupQueries;
    static QMutex dbSettingsMutex;
    // Number versions of positiveTTL and negativeTTL
    static qint64 positiveTTLint;
    static qint64 negativeTTLint;
    // Mutex to handle reentrancy of the QTextCodec class
    static QMutex textCoDecMutex;
    // A static method for datetime retrieval, because the native class is reentrant
    static QDateTime currentDateTime();
    // Static methods for file reading. I use them several times along the program
    static QString readFileContents (const QString& fileName);
    static QString readFileContents (QFile& fileObj);
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
};

class AppHelperObject {
public:
    QJsonDocument data;
    qint64 timestampCreated;
    AppHelperClass* objectClass;
    QLinkedList<AppHelperObject*>::iterator cachePosition;
    QHash<QString,AppHelperObject*>::iterator classPosition;
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
    QLinkedList<AppHelperObject*> cachedIds;
    int cacheSize;
    ~AppHelperObjectCache ();
};

// QHash<QString,AppHelperObjectCache> helperMemoryCache
// qDebug ("%s", helperMemoryCache["youtube"]->className["video"]->id["dQw4w9WgXcQ"]->data.toLatin1().constData());

enum PropertyMatchType {
    MatchObject = 1,
    MatchArray = 2
};

enum PropertyMatchQuantity {
    MatchAny = 1,
    MatchAll = 2
};

class AppSquidPropertyMatch {
public:
    QString componentName;
    PropertyMatchType matchType;
    QList< QPair<int,int> > matchIntervals;
    PropertyMatchQuantity matchQuantity;
};

enum AppSquidMathMatchOperator {
    OperatorString = 1,
    OperatorLessThan = 2,
    OperatorLessThanOrEquals = 3,
    OperatorEquals = 4,
    OperatorNotEquals = 5,
    OperatorGreaterThanOrEquals = 6,
    OperatorGreaterThan = 7
};

enum AppRequestType {
    RequestFromSquid = 1,
    RequestFromHelper = 2
};

class AppRequestObject {
public:
    QString className;
    QString id;
};

class AppSquidRequest {
public:
    QUrl url;
    QLinkedList<AppSquidPropertyMatch> properties;
    AppSquidMathMatchOperator mathMatchOperator;
    Qt::CaseSensitivity caseSensitivity;
    QRegExp::PatternSyntax patternSyntax;
    bool invertedMatch;
    QStringList criteria;
};

class AppHelperRequest {
public:
    QJSValue callback;
    QJSValue object;
};

class AppRequestHelper {
public:
    QString name;
    int id;
    bool isOnProgress;
};

class AppJobRequest {
protected:
    virtual ~AppJobRequest ();
public:
    AppRequestObject object;
    AppRequestHelper helper;
    virtual AppRequestType type() = 0;
};

class AppJobRequestFromSquid : public AppJobRequest {
public:
    virtual ~AppJobRequestFromSquid ();
    AppSquidRequest data;
    virtual AppRequestType type();
};

class AppJobRequestFromHelper : public AppJobRequest {
public:
    virtual ~AppJobRequestFromHelper ();
    AppHelperRequest data;
    virtual AppRequestType type();
};
#endif // APPRUNTIME_H
