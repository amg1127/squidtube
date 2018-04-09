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
            QString defaultLogLevel (AppRuntime::loglevel);
            AppRuntime::loglevel = optionsParser.value("main.loglevel").trimmed();
            if (! AppConfig::validateSettings ()) {
                AppRuntime::loglevel = defaultLogLevel;
                return (false);
            }
        }
        // An error may have found while parsing command line arguments.
        // This is not a fatal error yet, because the Squid administrator may have set global variables to helpers.
    }

    // The configuration file must be read before parsing command line options,
    // because they must override settings found in configuration file
    QString configFile(optionsParser.value ("config"));
    qDebug() << QString("Reading configuration options from file '%1'...").arg(configFile);
    QSettings configSettings (configFile, QSettings::IniFormat);
    QSettings::Status configStatus(configSettings.status());
    if (configStatus == QSettings::NoError) {
        QVariant configValue;
        QStringList validConfigSections;
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! validConfigSections.contains (i->configSection)) {
                validConfigSections.append (i->configSection);
            }
            configValue = configSettings.value (i->configSection + "/" + i->configName);
            if (configValue.isValid() && (! configValue.isNull())) {
                // https://stackoverflow.com/a/27558297
                if (configValue.toStringList().count() != 1) {
                    qCritical() << QString("Invalid value '%3' is set for configuration key '%1/%2'. Please, quote the parameter!").arg(configValue.toStringList().join(",").trimmed()).arg(i->configSection).arg(i->configName);
                    return (false);
                }
                *(i->configValue) = configValue.toString().trimmed();
            }
        }
        for (QStringList::const_iterator group = validConfigSections.constBegin(); group != validConfigSections.constEnd(); group++) {
            configSettings.beginGroup (*group);
            QStringList configNames (configSettings.childKeys ());
            for (QStringList::const_iterator configName = configNames.constBegin(); configName != configNames.constEnd(); configName++) {
                bool configNameIsValid = false;
                for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
                    if (i->configSection == (*group) && i->configName == (*configName)) {
                        configNameIsValid = true;
                        break;
                    }
                }
                if (! configNameIsValid) {
                    qWarning() << QString("Invalid configuration key '%1/%2' found in the configuration file!").arg(*group).arg(*configName);
                }
            }
            configSettings.endGroup ();
        }
    } else {
        qWarning() << QString("Error #%1 while reading configuration file '%2'!").arg(configStatus).arg(configFile);
    }

    // Override definitions specified in command line
    qDebug() << "Overriding configuration options specified in command line...";
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        if (optionsParser.isSet (i->configSection + "." + i->configName)) {
            *(i->configValue) = optionsParser.value(i->configSection + "." + i->configName).trimmed();
        }
    }

    if (! AppConfig::validateSettings ()) {
        return (false);
    }

    // Now figure what helpers are enabled
    QString configurationDir (QFileInfo(configFile).path());
    AppRuntime::helperNames = AppRuntime::helpers.split(",", QString::SkipEmptyParts);
    AppRuntime::helperNames.removeDuplicates();
    qDebug() << "Detecting active helpers... Preliminary list is:" << AppRuntime::helperNames;
    QStringList unknownOptionNames(optionsParser.unknownOptionNames());
    // QStringList addOptionNames;
    QHash<QString,QString> globalVariables;
    for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helper != AppRuntime::helperNames.end(); helper++) {
        (*helper) = helper->trimmed ();
        if (helper->isEmpty()) {
            qCritical() << QString("Helper names can not be empty!");
            return (false);
        }
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! i->configSection.compare (*helper, Qt::CaseInsensitive)) {
                qCritical() << QString("Invalid helper specification: '%1'!").arg(*helper);
                return (false);
            }
        }
        if (! (QFile::exists (QString(APP_install_share_dir) + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension) || QFile::exists (configurationDir + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension))) {
            qCritical() << QString("Helper name '%1' is not defined!").arg(*helper);
            return (false);
        }
        // Get helper variables from configuration file
        configSettings.beginGroup (*helper);
        QStringList globalVariableNames (configSettings.childKeys ());
        for (QStringList::const_iterator globalVariable = globalVariableNames.constBegin (); globalVariable != globalVariableNames.constEnd (); globalVariable++) {
            QVariant configValue (configSettings.value(*globalVariable));
            // https://stackoverflow.com/a/27558297
            if (configValue.toStringList().count() != 1) {
                qCritical() << QString("Invalid value '%3' is set for configuration key '%1/%2'. Please, quote the parameter!").arg(*helper).arg(*globalVariable).arg(configValue.toStringList().join(",").trimmed());
                return (false);
            }
            globalVariables[(*helper) + "." + (*globalVariable)] = configValue.toString().trimmed();
        }
        configSettings.endGroup ();
        for (QStringList::const_iterator optionName = unknownOptionNames.constBegin(); optionName != unknownOptionNames.constEnd(); optionName++) {
            if (optionName->startsWith ((*helper) + ".")) {
                globalVariables.insert ((*optionName), QString());
            }
        }
    }

    // Parse command line arguments again, considering variables set for helpers
    // This time, parse errors are fatal
    qDebug() << "Parsing command line options again...";
    for (QHash<QString,QString>::const_iterator globalVariable = globalVariables.constBegin(); globalVariable != globalVariables.constEnd(); globalVariable++) {
        optionsParser.addOption (QCommandLineOption (globalVariable.key(), QString("Custom value for a global variable '%1'").arg(globalVariable.key()), (globalVariable.key()), (globalVariable.key())));
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

    QStringList foundOptionNames (optionsParser.optionNames ());
    for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helper != AppRuntime::helperNames.end(); helper++) {
        for (QStringList::const_iterator optionName = foundOptionNames.constBegin(); optionName != foundOptionNames.constEnd(); optionName++) {
            if ((*optionName).startsWith ((*helper) + ".")) {
                globalVariables[(*optionName)] = optionsParser.value (*optionName).trimmed();
            }
        }
        QHash<QString,QString>::iterator helperCode (AppRuntime::helperSourcesByName.insert ((*helper), ""));
        for (QHash<QString,QString>::const_iterator variable = globalVariables.constBegin(); variable != globalVariables.constEnd(); variable++) {
            if (variable.key().startsWith ((*helper) + ".")) {
                helperCode.value() += "var " + variable.key().mid(helper->length() + 1) + " = unescape ('" + QString::fromUtf8(QUrl::toPercentEncoding (variable.value())) + "');\n";
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
    AppRuntime::positiveTTLint = AppRuntime::positiveTTL.toLongLong (Q_NULLPTR, 10);
    AppRuntime::negativeTTLint = AppRuntime::negativeTTL.toLongLong (Q_NULLPTR, 10);

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
        QString libraryFileContents (AppRuntime::readFileContents (library.value()));
        if (libraryFileContents.isNull ()) {
            qCritical() << QString("Unable to read library file '%1'!").arg(library.value());
            librariesLoaded = false;
        } else {
            AppRuntime::commonSources.insert (library.key(), libraryFileContents);
        }
    }
    if (! librariesLoaded) {
        return (false);
    }

    // Load helpers into the memory
    qDebug() << "Loading helper contents into memory...";
    QString helperJailTemplate (AppRuntime::readFileContents (":/helperjail.js"));
    if (helperJailTemplate.isNull ()) {
        qFatal("Unable to read resource file ':/helperjail.js'!");
    } else {
        QString helperContents;
        QRegExp helperJailPlaceHolder ("/\\*\\s*Helper\\s+code\\s+goes\\s+here\\W", Qt::CaseInsensitive);
        if (! helperJailPlaceHolder.isValid ()) {
            qFatal(QString("'helperJailPlaceHolder' regular expression did not build: '%1'!").arg(helperJailPlaceHolder.errorString()).toLocal8Bit().constData());
        }
        int helperPlaceHolderBegin = helperJailTemplate.indexOf (helperJailPlaceHolder);
        if (helperPlaceHolderBegin >= 0) {
            QString helperJailTemplateBegin (helperJailTemplate.left (helperPlaceHolderBegin));
            helperJailTemplate.remove (0, helperPlaceHolderBegin);
            // The variable 'AppRuntime::helperSourcesStartLine' will help administrator to figure
            // the correct line of any exceptions or syntax errors generated by helpers.
            int numNewLines = helperJailTemplateBegin.count('\n');
            int numCarriageReturns = helperJailTemplateBegin.count('\r');
            if (numNewLines > numCarriageReturns) {
                AppRuntime::helperSourcesStartLine = -2 - numNewLines;
            } else {
                AppRuntime::helperSourcesStartLine = -2 - numCarriageReturns;
            }
            int helperPlaceHolderEnd = helperJailTemplate.indexOf ("*/");
            if (helperPlaceHolderEnd >= 0) {
                helperJailTemplate.remove (0, helperPlaceHolderEnd + 2);
                bool helpersLoaded = true;
                for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helpersLoaded && helper != AppRuntime::helperNames.end(); helper++) {
                    QFile helperFile (configurationDir + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension);
                    if (! helperFile.exists ()) {
                        helperFile.setFileName (QString(APP_install_share_dir) + "/" + AppConstants::AppHelperSubDir + "/" + (*helper) + AppConstants::AppHelperExtension);
                    }
                    helperContents = AppRuntime::readFileContents (helperFile);
                    if (helperContents.isNull ()) {
                        qCritical() << QString("Unable to read helper file '%1'!").arg(helperFile.fileName());
                        helpersLoaded = false;
                    } else {
                        AppRuntime::helperSourcesByName[(*helper)] = helperJailTemplateBegin + "\n" + AppRuntime::helperSourcesByName[(*helper)] + "\n" + helperContents + helperJailTemplate;
                        AppRuntime::helperMemoryCache.append (new AppHelperObjectCache());
                        qDebug() << QString("Loaded helper '%1' from script file '%2'.").arg(*helper).arg(helperFile.fileName());
                    }
                }
                if (! helpersLoaded) {
                    return (false);
                }
            } else {
                qFatal("'helperPlaceHolderEnd' search did not succeed!");
            }
        } else {
            qFatal("'helperPlaceHolderBegin' search did not succeed!");
        }
    }

    return (true);
}

void unloadRuntimeVariables () {
    for (QList<AppHelperObjectCache*>::iterator helperMemoryCache = AppRuntime::helperMemoryCache.begin(); helperMemoryCache != AppRuntime::helperMemoryCache.end(); helperMemoryCache++) {
        delete (*helperMemoryCache);
    }
    AppRuntime::helperMemoryCache.clear ();
    AppRuntime::helperSourcesByName.clear ();
    AppRuntime::helperNames.clear ();
    AppRuntime::commonSources.clear ();
    AppRuntime::dbStartupQueries.clear ();
}
