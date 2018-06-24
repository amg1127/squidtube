/*
 * squidtube - An external Squid ACL class helper that provides control over access to videos
 * Copyright (C) 2018  Anderson M. Gomes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "appconfig.h"
#include "appruntime.h"

// Rules that dictate valid configuration parameters and their syntaxes go below

QList<AppConfigValidSetting> AppConfig::AppConfigValidSettings = QList<AppConfigValidSetting>()

    << AppConfigValidSetting(QStringLiteral("main"), QStringLiteral("loglevel"), QStringLiteral("Logging level used by the program. One of: DEBUG, INFO, WARNING, ERROR"),
        QRegExp(QStringLiteral("(DEBUG|INFO|WARNING|ERROR)")), AppRuntime::loglevel)

    << AppConfigValidSetting(QStringLiteral("main"), QStringLiteral("helpers"), QStringLiteral("Comma-separated list of helpers that the program will load from the architecture independent data directory."),
        QRegExp(QStringLiteral("\\s*([a-zA-Z_]\\w*(\\s*,\\s*[a-zA-Z_]\\w*)*)?\\s*")), AppRuntime::helpers)

    << AppConfigValidSetting(QStringLiteral("main"), QStringLiteral("positiveTTL"), QStringLiteral("Video information collected from helpers are saved into the application database. This value specifies how many seconds the information is valid for."),
        QRegExp(QStringLiteral("\\d+")), AppRuntime::positiveTTL)

    << AppConfigValidSetting(QStringLiteral("main"), QStringLiteral("negativeTTL"), QStringLiteral("When a failure is registered while collecting video information from helpers, this value specifies how many seconds that failure registry is valid for."),
        QRegExp(QStringLiteral("\\d+")), AppRuntime::negativeTTL)

    << AppConfigValidSetting(QStringLiteral("main"), QStringLiteral("protocol"), QStringLiteral("Sets the protocol compatibility mode to use while retrieving and answering request from Squid. Set '3.4' if using Squid >= 3.4 or '3.0' if using Squid >= 3.0 and < 3.4. Squid 2.x is not supported."),
        QRegExp(QStringLiteral("(3\\.0|3\\.4)")), AppRuntime::squidProtocol)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("driver"), QStringLiteral("SQL driver to use for database access. See documentation for QSqlDatabase::addDatabase() for driver names."),
        QRegExp(QStringLiteral("Q[A-Z]+")), AppRuntime::dbDriver)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("hostname"), QStringLiteral("Hostname that the database driver will connect to."),
        QRegExp(QStringLiteral(".*")), AppRuntime::dbHost)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("port"), QStringLiteral("Port that the database driver will connect to."),
        QRegExp(QStringLiteral("\\d+")), AppRuntime::dbPort)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("username"), QStringLiteral("Username that the database driver will present during the connection."),
        QRegExp(QStringLiteral(".*")), AppRuntime::dbUser)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("password"),  QStringLiteral("Database user's password."),
        QRegExp(QStringLiteral(".*")),AppRuntime::dbPassword)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("options"), QStringLiteral("Database-specific options that the driver will use. See documentation for QSqlDatabase::setConnectionOptions() for details."),
        QRegExp(QStringLiteral(".*")), AppRuntime::dbOptions)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("name"), QStringLiteral("Name of the database that the driver will connect to."),
        QRegExp(QStringLiteral(".*")), AppRuntime::dbName)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("startup"), QStringLiteral("Semicolon-separated list of SQL queries that the database driver will run before issuing normal queries."),
        QRegExp(QStringLiteral(".*")), AppRuntime::dbStartupQuery)

    << AppConfigValidSetting(QStringLiteral("db"), QStringLiteral("tableprefix"), QStringLiteral("Prefix used to name tables managed by the program."),
        QRegExp(QStringLiteral("[a-zA-Z_]\\w*")), AppRuntime::dbTblPrefix);

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
