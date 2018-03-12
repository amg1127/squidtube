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
    inline AppConfigValidSetting(const QString configSection, const QString configName, const QString configDescription, const QRegExp configSyntax, QString& configValue) :
        configSection(configSection),
        configName(configName),
        configDescription(configDescription),
        configSyntax(configSyntax),
        configValue(&configValue) { }
};

class AppConfig
{
public:
    static QList<AppConfigValidSetting> AppConfigValidSettings;
    static bool validateSettings ();
};

#endif // APPCONFIG_H
