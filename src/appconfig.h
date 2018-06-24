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

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QList>
#include <QString>

class AppConfigValidSetting {
public:
    QString configSection;
    QString configName;
    QString configDescription;
    QRegExp configSyntax;
    QString* configValue;
    inline AppConfigValidSetting (const QString _configSection, const QString _configName, const QString _configDescription, const QRegExp _configSyntax, QString& _configValue) :
        configSection(_configSection),
        configName(_configName),
        configDescription(_configDescription),
        configSyntax(_configSyntax),
        configValue(&_configValue) { }
};

class AppConfig
{
public:
    static QList<AppConfigValidSetting> AppConfigValidSettings;
    static bool validateSettings ();
};

#endif // APPCONFIG_H
