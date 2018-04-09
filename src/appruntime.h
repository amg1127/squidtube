#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QLinkedList>
#include <QList>
#include <QMutex>
#include <QPair>
#include <QRegExp>
#include <QSemaphore>
#include <QString>
#include <QtDebug>
#include <QTextStream>
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
    // Static method that forces a deep copy of a list
    template<class T> static void deepCopyList (T& destination, const T& source);
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
// qDebug() << helperMemoryCache["youtube"]->className["video"]->id["dQw4w9WgXcQ"]->data;

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
    OperatorLessThan = 1,
    OperatorLessThanOrEquals = 2,
    OperatorEquals = 3,
    OperatorNotEquals = 4,
    OperatorGreaterThanOrEquals = 5,
    OperatorGreaterThan = 6
};

class AppSquidRequest {
public:
    QUrl requestUrl;
    QLinkedList<AppSquidPropertyMatch> requestProperties;
    AppSquidMathMatchOperator requestMathMatchOperator;
    Qt::CaseSensitivity requestCaseSensitivity;
    QRegExp::PatternSyntax requestPatternSyntax;
    bool requestInvertMatch;
    QStringList requestCriteria;
    QString requestHelperName;
    int requestHelperId;
    bool hasRequestHelperOnProgress;
    QString objectClassName;
    QString objectId;
    // Method that forces a deep copy of the object
    AppSquidRequest deepCopy () const;
};

#endif // APPRUNTIME_H
