#include "appruntime.h"
#include "appconfig.h"
#include "jobdispatcher.h"

#include <iostream>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLinkedList>
#include <QMutex>
#include <QMutexLocker>
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
void unloadRuntimeVariables ();
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

    int returnValue = 1;

    if (loadRuntimeVariables ()) {
        increaseMemoryLimit ();

        // Initialize networking environment
        QNetworkProxyFactory::setUseSystemConfiguration (true);

        // Initialize the dispatcher object
        JobDispatcher jobDispatcher;
        QObject::connect (&jobDispatcher, &JobDispatcher::finished, &app, &QCoreApplication::quit, Qt::QueuedConnection);
        jobDispatcher.start ();

        qDebug() << "Startup finished. Entering main loop...";
        returnValue = app.exec ();
    }

    qDebug() << "Performing final cleaning and finishing program...";
    unloadRuntimeVariables ();
    return (returnValue);
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
        prefix = "ERROR" ; break;
    case QtWarningMsg:
        prefix = "WARNING" ; break;
    case QtInfoMsg:
        prefix = "INFO" ; break;
    case QtDebugMsg:
        prefix = "DEBUG" ; break;
    default:
        prefix = "UNKNOWN";
    }
    static QString possibleLevels ("DIWEFU");
    if (! (possibleLevels.indexOf (AppRuntime::loglevel[0]) > possibleLevels.indexOf (prefix[0]))) {
#ifdef QT_NO_DEBUG
        QString msgLineContext ("");
        QString dateTimeFormat ("yyyy-MM-dd'T'HH:mm:sst");
#else
        QString msgLineContext (QString(" (%1:%2%3)").arg((context.file) ? ((context.file[0]) ? context.file : "(unknown)") : "(unknown)").arg((context.line) ? context.line : 0).arg((context.function) ? ((context.function[0]) ? (QString(", %1").arg(context.function)) : "") : ""));
        QString dateTimeFormat ("yyyy-MM-dd'T'HH:mm:ss.zzzt");
#endif
        QStringList msgLines (msg.split(QRegExp("[\\r\\n]+"), QString::SkipEmptyParts));
        for (QStringList::const_iterator msgLine = msgLines.constBegin(); msgLine != msgLines.constEnd(); msgLine++) {
            QString msgTransform (*msgLine);
            while ((! msgTransform.isEmpty ()) && msgTransform.right(1).at(0).isSpace()) {
                msgTransform.chop(1);
            }
            if (msgTransform.left(1) == "\"")
                msgTransform.remove (0, 1);
            if (msgTransform.right(1) == "\"")
                msgTransform.chop(1);
            QString msgLineFormatted (QString("[%1] 0x%2 %3: %4").arg(AppRuntime::currentDateTime().toString(dateTimeFormat)).arg((ulong) QThread::currentThreadId(), 16, 16, QChar('0')).arg(prefix).arg(msgTransform.trimmed() + msgLineContext));
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
    optionsParser.setApplicationDescription ("An external Squid ACL class helper that provides control over access to videos (through my own helpers).");
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
        (*helper) = helper->trimmed ();
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! i->configSection.compare (*helper, Qt::CaseInsensitive)) {
                qCritical() << QString("Invalid helper specification: '%1'!").arg(*helper);
                return (false);
            }
        }
        // Get helper variables from configuration file
        configSettings.beginGroup (*helper);
        QStringList globalVariableNames = configSettings.childKeys ();
        for (QStringList::const_iterator globalVariable = globalVariableNames.constBegin (); globalVariable != globalVariableNames.constEnd (); globalVariable++) {
            QVariant configValue (configSettings.value(*globalVariable));
            // https://stackoverflow.com/a/27558297
            if (configValue.toStringList().count() != 1) {
                qCritical() << QString("Invalid value '%3' is set for config key '%1/%2'. Please, quote the parameter!").arg(*helper).arg(*globalVariable).arg(configValue.toStringList().join(",").trimmed());
                return (false);
            }
            globalVariables[(*helper) + "." + (*globalVariable)] = configValue.toString().trimmed();
        }
        configSettings.endGroup ();
        for (QStringList::const_iterator optionName = unknownOptionNames.constBegin(); optionName != unknownOptionNames.constEnd(); optionName++) {
            if (optionName->startsWith ((*helper) + ".")) {
                globalVariables.insert ((*optionName), QString());
                addOptionNames << (*optionName);
            }
        }
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

    for (QStringList::iterator helper = helpersList.begin(); helper != helpersList.end(); helper++) {
        for (QStringList::const_iterator optionName = addOptionNames.constBegin(); optionName != addOptionNames.constEnd(); optionName++) {
            if ((*optionName).startsWith ((*helper) + ".")) {
                globalVariables[(*optionName)] = optionsParser.value (*optionName).trimmed();
            }
        }
        QHash<QString,QString>::iterator helperCode (AppRuntime::helperSources.insert ((*helper), ""));
        for (QHash<QString,QString>::const_iterator variable = globalVariables.constBegin(); variable != globalVariables.constEnd(); variable++) {
            if (variable.key().startsWith ((*helper) + ".")) {
                helperCode.value() += "let " + variable.key().mid(helper->length() + 1) + " = unescape ('" + QString::fromUtf8(QUrl::toPercentEncoding (variable.value())) + "');\n";
            }
        }
    }

    // This will be used by the DatabaseBridge object
    AppRuntime::dbStartupQueries = AppRuntime::dbStartupQuery.split(QRegExp("\\s*;\\s*"), QString::SkipEmptyParts);
    int length = AppRuntime::dbStartupQueries.count();
    for (int index = 0; index < length; index++) {
        AppRuntime::dbStartupQueries[index] = AppRuntime::dbStartupQueries[index].trimmed ();
        if (AppRuntime::dbStartupQueries[index].isEmpty()) {
            AppRuntime::dbStartupQueries.removeAt (index);
            index--;
            length--;
        }
    }

    // This will be used by all JobWorker objects
    AppRuntime::registryTTLint = AppRuntime::registryTTL.toLongLong (Q_NULLPTR, 10);

    // Load common libraries into the memory
    qDebug() << "Loading common library contents into memory...";
    bool librariesLoaded = true;
    QLinkedList<QFileInfo> libraries;

    // Search libraries from the architecture-independent data directory first
    QFileInfo libraryRoot (QString(APP_install_share_dir) + "/" + AppConstants::AppCommonSubDir);
    int libraryRootPathLength (libraryRoot.filePath().length());
    libraries.prepend (libraryRoot);
    QHash<QString,QString> libraryCandidates;
    while (! libraries.isEmpty ()) {
        QFileInfo library (libraries.takeFirst ());
        if (library.isDir()) {
            if (library.fileName() != "." && library.fileName() != "..") {
                QDir libraryDir (library.filePath ());
                QFileInfoList libraryDirContents (libraryDir.entryInfoList ());
                for (QFileInfoList::const_iterator libraryFile = libraryDirContents.constBegin(); libraryFile != libraryDirContents.constEnd(); libraryFile++) {
                    libraries.prepend (*libraryFile);
                }
            }
        } else if (library.isFile() && library.fileName().endsWith(AppConstants::AppHelperExtension)) {
            QString libraryName (library.filePath().mid(libraryRootPathLength + 1));
            libraryName.chop (AppConstants::AppHelperExtension.length());
            libraryCandidates.insert (libraryName, library.filePath ());
        }
    }
    // Figure which libraries were overriden by administrator
    QString configurationDir (QFileInfo(configFile).path());
    libraryRoot.setFile (configurationDir + "/" + AppConstants::AppCommonSubDir);
    libraryRootPathLength = libraryRoot.filePath().length();
    libraries.prepend (libraryRoot);
    while (! libraries.isEmpty ()) {
        QFileInfo library (libraries.takeFirst ());
        if (library.isDir()) {
            if (library.fileName() != "." && library.fileName() != "..") {
                QDir libraryDir (library.filePath ());
                QFileInfoList libraryDirContents (libraryDir.entryInfoList ());
                for (QFileInfoList::const_iterator libraryFile = libraryDirContents.constBegin(); libraryFile != libraryDirContents.constEnd(); libraryFile++) {
                    libraries.prepend (*libraryFile);
                }
            }
        } else if (library.isFile() && library.fileName().endsWith(AppConstants::AppHelperExtension)) {
            QString libraryName (library.filePath().mid(libraryRootPathLength + 1));
            libraryName.chop (AppConstants::AppHelperExtension.length());
            libraryCandidates.insert (libraryName, library.filePath ());
        }
    }
    // Now load library contents
    for (QHash<QString,QString>::const_iterator library = libraryCandidates.constBegin(); librariesLoaded && library != libraryCandidates.constEnd(); library++) {
        QFile libraryFile (library.value ());
        if (libraryFile.open (QIODevice::ReadOnly | QIODevice::Text)) {
            {
                QTextStream libraryFileStream (&libraryFile);
                AppRuntime::commonSources.insert (library.key(), libraryFileStream.readAll());
                qDebug() << QString("Loaded library '%1' from script file '%2'.").arg(library.key()).arg(library.value());
            }
            if (libraryFile.error() != QFileDevice::NoError) {
                qCritical() << QString("Error reading contents of file '%1': '%2'!").arg(library.value()).arg(libraryFile.errorString());
                librariesLoaded = false;
            }
            if (! libraryFile.atEnd ()) {
                qWarning() << QString("The file '%1' was not read completely!").arg(library.value());
                librariesLoaded = false;
            }
            libraryFile.close ();
        } else {
            qCritical() << QString("Unable to open file '%1' for reading: '%2'!").arg(library.value()).arg(libraryFile.errorString());
            librariesLoaded = false;
        }
    }
    if (! librariesLoaded) {
        return (false);
    }

    // Load helpers into the memory
    qDebug() << "Loading helper contents into memory...";
    bool helpersLoaded = true;
    for (QStringList::iterator helper = helpersList.begin(); helpersLoaded && helper != helpersList.end(); helper++) {
        QFile helperFile (configurationDir + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension);
        if (! helperFile.exists ()) {
            helperFile.setFileName (QString(APP_install_share_dir) + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension);
        }
        if (helperFile.open (QIODevice::ReadOnly | QIODevice::Text)) {
            {
                QTextStream helperFileStream (&helperFile);
                AppRuntime::helperSources[(*helper)] = AppConstants::AppHelperCodeHeader + AppRuntime::helperSources[(*helper)] + "\n" + helperFileStream.readAll() + AppConstants::AppHelperCodeFooter;
                AppRuntime::helperMemoryCache.insert ((*helper), new AppHelperObjectCache());
                qDebug() << QString("Loaded helper '%1' from script file '%2'.").arg(*helper).arg(helperFile.fileName());
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

void unloadRuntimeVariables () {
    for (QHash<QString,AppHelperObjectCache*>::iterator helperMemoryCache = AppRuntime::helperMemoryCache.begin(); helperMemoryCache != AppRuntime::helperMemoryCache.end(); helperMemoryCache++) {
        delete (*helperMemoryCache);
    }
    AppRuntime::helperMemoryCache.clear ();
    AppRuntime::helperSources.clear ();
    AppRuntime::commonSources.clear ();
    AppRuntime::dbStartupQueries.clear ();
}
