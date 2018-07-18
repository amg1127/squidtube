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

#include "appruntime.h"

//////////////////////////////////////////////////////////////////
// Default application settings go below

#ifdef QT_NO_DEBUG
QString AppRuntime::loglevel(QStringLiteral("INFO"));
#else
QString AppRuntime::loglevel(QStringLiteral("DEBUG"));
#endif
QString AppRuntime::helpers(QStringLiteral(""));
QString AppRuntime::positiveTTL(QStringLiteral("2592000"));
QString AppRuntime::negativeTTL(QStringLiteral("600"));
QString AppRuntime::squidProtocol(QStringLiteral("3.4"));
QString AppRuntime::dbDriver(QStringLiteral("QSQLITE3"));
QString AppRuntime::dbHost(QStringLiteral(""));
QString AppRuntime::dbPort(QStringLiteral("0"));
QString AppRuntime::dbUser(QStringLiteral(""));
QString AppRuntime::dbPassword(QStringLiteral(""));
QString AppRuntime::dbOptions(QStringLiteral(""));
QString AppRuntime::dbName(QStringLiteral(":memory:"));
QString AppRuntime::dbStartupQuery(QStringLiteral(""));
QString AppRuntime::dbTblPrefix(QStringLiteral("tblHelper_"));

// Default application settings go above
//////////////////////////////////////////////////////////////////

QMutex AppRuntime::helperMemoryCacheMutex;
QList<AppHelperObjectCache*> AppRuntime::helperMemoryCache;
QHash<QString,QString> AppRuntime::helperSourcesByName;
int AppRuntime::helperSourcesStartLine;
QStringList AppRuntime::helperNames;
QMutex AppRuntime::commonSourcesMutex;
QHash<QString,QString> AppRuntime::commonSources;

unsigned int AppRuntime::dbInstance = 1;
QStringList AppRuntime::dbStartupQueries;
QMutex AppRuntime::dbSettingsMutex;

qint64 AppRuntime::positiveTTLint = 0;
qint64 AppRuntime::negativeTTLint = 0;

QMutex AppRuntime::textCoDecMutex;

QDateTime AppRuntime::currentDateTime () {
    static QMutex m (QMutex::Recursive);
    QMutexLocker m_lck (&m);
    QDateTime dateTime (QDateTime::currentDateTime());
    return (dateTime);
}


QString AppRuntime::readFileContents (const QString& fileName) {
    QFile fileObj (fileName);
    return (readFileContents (fileObj));
}

QString AppRuntime::readFileContents (QFile& fileObj) {
    if (fileObj.open (QIODevice::ReadOnly | QIODevice::Text)) {
        QString fileContents;
        bool read = true;
        {
            QTextStream fileStreamObj (&fileObj);
            fileContents = fileStreamObj.readAll ();
        }
        if (fileObj.error() != QFileDevice::NoError) {
            qWarning ("Error reading contents of file '%s': '%s'!", fileObj.fileName().toLatin1().constData(), fileObj.errorString().toLatin1().constData());
            read = false;
        }
        if (! fileObj.atEnd ()) {
            qWarning ("The file '%s' was not read completely!", fileObj.fileName().toLatin1().constData());
            read = false;
        }
        fileObj.close ();
        if (read) {
            qDebug ("Successfully loaded contents of the file '%s'.", fileObj.fileName().toLatin1().constData());
            return (fileContents);
        }
    } else {
        qWarning ("Unable to open file '%s' for reading: '%s'!", fileObj.fileName().toLatin1().constData(), fileObj.errorString().toLatin1().constData());
    }
    return (QString());
}

//////////////////////////////////////////////////////////////////

// Constants used by the memory cache system
// The maximum time in miliseconds for the memory cache mutex to become available
const int AppConstants::AppHelperMutexTimeout = 30000;
// How many objects at most the cache is allowed to store
const int AppConstants::AppHelperCacheMaxSize = 8192;
// How many elements will be dropped from the cache if it has reached the maximum size
const int AppConstants::AppHelperCacheVacuumSize = 2048;

// How long the program will wait after detecting that another thread or process started
// to fetch data from an object whose information the program wanted to get.
// If this timer expires the program will check again if such thread of process has already
// finished its job.
// Unit is milliseconds.
const int AppConstants::AppHelperTimerTimeout = 1000;

// How long the program will wait for such thread or process. If this timer expires,
// The program will by itself fetch data from the object
// Unit is seconds.
const int AppConstants::AppHelperMaxWait = 30;

// Path component where common libraries will be searched for
const QString AppConstants::AppCommonSubDir(QStringLiteral("common"));

// Path component where helpers will be searched for
const QString AppConstants::AppHelperSubDir(QStringLiteral("helpers"));

// File extension of common libraries and helpers
const QString AppConstants::AppHelperExtension(QStringLiteral(".js"));

//////////////////////////////////////////////////////////////////

// Memory cache destructors

AppHelperClass::~AppHelperClass () {
    for (QHash<QString,AppHelperObject*>::iterator helperId = this->ids.begin(); helperId != this->ids.end(); helperId++) {
        delete (*helperId);
    }
    this->ids.clear ();
}

AppHelperObjectCache::~AppHelperObjectCache () {
    for (QHash<QString,AppHelperClass*>::iterator helperClass = this->classNames.begin(); helperClass != this->classNames.end(); helperClass++) {
        delete (*helperClass);
    }
    this->classNames.clear ();
}

//////////////////////////////////////////////////////////////////

AppJobRequest::~AppJobRequest () {
}

//////////////////////////////////////////////////////////////////

AppJobRequestFromSquid::~AppJobRequestFromSquid () {
}

AppRequestType AppJobRequestFromSquid::type () {
    return (RequestFromSquid);
}

//////////////////////////////////////////////////////////////////

AppJobRequestFromHelper::~AppJobRequestFromHelper () {
}

AppRequestType AppJobRequestFromHelper::type () {
    return (RequestFromHelper);
}
