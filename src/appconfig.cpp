#include "appconfig.h"
#include "appruntime.h"

// Rules that dictate valid configuration parameters and their syntaxes go below

QList<AppConfigValidSetting> AppConfig::AppConfigValidSettings = QList<AppConfigValidSetting>()

    << AppConfigValidSetting("main", "loglevel", "Logging level used by the program. One of: DEBUG, INFO, WARNING, ERROR",
        QRegExp("(DEBUG|INFO|WARNING|ERROR)"), AppRuntime::loglevel)

    << AppConfigValidSetting("main", "helpers", "Comma-separated list of helpers that the program will load from the architecture independent data directory.",
        QRegExp("\\s*([a-zA-Z_]\\w*(\\s*,\\s*[a-zA-Z_]\\w*)*)?\\s*"), AppRuntime::helpers)

    << AppConfigValidSetting("main", "positiveTTL", "Video information collected from helpers are saved into the application database. This value specifies how many seconds the information is valid for.",
        QRegExp("\\d+"), AppRuntime::positiveTTL)

    << AppConfigValidSetting("main", "negativeTTL", "When a failure is registered while collecting video information from helpers, this value specifies how many seconds that failure registry is valid for.",
        QRegExp("\\d+"), AppRuntime::negativeTTL)

    << AppConfigValidSetting("main", "protocol", "Sets the protocol compatibility mode to use while retrieving and answering request from Squid. Set '3.4' if using Squid >= 3.4 or '3.0' if using Squid >= 3.0 and < 3.4. Squid 2.x is not supported.",
        QRegExp("(3\\.0|3\\.4)"), AppRuntime::squidProtocol)

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
        QRegExp(".*"), AppRuntime::dbStartupQuery)

    << AppConfigValidSetting("db", "tableprefix", "Prefix used to name tables managed by the program.",
        QRegExp("[a-zA-Z_]\\w*"), AppRuntime::dbTblPrefix);

// Rules that dictate valid configuration parameters and their syntaxes go above

bool AppConfig::validateSettings () {
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        if (i->configSyntax.isValid()) {
            if (i->configSyntax.exactMatch(*(i->configValue))) {
                continue;
            } else {
                qCritical ("Configuration key '%s/%s' is set an invalid value '%s'!", i->configSection.toLatin1().constData(), i->configName.toLatin1().constData(), i->configValue->toLatin1().constData());
            }
        } else {
            qCritical ("Syntax check validator for configuration key '%s/%s' does not work!", i->configSection.toLatin1().constData(), i->configName.toLatin1().constData());
        }
        return (false);
    }
    return (true);
}
