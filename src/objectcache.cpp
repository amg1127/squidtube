#include "objectcache.h"

ObjectCache::ObjectCache (const QString& helperName, ObjectCache* lowerCache) :
    lowerCache (lowerCache),
    helperName (helperName),
    cacheType ("(unknown)") {
}

CacheStatus ObjectCache::read (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated) {
    CacheStatus returnValue = CacheStatus::CacheMiss;
    qDebug() << QString("[%1] Trying to search %2 cache for information concerning 'className=%3, id=%4'...").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
    bool lockStatus = this->lock ();
    if (lockStatus) {
        returnValue = this->unlockedRead (className, id, timestampMinimum, data, timestampCreated);
    } else {
        qDebug() << QString("[%1] Error locking %2 cache for information retrieval concerning 'className=%3, id=%4'!").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
    }
    if (returnValue == CacheStatus::CacheMiss && this->lowerCache != Q_NULLPTR) {
        returnValue = this->lowerCache->read (className, id, timestampMinimum, data, timestampCreated);
    }
    if (lockStatus && returnValue == CacheStatus::CacheMiss) {
        qDebug() << QString("[%1] Setting status to 'OnProgress' in %2 cache for 'className=%3, id=%4'...").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
        this->unlockedWrite (className, id, QJsonDocument::fromJson("{}"), timestampCreated);
    }
    if (returnValue == CacheStatus::CacheHit) {
        if (! data.object().count()) {
            returnValue = CacheStatus::CacheOnProgress;
        }
    }
    if (lockStatus) {
        if (! this->unlock ()) {
            qDebug() << QString("[%1] Error unlocking %2 cache while retrieving information concerning 'className=%3, id=%4'!").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
        }
    }
    return (returnValue);
}

bool ObjectCache::write (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    bool returnValue = true;
    qDebug() << QString("[%1] Trying to write on %2 cache information concerning 'className=%3, id=%4'...").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
    if (this->lock ()) {
        returnValue = this->unlockedWrite (className, id, data, timestampCreated);
        if (! this->unlock ()) {
            qDebug() << QString("[%1] Error unlocking %2 cache after storing information concerning 'className=%3, id=%4'!").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
            returnValue = false;
        }
    } else {
        qDebug() << QString("[%1] Error locking %2 cache to store information concerning 'className=%3, id=%4'!").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
        returnValue = false;
    }
    if (this->lowerCache != Q_NULLPTR) {
        if (! this->lowerCache->write (className, id, data, timestampCreated)) {
            returnValue = false;
        }
    }
    return (returnValue);
}

//////////////////////////////////////////////////////////////////

ObjectCacheDatabase::ObjectCacheDatabase (const QString& helperName, const QString& dbTblPrefix) :
    ObjectCache (helperName, Q_NULLPTR) {
    this->cacheType = "database";
    // Force a deep copy of the string
    this->dbTable = QString("%1%2").arg(dbTblPrefix).arg(helperName);
}

ObjectCacheDatabase::~ObjectCacheDatabase () {
    databaseBridge::closeDatabase (this->dbConnection);
}

bool ObjectCacheDatabase::lock () {
    // I try to connect to the database twice, because a server may have closed
    // an opened connection due to inactivity.
    for (int step = 0; step < 2; step++) {
        if (this->dbConnection.isOpen ()) {
            if (this->dbConnection.tables().contains (this->dbTable, Qt::CaseInsensitive)) {
                if (this->dbConnection.transaction ()) {
                    return (true);
                } else {
                    databaseBridge::warnSqlError (this->dbConnection.lastError(), QString("Unable to start a transaction on the table '%1'").arg(this->dbTable));
                }
            } else {
                QSqlQuery query (this->dbConnection);
                if (query.exec (QString("CREATE TABLE %1 ("
                    "className        VARCHAR(255) NOT NULL, "
                    "id               VARCHAR(255) NOT NULL, "
                    "data             TEXT         NOT NULL, "
                    "timestampCreated INTEGER      NOT NULL, "
                    "PRIMARY KEY (className, id)"
                    ");").arg(this->dbTable))) {
                    step--;
                    continue;
                } else {
                    databaseBridge::warnSqlError (query, QString("Unable to create table '%1' within the database").arg(this->dbTable));
                }
                query.clear ();
            }
        } else if (step == 0) {
            databaseBridge::closeDatabase (this->dbConnection);
            this->dbConnection = databaseBridge::database ();
        }
    }
    return (false);
}

bool ObjectCacheDatabase::unlock () {
    return (databaseBridge::commitTransaction (this->dbConnection, this->helperName));
}

