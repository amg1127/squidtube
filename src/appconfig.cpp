#include "appconfig.h"
#include "appruntime.h"

#include <QtDebug>

// Rules that dictate valid configuration parameters and their syntaxes go below

QList<AppConfigValidSetting> AppConfig::AppConfigValidSettings = QList<AppConfigValidSetting>()

    << AppConfigValidSetting("main", "loglevel", "Logging level used by the program. One of: DEBUG, INFO, WARNING, CRITICAL",
        QRegExp("(CRITICAL|WARNING|INFO|DEBUG)"), AppRuntime::loglevel)

    << AppConfigValidSetting("main", "helpers", "Comma-separated list of helpers that the program will load from the architecture independent data directory.",
        QRegExp("([\\w-_]+(\\s*,\\s*[\\w-_]+)*)?"), AppRuntime::helpers)

    << AppConfigValidSetting("main", "registryTTL", "Video information collected from helpers are saved into the application database. This value specifies how many seconds the information is valid for.",
        QRegExp("\\d+"), AppRuntime::registryTTL)

    << AppConfigValidSetting("db", "driver", "SQL driver to use for database access. See documentation for QSqlDatabase::addDatabase() for driver names.",
        QRegExp("Q[A-Z]+"), AppRuntime::dbDriver)

    << AppConfigValidSetting("db", "hostname", "Hostname that the database driver will connect to.",
        QRegExp(".*"), AppRuntime::dbHost)

    << AppConfigValidSetting("db", "port", "Port that the database driver will connect to.",
        QRegExp("\\d+"), AppRuntime::dbPort)

    << AppConfigValidSetting("db", "username", "Username that the database driver will present during the connection.",
        QRegExp(".*"), AppRuntime::dbUser)

    << AppConfigValidSetting("db", "password",  "Database user's password.",
        QRegExp(".*"),AppRuntime::dbPassword)

    << AppConfigValidSetting("db", "options", "Database-specific options that the driver will use. See documentation for QSqlDatabase::setConnectionOptions() for details.",
        QRegExp(".*"), AppRuntime::dbOptions)

    << AppConfigValidSetting("db", "name", "Name of the database that the driver will connect to.",
        QRegExp(".*"), AppRuntime::dbName)

    << AppConfigValidSetting("db", "startup", "Semicolon-separated list of SQL queries that the database driver will run before issuing normal queries.",
        QRegExp("([^;]+;)*"), AppRuntime::dbStartupQuery);

// Rules that dictate valid configuration parameters and their syntaxes go above

bool AppConfig::validateSettings () {
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        if (i->configSyntax.isValid()) {
            if (i->configSyntax.exactMatch(*(i->configValue))) {
                continue;
            } else {
                qWarning() << QString("Invalid value '%3' is set for config key '%1/%2'.").arg(i->configSection).arg(i->configName).arg(*(i->configValue));
            }
        } else {
            qCritical() << QString("Syntax check validator for config key '%1/%2' does not work!").arg(i->configSection).arg(i->configName);
        }
        return (false);
    }
    return (true);
}
