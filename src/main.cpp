#include "appruntime.h"
#include "appconfig.h"
#include "jobdispatcher.h"

#include <iostream>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkProxyFactory>
#include <QSettings>
#include <QtDebug>

#ifdef Q_OS_LINUX
#include <cstring>
#include <errno.h>
#include <sys/resource.h>
#endif

void messageHandlerFunction (QtMsgType, const QMessageLogContext&, const QString&);
bool loadRuntimeVariables ();
void increaseMemoryLimit ();

int main(int argc, char *argv[]) {
    qInstallMessageHandler (messageHandlerFunction);

    QCoreApplication::setApplicationName(APP_project_name);
    QCoreApplication::setApplicationVersion(APP_project_version);
    QCoreApplication::setOrganizationName(APP_owner_name);

    QCoreApplication app(argc, argv);
    qDebug() << "Starting...";

    // Manually register some QtMetaTypes, so I can queue variables of those types under QObject::connect
    qRegisterMetaType<Qt::CaseSensitivity>("Qt::CaseSensitivity");
    qRegisterMetaType<QRegExp::PatternSyntax>("QRegExp::PatternSyntax");
    qRegisterMetaType<AppSquidRequest>("AppSquidRequest");

    // Make sure that default program settings are valid
    // The validation code builds only if debug build is requested during compilation
#ifndef QT_NO_DEBUG
    if (! AppConfig::validateSettings ()) {
        qFatal("Application default settings is invalid!");
    }
#endif

    if (! loadRuntimeVariables ()) {
        return (1);
    }

    increaseMemoryLimit ();

    // Initialize networking environment
    QNetworkProxyFactory::setUseSystemConfiguration (true);

    // Initialize the dispatcher object
    JobDispatcher jobDispatcher;
    QObject::connect (&jobDispatcher, &JobDispatcher::finished, &app, &QCoreApplication::quit, Qt::QueuedConnection);
    jobDispatcher.start ();

    qDebug() << "Startup finished. Entering main loop...";
    return (app.exec ());
}