CacheStatus ObjectCacheDatabase::unlockedRead (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated) {
    static const QStringList fields (QStringList() << "data" << "timestampCreated");
    QHash<QString,QVariant> searchFields;
    searchFields["className"] = className;
    searchFields["id"] = id;
    QHash<QString,QVariant> result = databaseBridge::genericSelect (this->dbConnection, this->dbTable, searchFields, fields);
    if (! result.isEmpty ()) {
        QJsonParseError jsonParseError;
        QJsonDocument _data (QJsonDocument::fromJson (result.value("data").toByteArray(), &jsonParseError));
        if (jsonParseError.error == QJsonParseError::NoError) {
            bool ok;
            qint64 _timestampCreated (result.value("timestampCreated").toLongLong (&ok));
            if (ok) {
                if (ObjectCache::validateJsonData (_data)) {
                    if (timestampMinimum <= _timestampCreated) {
                        data = _data;
                        timestampCreated = _timestampCreated;
                        return (CacheStatus::CacheHit);
                    }
                }
            } else {
                qWarning() << QString("[%1] Timestamp fetched from the database could not be parsed as integer: 'className=%2, id=%3'!").arg(this->helperName).arg(className).arg(id);
            }
        } else {
            qWarning() << QString("[%1] Data fetched from the database could not be parsed as JSON: 'className=%2, id=%3, offset=%4, errorString=%5'!").arg(this->helperName).arg(className).arg(id).arg(jsonParseError.offset).arg(jsonParseError.errorString());
        }
    }
    return (CacheStatus::CacheMiss);
}

bool ObjectCacheDatabase::unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    if (ObjectCache::validateJsonData (data)) {
        QHash<QString,QVariant> searchFields;
        searchFields["className"] = className;
        searchFields["id"] = id;
        QHash<QString,QVariant> updateFields;
        updateFields["data"] = data.toJson (QJsonDocument::Compact);
        updateFields["timestampCreated"] = timestampCreated;
        return (databaseBridge::genericUpdate (this->dbConnection, this->dbTable, searchFields, updateFields, true));
    }
    return (false);
}

//////////////////////////////////////////////////////////////////

ObjectCacheMemory::ObjectCacheMemory (const QString& helperName, ObjectCacheDatabase& databaseCache) :
    ObjectCache (helperName, &databaseCache){
    this->cacheType = "memory";
    AppRuntime::helperMemoryCacheMutex.lock ();
    if (AppRuntime::helperMemoryCache.contains (helperName)) {
        this->objectCache = AppRuntime::helperMemoryCache.value (helperName);
    } else {
        qFatal("QHash 'AppRuntime::helperMemoryCache' is not initialized correctly!");
    }
    AppRuntime::helperMemoryCacheMutex.unlock ();
}

ObjectCacheMemory::~ObjectCacheMemory () {
}

bool ObjectCacheMemory::lock () {
    bool locked = this->objectCache->mutex.tryLock (AppConstants::AppHelperMutexTimeout);
    if (! locked) {
        qWarning() << QString("[%1] Timeout reached while locking %2 cache for exclusive access. Thread congestion?").arg(this->helperName).arg(this->cacheType);
    }
    return (locked);
}

bool ObjectCacheMemory::unlock () {
    this->objectCache->mutex.unlock();
    return (true);
}

CacheStatus ObjectCacheMemory::unlockedRead (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated) {
    if (this->objectCache->classNames.contains (className)) {
        AppHelperClass* classNameObj = this->objectCache->classNames.value (className);
        if (classNameObj->ids.contains (id)) {
            AppHelperObject* idObj = classNameObj->ids.value (id);
            if (timestampMinimum <= idObj->timestampCreated) {
                if (ObjectCache::validateJsonData (idObj->data)) {
                    data = idObj->data;
                    timestampCreated = idObj->timestampCreated;
                    return CacheStatus::CacheHit;
                }
            }
            classNameObj->ids.remove (id);
            delete idObj;
        }
    }
    return (CacheStatus::CacheMiss);
}

bool ObjectCacheMemory::unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    AppHelperClass* classNameObj;
    AppHelperObject* idObj;
    if (this->objectCache->classNames.contains (className)) {
        classNameObj = this->objectCache->classNames.value (className);
    } else {
        this->objectCache->classNames[className] = classNameObj = new AppHelperClass ();
    }
    if (classNameObj->ids.contains (id)) {
        idObj = classNameObj->ids.value (id);
    } else {
        if (classNameObj->ids.count() >= AppConstants::AppHelperCacheMaxSize) {
            qDebug() << QString("[%1] Memory cache for 'className=%2' has reached the maximum size. Vacuuming...").arg(this->helperName).arg(className);
            QMultiMap<qlonglong,QString> idsByAge;
            for (QHash<QString,AppHelperObject*>::const_iterator otherIdObj = classNameObj->ids.constBegin(); otherIdObj != classNameObj->ids.constEnd(); otherIdObj++) {
                idsByAge.insert (otherIdObj.value()->timestampCreated, otherIdObj.key());
            }
            QMultiMap<qlonglong,QString>::iterator oldestId = idsByAge.begin ();
            for (int i = 0; i < AppConstants::AppHelperCacheVacuumSize && (! classNameObj->ids.isEmpty()); i++) {
                delete (classNameObj->ids.take(oldestId.value()));
                oldestId = idsByAge.erase (oldestId);
            }
        }
        classNameObj->ids[id] = idObj = new AppHelperObject ();
    }
    idObj->data = data;
    idObj->timestampCreated = timestampCreated;
    return (true);
}
