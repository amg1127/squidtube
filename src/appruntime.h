#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QSemaphore>
#include <QString>

class AppHelper {
public:
    QString name;
    QString code;
    static const QString AppHelperCodeHeader;
    static const QString AppHelperCodeFooter;
    static const QString AppHelperExtension;
};

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

    // Runtime configuration
    // Helpers information
    static QList<AppHelper> helperObjects;
    // Database information
    static int dbInstance;
    static QStringList dbStartupQueries;
    static QMutex dbSettingsMutex;
    // A static method for datetime retrieval, because the native class is reentrant
    static QDateTime currentDateTime();
};

#endif // APPRUNTIME_H