#ifdef QT_NO_DEBUG
void messageHandlerFunction (QtMsgType type, const QMessageLogContext&, const QString& msg) {
#else
void messageHandlerFunction (QtMsgType type, const QMessageLogContext& context, const QString& msg) {
#endif
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QString prefix;
    switch (type) {
    case QtFatalMsg:
        prefix = "FATAL" ; break;
    case QtCriticalMsg:
        prefix = "CRITICAL" ; break;
    case QtWarningMsg:
        prefix = "WARNING" ; break;
    case QtInfoMsg:
        prefix = "INFO" ; break;
    case QtDebugMsg:
        prefix = "DEBUG" ; break;
    default:
        prefix = "UNKNOWN";
    }
    QString possibleLevels ("DIWCFU");
    if (! (possibleLevels.indexOf (AppRuntime::loglevel[0]) > possibleLevels.indexOf (prefix[0]))) {
#ifdef QT_NO_DEBUG
        QString msgLineContext ("");
#else
        QString msgLineContext (QString(" (%1:%2%3)").arg(context.file).arg(context.line).arg((context.function[0]) ? (QString(", %1").arg(context.function)) : ""));
#endif
        QStringList msgLines (msg.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts));
        for (QStringList::const_iterator msgLine = msgLines.constBegin(); msgLine != msgLines.constEnd(); msgLine++) {
            QString msgTransform ((*msgLine).trimmed ());
            if (msgTransform.left(1) == "\"")
                msgTransform.remove (0, 1);
            if (msgTransform.right(1) == "\"")
                msgTransform.chop(1);
            QString msgLineFormatted (QString("[%1] 0x%2 %3: %4").arg(AppRuntime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")).arg((ulong) QThread::currentThread(), 16, 16, QChar('0')).arg(prefix).arg(msgTransform.trimmed() + msgLineContext));
            std::cerr << msgLineFormatted.toLocal8Bit().constData() << std::endl;
        }
    }
    if (type == QtFatalMsg) {
        abort ();
    }
}

void increaseMemoryLimit () {
#ifdef Q_OS_LINUX
    // Tries to use the maximum memory allowed by the administrator
    qDebug() << "Increasing memory limit...";
    struct rlimit rlp;
    int ret = getrlimit(RLIMIT_AS, &rlp);
    if (ret) {
        qWarning() << QString("'getrlimit' call failed: %1: %2").arg(errno).arg(strerror(errno));
    } else {
        rlp.rlim_cur = 3 << 30;
        if (rlp.rlim_cur < rlp.rlim_max || rlp.rlim_max == RLIM_INFINITY) {
            ret = setrlimit (RLIMIT_AS, &rlp);
            if (ret) {
                qWarning() << QString("'setrlimit' call failed: %1: %2").arg(errno).arg(strerror(errno));
            }
        }
    }
#endif
}

bool loadRuntimeVariables () {
    // Parse command line options
    qDebug() << "Parsing command line options...";
    QStringList arguments(QCoreApplication::arguments());
    QCommandLineParser optionsParser;
    optionsParser.setApplicationDescription ("An external ACL helper that enables control over accesses to videos.");
    optionsParser.addOption (QCommandLineOption ("config", "Path of the program configuration file.", "config", QString("%1/%2.conf").arg(APP_install_etc_dir).arg(APP_project_name)));
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        optionsParser.addOption (QCommandLineOption (i->configSection + "." + i->configName, i->configDescription, i->configName, *(i->configValue)));
    }
    const QCommandLineOption helpOption = optionsParser.addHelpOption ();
    const QCommandLineOption versionOption = optionsParser.addVersionOption ();
    if (optionsParser.parse (arguments)) {
        if (optionsParser.isSet (helpOption)) {
            optionsParser.showHelp (); return (false);
        } else if (optionsParser.isSet (versionOption)) {
            optionsParser.showVersion (); return (false);
        } else if (! optionsParser.positionalArguments().isEmpty ()) {
            qCritical() << "This program does not accept positional arguments!";
            optionsParser.showHelp (1); return (false);
        } else if (optionsParser.isSet ("main.loglevel")) {
            // Apply loglevel option in advance if it is specified
            AppRuntime::loglevel = optionsParser.value("main.loglevel").trimmed();
            if (! AppConfig::validateSettings ()) {
                return (false);
            }
        }
        // An error may have found while parsing command line arguments.
        // This is not a fatal error yet, because the Squid administrator may have set global variables to helpers.
    }

    // The configuration file must be read before parsing command line options,
    // because they must override settings found in configuration file
    QString configFile(optionsParser.value ("config"));
    qDebug() << QString("Reading config options from file '%1'...").arg(configFile);
    QSettings configSettings (configFile, QSettings::IniFormat);
    QSettings::Status configStatus(configSettings.status());
    if (configStatus == QSettings::NoError) {
        QVariant configValue;
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            configValue = configSettings.value (i->configSection + "/" + i->configName);
            if (configValue.isValid() && (! configValue.isNull())) {
                // https://stackoverflow.com/a/27558297
                if (configValue.toStringList().count() != 1) {
                    qCritical() << QString("Invalid value '%3' is set for config key '%1/%2'. Please, quote the parameter!").arg(configValue.toStringList().join(",").trimmed()).arg(i->configSection).arg(i->configName);
                    return (false);
                }
                *(i->configValue) = configValue.toString().trimmed();
            }
        }
    } else {
        qWarning() << QString("Error #%1 while reading configuration file '%2'!").arg(configStatus).arg(configFile);
    }

    // Override definitions specified in command line
    qDebug() << "Overriding config options specified in command line...";
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        if (optionsParser.isSet (i->configSection + "." + i->configName)) {
            *(i->configValue) = optionsParser.value(i->configSection + "." + i->configName).trimmed();
        }
    }

    if (! AppConfig::validateSettings ()) {
        return (false);
    }

    // Now figure what helpers are enabled
    qDebug() << "Detecting active helpers...";
    QStringList helpersList = AppRuntime::helpers.split(",", QString::SkipEmptyParts);
    helpersList.removeDuplicates();
    QStringList unknownOptionNames(optionsParser.unknownOptionNames());
    QStringList addOptionNames;
    QHash<QString,QString> globalVariables;
    for (QStringList::iterator helper = helpersList.begin(); helper != helpersList.end(); helper++) {
        (*helper) = (*helper).trimmed ();
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! i->configSection.compare (*helper, Qt::CaseInsensitive)) {
                qCritical() << QString("Invalid helper specification: '%1'!").arg(*helper);
                return (false);
            }
        }
        AppHelper objHelper;
        objHelper.name = (*helper);
        // Get helper variables from configuration file
        configSettings.beginGroup (objHelper.name);
        QStringList globalVariableNames = configSettings.childKeys ();
        for (QStringList::const_iterator globalVariable = globalVariableNames.constBegin (); globalVariable != globalVariableNames.constEnd (); globalVariable++) {
            QVariant configValue (configSettings.value(*globalVariable));
            // https://stackoverflow.com/a/27558297
            if (configValue.toStringList().count() != 1) {
                qCritical() << QString("Invalid value '%3' is set for config key '%1/%2'. Please, quote the parameter!").arg(*helper).arg(*globalVariable).arg(configValue.toStringList().join(",").trimmed());
                return (false);
            }
            globalVariables[objHelper.name + "." + (*globalVariable)] = configValue.toString().trimmed();
        }
        configSettings.endGroup ();
        for (QStringList::const_iterator optionName = unknownOptionNames.constBegin(); optionName != unknownOptionNames.constEnd(); optionName++) {
            if ((*optionName).startsWith (objHelper.name + ".")) {
                globalVariables.insert ((*optionName), QString());
                addOptionNames << (*optionName);
            }
        }
        AppRuntime::helperObjects << objHelper;
    }

    // Parse command line arguments again, considering variables set for helpers
    // This time, parse errors are fatal
    qDebug() << "Parsing command line options again...";
    for (QStringList::const_iterator optionName = addOptionNames.constBegin(); optionName != addOptionNames.constEnd(); optionName++) {
        optionsParser.addOption (QCommandLineOption ((*optionName), QString("Value for a global variable '%1'").arg(*optionName), (*optionName), (*optionName)));
    }
    if (optionsParser.parse (arguments)) {
        if (optionsParser.isSet (helpOption)) {
            optionsParser.showHelp (); return (false);
        } else if (optionsParser.isSet (versionOption)) {
            optionsParser.showVersion (); return (false);
        } else if (! optionsParser.positionalArguments().isEmpty ()) {
            qCritical() << "This program does not accept positional arguments!";
            optionsParser.showHelp (1); return (false);
        }
    } else {
        optionsParser.showHelp (); return (false);
    }
    addOptionNames = optionsParser.optionNames ();
    for (QList<AppHelper>::iterator objHelper = AppRuntime::helperObjects.begin(); objHelper != AppRuntime::helperObjects.end(); objHelper++) {
        for (QStringList::const_iterator optionName = addOptionNames.constBegin(); optionName != addOptionNames.constEnd(); optionName++) {
            if ((*optionName).startsWith (objHelper->name + ".")) {
                globalVariables[(*optionName)] = optionsParser.value (*optionName).trimmed();
            }
        }
        objHelper->code = "";
        for (QHash<QString,QString>::const_iterator variable = globalVariables.constBegin(); variable != globalVariables.constEnd(); variable++) {
            if (variable.key().startsWith (objHelper->name + ".")) {
                objHelper->code += "var " + variable.key().mid(objHelper->name.length() + 1) + " = unescape ('" + QString::fromUtf8(QUrl::toPercentEncoding (variable.value())) + "');\n";
            }
        }
    }

    // This will be used by the databaseBridge object
    AppRuntime::dbStartupQueries = AppRuntime::dbStartupQuery.split(";");

    // Load helpers into the memory
    qDebug() << "Loading helper contents into memory...";
    bool helpersLoaded = true;
    for (QList<AppHelper>::iterator objHelper = AppRuntime::helperObjects.begin(); helpersLoaded && objHelper != AppRuntime::helperObjects.end(); objHelper++) {
        QFile helperFile (QString(APP_install_share_dir) + "/" + objHelper->name + AppHelper::AppHelperExtension);
        if (helperFile.open (QIODevice::ReadOnly | QIODevice::Text)) {
            {
                QTextStream helperFileStream (&helperFile);
                objHelper->code = AppHelper::AppHelperCodeHeader + objHelper->code + "\n" + helperFileStream.readAll() + AppHelper::AppHelperCodeFooter;
            }
            if (helperFile.error() != QFileDevice::NoError) {
                qCritical() << QString("Error reading contents of file '%1': '%2'!").arg(helperFile.fileName()).arg(helperFile.errorString());
                helpersLoaded = false;
            }
            if (! helperFile.atEnd ()) {
                qWarning() << QString("The file '%1' was not read completely!").arg(helperFile.fileName());
                helpersLoaded = false;
            }
            helperFile.close ();
        } else {
            qCritical() << QString("Unable to open file '%1' for reading: '%2'!").arg(helperFile.fileName()).arg(helperFile.errorString());
            helpersLoaded = false;
        }
    }
    if (! helpersLoaded) {
        return (false);
    }

    return (true);
}
