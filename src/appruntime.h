#ifndef APPRUNTIME_H
#define APPRUNTIME_H

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QRegExp>
#include <QSemaphore>
#include <QString>
#include <QUrl>

class AppHelper {
public:
    static const QString AppHelperSubDir;
    static const QString AppCommonSubDir;
    static const QString AppHelperExtension;
    static const QString AppHelperCodeHeader;
    static const QString AppHelperCodeFooter;
};

class AppSquidRequest {
public:
    QUrl url;
    QString property;
    Qt::CaseSensitivity caseSensivity;
    QRegExp::PatternSyntax patternSyntax;
    QStringList criteria;
    qint64 timestamp;
    QString helperName;
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
    static QHash<QString,QString> helperSources;
    static QHash<QString,QString> commonSources;
    static QMutex sourcesMutex;
    // Database information
    static int dbInstance;
    static QStringList dbStartupQueries;
    static QMutex dbSettingsMutex;
    // A static method for datetime retrieval, because the native class is reentrant
    static QDateTime currentDateTime();
};

#endif // APPRUNTIME_H
