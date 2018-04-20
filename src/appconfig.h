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
