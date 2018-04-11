#include "appruntime.h"

//////////////////////////////////////////////////////////////////
// Default application settings go below

#ifdef QT_NO_DEBUG
QString AppRuntime::loglevel("INFO");
#else
QString AppRuntime::loglevel("DEBUG");
#endif
QString AppRuntime::helpers("");
QString AppRuntime::positiveTTL("2592000");
QString AppRuntime::negativeTTL("600");
QString AppRuntime::squidProtocol("3.4");
QString AppRuntime::dbDriver("QSQLITE");
QString AppRuntime::dbHost("");
QString AppRuntime::dbPort("0");
QString AppRuntime::dbUser("");
QString AppRuntime::dbPassword("");
QString AppRuntime::dbOptions("");
QString AppRuntime::dbName(":memory:");
QString AppRuntime::dbStartupQuery("");
QString AppRuntime::dbTblPrefix("tblHelper_");

// Default application settings go above
//////////////////////////////////////////////////////////////////

QMutex AppRuntime::helperMemoryCacheMutex;
QList<AppHelperObjectCache*> AppRuntime::helperMemoryCache;
QHash<QString,QString> AppRuntime::helperSourcesByName;
int AppRuntime::helperSourcesStartLine;
QStringList AppRuntime::helperNames;
QMutex AppRuntime::commonSourcesMutex;
QHash<QString,QString> AppRuntime::commonSources;

unsigned int AppRuntime::dbInstance = 1;
QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;

qint64 AppRuntime::positiveTTLint = 0;
qint64 AppRuntime::negativeTTLint = 0;

QMutex AppRuntime::textCoDecMutex;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}


QString AppRuntime::readFileContents (const QString& fileName) {
    QFile fileObj (fileName);
    return (readFileContents (fileObj));
}

QString AppRuntime::readFileContents (QFile& fileObj) {
    if (fileObj.open (QIODevice::ReadOnly | QIODevice::Text)) {
        QString fileContents;
        bool read = true;
        {
            QTextStream fileStreamObj (&fileObj);
            fileContents = fileStreamObj.readAll ();
        }
        if (fileObj.error() != QFileDevice::NoError) {
            qWarning() << QString("Error reading contents of file '%1': '%2'!").arg(fileObj.fileName()).arg(fileObj.errorString());
            read = false;
        }
        if (! fileObj.atEnd ()) {
            qWarning() << QString("The file '%1' was not read completely!").arg(fileObj.fileName());
            read = false;
        }
        fileObj.close ();
        if (read) {
            qDebug() << QString("Sucessfully loaded contents of the file '%1'.").arg(fileObj.fileName());
            return (fileContents);
        }
    } else {
        qWarning() << QString("Unable to open file '%1' for reading: '%2'!").arg(fileObj.fileName()).arg(fileObj.errorString());
    }
    return (QString());
}

//////////////////////////////////////////////////////////////////

// Constants used by the memory cache system
// The maximum time in miliseconds for the memory cache mutex to become available
const int AppConstants::AppHelperMutexTimeout = 30000;
// How many objects at most the cache is allowed to store
const int AppConstants::AppHelperCacheMaxSize = 8192;
// How many elements will be dropped from the cache if it has reached the maximum size
const int AppConstants::AppHelperCacheVacuumSize = 2048;

// How long the program will wait after detecting that another thread or process started
// to fetch data from an object whose information the program wanted to get.
// If this timer expires the program will check again if such thread of process has already
// finished its job.
// Unit is milliseconds.
const int AppConstants::AppHelperTimerTimeout = 1000;

// How long the program will wait for such thread or process. If this timer expires,
// The program will by itself fetch data from the object
// Unit is seconds.
const int AppConstants::AppHelperMaxWait = 300;

// Path component where common libraries will be searched for
const QString AppConstants::AppCommonSubDir("common");

// Path component where helpers will be searched for
const QString AppConstants::AppHelperSubDir("helpers");

// File extension of common libraries and helpers
const QString AppConstants::AppHelperExtension(".js");

//////////////////////////////////////////////////////////////////

// Memory cache destructors

AppHelperClass::~AppHelperClass () {
    for (QHash<QString,AppHelperObject*>::iterator helperId = this->ids.begin(); helperId != this->ids.end(); helperId++) {
        delete (*helperId);
    }
    this->ids.clear ();
}

AppHelperObjectCache::~AppHelperObjectCache () {
    for (QHash<QString,AppHelperClass*>::iterator helperClass = this->classNames.begin(); helperClass != this->classNames.end(); helperClass++) {
        delete (*helperClass);
    }
    this->classNames.clear ();
}

//////////////////////////////////////////////////////////////////

AppJobRequest::~AppJobRequest () {
}

//////////////////////////////////////////////////////////////////

AppJobRequestFromSquid::~AppJobRequestFromSquid () {
}

AppRequestType AppJobRequestFromSquid::type () {
    return (RequestFromSquid);
}

//////////////////////////////////////////////////////////////////

AppJobRequestFromHelper::~AppJobRequestFromHelper () {
}

AppRequestType AppJobRequestFromHelper::type () {
    return (RequestFromHelper);
}
