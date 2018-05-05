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

void messageHandlerFunction (QtMsgType, const QMessageLogContext&, const QString&);
bool loadRuntimeVariables ();
void unloadRuntimeVariables ();

int main(int argc, char *argv[]) {
    qInstallMessageHandler (messageHandlerFunction);

    QCoreApplication::setApplicationName(QStringLiteral(APP_project_name));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_project_version));
    QCoreApplication::setOrganizationName(QStringLiteral(APP_owner_name));

    QCoreApplication app(argc, argv);
    qDebug ("Starting ...");

    // Make sure that default program settings are valid
    // The validation code builds only if debug build is requested during compilation
#ifndef QT_NO_DEBUG
    if (! AppConfig::validateSettings ()) {
        qFatal ("Application default settings is invalid!");
    }
#endif

    int returnValue = 1;

    if (loadRuntimeVariables ()) {

        // Initialize networking environment
        QNetworkProxyFactory::setUseSystemConfiguration (true);

        // Initialize the dispatcher object
        JobDispatcher jobDispatcher;
        QObject::connect (&jobDispatcher, &JobDispatcher::finished, &app, &QCoreApplication::quit, Qt::QueuedConnection);
        int tentative = 5;
        for (; tentative > 0; tentative--) {
            if (jobDispatcher.start ()) {
                break;
            }
            qWarning ("StdinReader thread did not start!");
            QThread::sleep (1);
        }
        if (tentative) {
            qDebug ("Startup finished. Entering main loop... (appVer=%s, qtBuild=%s, qtRun=%s)", APP_project_version, QT_VERSION_STR, qVersion());
            returnValue = app.exec ();
        } else {
            qCritical ("Unable to initialize JobDispatcher object correctly. The program can not work without it!");
        }
    }

    qDebug ("Performing final cleaning and finishing program...");
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
    QByteArray prefix;
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
    static QByteArray possibleLevels ("DIWEFU");
    if (! (possibleLevels.indexOf (AppRuntime::loglevel[0].toLatin1()) > possibleLevels.indexOf (prefix[0]))) {
#ifdef QT_NO_DEBUG
        QByteArray msgLineContext ("");
        QString dateTimeFormat (QStringLiteral("yyyy-MM-dd'T'HH:mm:sst"));
#else
        QByteArray threadId (QByteArray::number (reinterpret_cast<qulonglong> (QThread::currentThreadId()), 16));
        for (int length = threadId.length(); length < 16; length++) {
            threadId.prepend ('0');
        }
        prefix.prepend (QByteArray("0x") + threadId + " ");
        QByteArray msgLineContext (" (");
        if (context.file) {
            if (context.file[0]) {
                msgLineContext = msgLineContext + context.file + ":" + QByteArray::number (context.line);
            }
        }
        if (context.function) {
            if (context.function[0]) {
                msgLineContext = msgLineContext + ", " + context.function;
            }
        }
        if (msgLineContext.length() > 2) {
            msgLineContext = msgLineContext + ")";
        } else {
            msgLineContext.clear ();
        }
        QString dateTimeFormat (QStringLiteral("yyyy-MM-dd'T'HH:mm:ss.zzzt"));
#endif
        QStringList msgLines (msg.split(QRegExp(QStringLiteral("[\\r\\n]+")), QString::SkipEmptyParts));
        for (QStringList::const_iterator msgLine = msgLines.constBegin(); msgLine != msgLines.constEnd(); msgLine++) {
            QString msgTransform (*msgLine);
            while ((! msgTransform.isEmpty ()) && msgTransform.right(1).at(0).isSpace()) {
                msgTransform.chop(1);
            }
            QString msgLineFormatted (QStringLiteral("[%1] %2: %3").arg(AppRuntime::currentDateTime().toString(dateTimeFormat), QString::fromUtf8(prefix), msgTransform + QString::fromUtf8 (msgLineContext)));
            std::cerr << msgLineFormatted.toLocal8Bit().constData() << std::endl;
        }
    }
    if (type == QtFatalMsg) {
        abort ();
    }
}

