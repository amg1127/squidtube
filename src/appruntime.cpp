#include "appruntime.h"

// Default application settings go below

#ifdef QT_NO_DEBUG
QString AppRuntime::loglevel("INFO");
#else
QString AppRuntime::loglevel("DEBUG");
#endif
QString AppRuntime::helpers("");
QString AppRuntime::registryTTL("2592000");
QString AppRuntime::dbDriver("QSQLITE");
QString AppRuntime::dbHost("");
QString AppRuntime::dbPort("0");
QString AppRuntime::dbUser("");
QString AppRuntime::dbPassword("");
QString AppRuntime::dbOptions("");
QString AppRuntime::dbName("");
QString AppRuntime::dbStartupQuery("");

// Default application settings go above

QList<AppHelper> AppRuntime::helperObjects;

QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;
int AppRuntime::dbInstance = 0;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}