bool loadRuntimeVariables () {
    // Parse command line options
    qDebug ("Parsing command line options...");
    QStringList arguments(QCoreApplication::arguments());
    QCommandLineParser optionsParser;
    optionsParser.setApplicationDescription (QStringLiteral("An external Squid ACL class helper that provides control over access to videos (through own helpers).\nSee more information at: " APP_project_url));
    optionsParser.addOption (QCommandLineOption (QStringLiteral("config"), QStringLiteral("Path of the program configuration file."), QStringLiteral("config"), QStringLiteral(APP_install_etc_dir "/" APP_project_name ".conf")));
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        optionsParser.addOption (QCommandLineOption (i->configSection + QStringLiteral(".") + i->configName, i->configDescription, i->configName, *(i->configValue)));
    }
    const QCommandLineOption helpOption = optionsParser.addHelpOption ();
    const QCommandLineOption versionOption = optionsParser.addVersionOption ();
    if (optionsParser.parse (arguments)) {
        if (optionsParser.isSet (helpOption)) {
            optionsParser.showHelp ();
        } else if (optionsParser.isSet (versionOption)) {
            optionsParser.showVersion ();
        } else if (! optionsParser.positionalArguments().isEmpty ()) {
            qCritical ("This program does not accept positional arguments!");
            optionsParser.showHelp (1);
        } else if (optionsParser.isSet (QStringLiteral("main.loglevel"))) {
            // Apply loglevel option in advance if it is specified
            QString defaultLogLevel (AppRuntime::loglevel);
            AppRuntime::loglevel = optionsParser.value(QStringLiteral("main.loglevel")).trimmed();
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
    QString configFile(optionsParser.value (QStringLiteral("config")));
    qDebug ("Reading configuration options from file '%s'...", configFile.toLatin1().constData());
    QSettings configSettings (configFile, QSettings::IniFormat);
    QSettings::Status configStatus(configSettings.status());
    if (configStatus == QSettings::NoError) {
        QVariant configValue;
        QStringList validConfigSections;
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! validConfigSections.contains (i->configSection)) {
                validConfigSections.append (i->configSection);
            }
            configValue = configSettings.value (i->configSection + QStringLiteral("/") + i->configName);
            if (configValue.isValid() && (! configValue.isNull())) {
                // https://stackoverflow.com/a/27558297
                if (configValue.toStringList().count() != 1) {
                    qCritical ("Invalid value '%s' is set for configuration key '%s/%s'. Please, quote the parameter!", configValue.toStringList().join(QStringLiteral(",")).trimmed().toLatin1().constData(), i->configSection.toLatin1().constData(), i->configName.toLatin1().constData());
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
                    qWarning ("Invalid configuration key '%s/%s' found in the configuration file!", group->toLatin1().constData(), configName->toLatin1().constData());
                }
            }
            configSettings.endGroup ();
        }
    } else {
        qWarning ("Error #%d while reading configuration file '%s'!", configStatus, configFile.toLatin1().constData());
    }

    // Override definitions specified in command line
    qDebug ("Overriding configuration options specified in command line...");
    for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
        if (optionsParser.isSet (i->configSection + QStringLiteral(".") + i->configName)) {
            *(i->configValue) = optionsParser.value(i->configSection + QStringLiteral(".") + i->configName).trimmed();
        }
    }

    if (! AppConfig::validateSettings ()) {
        return (false);
    }

    // Now figure what helpers are enabled
    QString configurationDir (QFileInfo(configFile).path());
    AppRuntime::helperNames = AppRuntime::helpers.split(QStringLiteral(","), QString::SkipEmptyParts);
    AppRuntime::helperNames.removeDuplicates();
    qDebug ("Detecting active helpers... Preliminary list is: ('%s').", AppRuntime::helperNames.join(QStringLiteral("', '")).toLatin1().constData());
    QStringList unknownOptionNames(optionsParser.unknownOptionNames());
    // QStringList addOptionNames;
    QHash<QString,QString> globalVariables;
    for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helper != AppRuntime::helperNames.end(); helper++) {
        (*helper) = helper->trimmed ();
        if (helper->isEmpty()) {
            qCritical ("Helper names can not be empty!");
            return (false);
        }
        for (QList<AppConfigValidSetting>::const_iterator i = AppConfig::AppConfigValidSettings.constBegin(); i != AppConfig::AppConfigValidSettings.constEnd(); i++) {
            if (! i->configSection.compare ((*helper), Qt::CaseInsensitive)) {
                qCritical ("Invalid helper specification: '%s'!", helper->toLatin1().constData());
                return (false);
            }
        }
        if (! (QFile::exists (QStringLiteral(APP_install_share_dir "/") + AppConstants::AppHelperSubDir + QStringLiteral("/") + (*helper) + AppConstants::AppHelperExtension) || QFile::exists (configurationDir + QStringLiteral("/") + AppConstants::AppHelperSubDir + QStringLiteral("/") + (*helper) + AppConstants::AppHelperExtension))) {
            qCritical ("Helper name '%s' is not defined!", helper->toLatin1().constData());
            return (false);
        }
        // Get helper variables from configuration file
        configSettings.beginGroup (*helper);
        QStringList globalVariableNames (configSettings.childKeys ());
        for (QStringList::const_iterator globalVariable = globalVariableNames.constBegin (); globalVariable != globalVariableNames.constEnd (); globalVariable++) {
            QVariant configValue (configSettings.value(*globalVariable));
            // https://stackoverflow.com/a/27558297
            if (configValue.toStringList().count() != 1) {
                qCritical ("Configuration key '%s/%s' has an invalid value '%s'. Please, quote the parameter!", helper->toLatin1().constData(), globalVariable->toLatin1().constData(), configValue.toStringList().join(QStringLiteral(",")).trimmed().toLatin1().constData());
                return (false);
            }
            globalVariables[(*helper) + QStringLiteral(".") + (*globalVariable)] = configValue.toString().trimmed();
        }
        configSettings.endGroup ();
        for (QStringList::const_iterator optionName = unknownOptionNames.constBegin(); optionName != unknownOptionNames.constEnd(); optionName++) {
            if (optionName->startsWith ((*helper) + QStringLiteral("."))) {
                globalVariables.insert ((*optionName), QString());
            }
        }
    }

    // Parse command line arguments again, considering variables set for helpers
    // This time, parse errors are fatal
    qDebug ("Parsing command line options again...");
    for (QHash<QString,QString>::const_iterator globalVariable = globalVariables.constBegin(); globalVariable != globalVariables.constEnd(); globalVariable++) {
        optionsParser.addOption (QCommandLineOption (globalVariable.key(), QStringLiteral("Custom value for a global variable '%1'").arg(globalVariable.key()), (globalVariable.key()), (globalVariable.key())));
    }
    if (optionsParser.parse (arguments)) {
        if (optionsParser.isSet (helpOption)) {
            optionsParser.showHelp ();
        } else if (optionsParser.isSet (versionOption)) {
            optionsParser.showVersion ();
        } else if (! optionsParser.positionalArguments().isEmpty ()) {
            qCritical ("This program does not accept positional arguments!");
            optionsParser.showHelp (1);
        }
    } else {
        optionsParser.showHelp ();
    }

    QStringList foundOptionNames (optionsParser.optionNames ());
    for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helper != AppRuntime::helperNames.end(); helper++) {
        for (QStringList::const_iterator optionName = foundOptionNames.constBegin(); optionName != foundOptionNames.constEnd(); optionName++) {
            if ((*optionName).startsWith ((*helper) + QStringLiteral("."))) {
                globalVariables[(*optionName)] = optionsParser.value (*optionName).trimmed();
            }
        }
        QHash<QString,QString>::iterator helperCode (AppRuntime::helperSourcesByName.insert ((*helper), QStringLiteral("")));
        for (QHash<QString,QString>::const_iterator variable = globalVariables.constBegin(); variable != globalVariables.constEnd(); variable++) {
            if (variable.key().startsWith ((*helper) + QStringLiteral("."))) {
                helperCode.value() += QStringLiteral("var ") + variable.key().mid(helper->length() + 1) + QStringLiteral(" = decodeURIComponent ('") + QString::fromUtf8(QUrl::toPercentEncoding (variable.value())) + QStringLiteral("');\n");
            }
        }
    }

    // This will be used by the DatabaseBridge object
    AppRuntime::dbStartupQueries = AppRuntime::dbStartupQuery.split(QRegExp(QStringLiteral("\\s*;\\s*")), QString::SkipEmptyParts);
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
    qDebug ("Loading common library contents into memory...");
    bool librariesLoaded = true;
    QLinkedList<QFileInfo> libraries;

    // Search libraries from the architecture-independent data directory first
    QFileInfo libraryRoot (QStringLiteral(APP_install_share_dir "/") + AppConstants::AppCommonSubDir);
    int libraryRootPathLength (libraryRoot.filePath().length());
    libraries.prepend (libraryRoot);
    QHash<QString,QString> libraryCandidates;
    while (! libraries.isEmpty ()) {
        QFileInfo library (libraries.takeFirst ());
        if (library.isDir()) {
            if (library.fileName() != QStringLiteral(".") && library.fileName() != QStringLiteral("..")) {
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
    libraryRoot.setFile (configurationDir + QStringLiteral("/") + AppConstants::AppCommonSubDir);
    libraryRootPathLength = libraryRoot.filePath().length();
    libraries.prepend (libraryRoot);
    while (! libraries.isEmpty ()) {
        QFileInfo library (libraries.takeFirst ());
        if (library.isDir()) {
            if (library.fileName() != QStringLiteral(".") && library.fileName() != QStringLiteral("..")) {
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
            qCritical ("Unable to read library file '%s'!", library.value().toLatin1().constData());
            librariesLoaded = false;
        } else {
            AppRuntime::commonSources.insert (library.key(), libraryFileContents);
        }
    }
    if (! librariesLoaded) {
        return (false);
    }

    // Load helpers into the memory
    qDebug ("Loading the helper jailer into memory...");
    QString helperJailTemplate (AppRuntime::readFileContents (QStringLiteral(":/helperjailer.js")));
    if (helperJailTemplate.isNull ()) {
        qFatal ("Unable to read resource file ':/helperjailer.js'!");
    } else {
        qDebug ("Loading helper contents into memory...");
        QString helperContents;
        QRegExp helperJailPlaceHolder (QStringLiteral("/\\*\\s*Helper\\s+code\\s+goes\\s+here\\W"), Qt::CaseInsensitive);
        if (! helperJailPlaceHolder.isValid ()) {
            qFatal ("'helperJailPlaceHolder' regular expression did not build: '%s'!", helperJailPlaceHolder.errorString().toLatin1().constData());
        }
        int helperPlaceHolderBegin = helperJailTemplate.indexOf (helperJailPlaceHolder);
        if (helperPlaceHolderBegin >= 0) {
            QString helperJailTemplateBegin (helperJailTemplate.left (helperPlaceHolderBegin));
            helperJailTemplate.remove (0, helperPlaceHolderBegin);
            // The variable 'AppRuntime::helperSourcesStartLine' will help administrator to figure
            // the correct line of any exceptions or syntax errors generated by helpers.
            int numNewLines = helperJailTemplateBegin.count(QLatin1Char('\n'));
            int numCarriageReturns = helperJailTemplateBegin.count(QLatin1Char('\r'));
            if (numNewLines > numCarriageReturns) {
                AppRuntime::helperSourcesStartLine = -2 - numNewLines;
            } else {
                AppRuntime::helperSourcesStartLine = -2 - numCarriageReturns;
            }
            int helperPlaceHolderEnd = helperJailTemplate.indexOf (QStringLiteral("*/"));
            if (helperPlaceHolderEnd >= 0) {
                helperJailTemplate.remove (0, helperPlaceHolderEnd + 2);
                bool helpersLoaded = true;
                for (QStringList::iterator helper = AppRuntime::helperNames.begin(); helpersLoaded && helper != AppRuntime::helperNames.end(); helper++) {
                    QFile helperFile (configurationDir + QStringLiteral("/") + AppConstants::AppHelperSubDir + QStringLiteral("/") + (*helper) + AppConstants::AppHelperExtension);
                    if (! helperFile.exists ()) {
                        helperFile.setFileName (QStringLiteral(APP_install_share_dir "/") + AppConstants::AppHelperSubDir + QStringLiteral("/") + (*helper) + AppConstants::AppHelperExtension);
                    }
                    helperContents = AppRuntime::readFileContents (helperFile);
                    if (helperContents.isNull ()) {
                        qCritical ("Unable to read helper file '%s'!", helperFile.fileName().toLatin1().constData());
                        helpersLoaded = false;
                    } else {
                        AppRuntime::helperSourcesByName[(*helper)] = helperJailTemplateBegin + QStringLiteral("\n") + AppRuntime::helperSourcesByName[(*helper)] + QStringLiteral("\n") + helperContents + helperJailTemplate;
                        AppRuntime::helperMemoryCache.append (new AppHelperObjectCache());
                    }
                }
                if (! helpersLoaded) {
                    return (false);
                }
            } else {
                qFatal ("'helperPlaceHolderEnd' search did not succeed!");
            }
        } else {
            qFatal ("'helperPlaceHolderBegin' search did not succeed!");
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
