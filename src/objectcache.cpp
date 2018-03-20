#include "objectcache.h"

ObjectCache::ObjectCache (const QString& helperName, ObjectCache* lowerCache) :
    lowerCache (lowerCache),
    helperName (helperName),
    cacheType ("(unknown)") {
}

/*
 * A reminder:
 *
 * * An invalid JSON Document means a failure during object data retrieval (negativeTTL).
 * * A valid JSON Document that is not an array or has no items means another thread or process is retrieving data already.
 * * A valid JSON Document that is an array with at least one item means valid cached data (positiveTTL).
 *
 */

CacheStatus ObjectCache::read (const QString& className, const QString& id, const qint64 timestampNow, QJsonDocument& data, qint64& timestampCreated) {
    bool cacheHit = false;
    CacheStatus returnValue = CacheStatus::CacheMiss;
    QJsonDocument _data;
    qint64 _timestampCreated;
    qDebug() << QString("[%1] Trying to search %2 cache for information concerning 'className=%3, id=%4'...").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
    bool lockStatus = this->lock ();
    if (lockStatus) {
        cacheHit = this->unlockedRead (className, id, _data, _timestampCreated);
    } else {
        qDebug() << QString("[%1] Error locking %2 cache for information retrieval concerning 'className=%3, id=%4'!").arg(this->helperName).arg(this->cacheType).arg(className).arg(id);
    }
    if ((! cacheHit) && this->lowerCache != Q_NULLPTR) {
        returnValue = this->lowerCache->read (className, id, timestampNow, _data, _timestampCreated);
        cacheHit = (returnValue != CacheStatus::CacheMiss);
    }
    if (cacheHit && returnValue == CacheStatus::CacheMiss) {
        returnValue = ObjectCache::jsonDocumentIsFresh (_data, _timestampCreated, timestampNow);
        cacheHit = (returnValue != CacheStatus::CacheMiss);
    }
    if (cacheHit) {
        if (returnValue == CacheStatus::CacheHitNegative || _data.array().count()) {
            data = _data;
            timestampCreated = _timestampCreated;
        } else if ((_timestampCreated + AppConstants::AppHelperMaxWait) >= timestampNow) {
            returnValue = CacheStatus::CacheOnProgress;
        } else {
            /*
             * If multiple threads or processes are waiting for an answer and the worker which took that job times out,
             * a race condition begins. It will not crash the program, fortunately. However, the race condition will
             * produce concurrent calls to helper's 'getPropertiesFromObject ();' function, so duplicated API calls will
             * be issued.
             *
             * Such race condition may be triggered by a network outage. In this scenario, the helper returns error
             * codes to the program and the race ends.
             *
             */
            returnValue = CacheStatus::CacheMiss;
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

bool ObjectCache::jsonDocumentIsValid (const QJsonDocument& data) {
    return (data.isArray() && (! data.isObject()) && (! data.isNull()) && (! data.isEmpty ()));
}

CacheStatus ObjectCache::jsonDocumentIsFresh (const QJsonDocument& data, const qint64 timestampCreated, const qint64 timestampNow) {
    if (jsonDocumentIsValid (data)) {
        if ((timestampCreated + AppRuntime::positiveTTLint) >= timestampNow) {
            return (CacheStatus::CacheHitPositive);
        }
    } else {
        if ((timestampCreated + AppRuntime::negativeTTLint) >= timestampNow) {
            return (CacheStatus::CacheHitNegative);
        }
    }
    return (CacheStatus::CacheMiss);
}

//////////////////////////////////////////////////////////////////

ObjectCacheDatabase::ObjectCacheDatabase (const QString& helperName, const QString& dbTblPrefix) :
    ObjectCache (helperName, Q_NULLPTR) {
    this->cacheType = "database";
    // Force a deep copy of the string
    this->dbTable = QString("%1%2").arg(dbTblPrefix).arg(helperName);
}

ObjectCacheDatabase::~ObjectCacheDatabase () {
    DatabaseBridge::closeDatabase (this->dbConnection);
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
                    DatabaseBridge::warnSqlError (this->dbConnection.lastError(), QString("Unable to start a transaction on the table '%1'").arg(this->dbTable));
                }
            } else {
                QSqlQuery query (this->dbConnection);
                if (query.exec (QString("CREATE TABLE %1 ("
                    "className        VARCHAR(48) NOT NULL, "
                    "id               VARCHAR(168) NOT NULL, "
                    "data             TEXT             NULL, "
                    "timestampCreated INTEGER      NOT NULL, "
                    "PRIMARY KEY (className, id)"
                    ");").arg(this->dbTable))) {
                    step--;
                    continue;
                } else {
                    DatabaseBridge::warnSqlError (query, QString("Unable to create table '%1' within the database").arg(this->dbTable));
                }
                query.clear ();
            }
        } else if (step == 0) {
            DatabaseBridge::closeDatabase (this->dbConnection);
            this->dbConnection = DatabaseBridge::database ();
        }
    }
    return (false);
}

bool ObjectCacheDatabase::unlock () {
    return (DatabaseBridge::commitTransaction (this->dbConnection, this->helperName));
}

bool ObjectCacheDatabase::unlockedRead (const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) {
    static const QStringList fields (QStringList() << "data" << "timestampCreated");
    QHash<QString,QVariant> searchFields;
    searchFields["className"] = className;
    searchFields["id"] = id;
    QHash<QString,QVariant> result = DatabaseBridge::genericSelect (this->dbConnection, this->dbTable, searchFields, fields);
    if (! result.isEmpty ()) {
        bool ok;
        qint64 _timestampCreated (result.value("timestampCreated").toLongLong (&ok));
        if (ok) {
            QJsonDocument _data;
            if (! result.value("data").isNull ()) {
                QJsonParseError jsonParseError;
                _data = QJsonDocument::fromJson (result.value("data").toByteArray(), &jsonParseError);
                if (jsonParseError.error != QJsonParseError::NoError) {
                    qWarning() << QString("[%1] Data fetched from the database could not be parsed as JSON: 'className=%2, id=%3, offset=%4, errorString=%5'!").arg(this->helperName).arg(className).arg(id).arg(jsonParseError.offset).arg(jsonParseError.errorString());
                }
            }
            data = _data;
            timestampCreated = _timestampCreated;
            return (true);
        } else {
            qWarning() << QString("[%1] Timestamp fetched from the database could not be parsed as integer: 'className=%2, id=%3'!").arg(this->helperName).arg(className).arg(id);
        }
    }
    return (false);
}

bool ObjectCacheDatabase::unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    QHash<QString,QVariant> searchFields;
    searchFields["className"] = className;
    searchFields["id"] = id;
    QHash<QString,QVariant> updateFields;
    if (ObjectCache::jsonDocumentIsValid (data)) {
        updateFields["data"] = data.toJson (QJsonDocument::Compact);
    } else {
        updateFields["data"] = QVariant();
    }
    updateFields["timestampCreated"] = timestampCreated;
    return (DatabaseBridge::genericUpdate (this->dbConnection, this->dbTable, searchFields, updateFields, true));
}

//////////////////////////////////////////////////////////////////

ObjectCacheMemory::ObjectCacheMemory (const QString& helperName, ObjectCacheDatabase& databaseCache) :
    ObjectCache (helperName, &databaseCache),
    cacheSize (0) {
    this->cacheType = "memory";
    AppRuntime::helperMemoryCacheMutex.lock ();
    int index = AppRuntime::helperNames.indexOf (helperName);
    if (index >= 0) {
        this->objectCache = AppRuntime::helperMemoryCache.at (index);
    } else {
        qFatal("QHash 'AppRuntime::helperMemoryCache' is not properly initialized!");
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

bool ObjectCacheMemory::unlockedRead (const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) {
    if (this->objectCache->classNames.contains (className)) {
        AppHelperClass* classNameObj = this->objectCache->classNames.value (className);
        if (classNameObj->ids.contains (id)) {
            AppHelperObject* idObj = classNameObj->ids.value (id);
            data = idObj->data;
            timestampCreated = idObj->timestampCreated;
            return (true);
        }
    }
    return (false);
}

bool ObjectCacheMemory::unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    AppHelperClass* classNameObj;
    AppHelperObject* idObj;
    if (this->objectCache->classNames.contains (className)) {
        classNameObj = this->objectCache->classNames.value (className);
    } else {
        this->objectCache->classNames[className] = classNameObj = new AppHelperClass ();
        classNameObj->objectCache = this->objectCache;
    }
    if (classNameObj->ids.contains (id)) {
        idObj = classNameObj->ids.value (id);
    } else {
        if (this->cacheSize >= AppConstants::AppHelperCacheMaxSize) {
            qDebug() << QString("[%1] Memory cache for 'className=%2' has reached the maximum size. Vacuuming...").arg(this->helperName).arg(className);
            QMultiMap<qlonglong,AppHelperObject*> idsByAge;
            AppHelperClass* otherClassNameObj;
            AppHelperObject* otherIdObj;
            for (QHash<QString,AppHelperClass*>::const_iterator otherClassNameIterator = this->objectCache->classNames.constBegin(); otherClassNameIterator != this->objectCache->classNames.constEnd(); otherClassNameIterator++) {
                otherClassNameObj = otherClassNameIterator.value();
                for (QHash<QString,AppHelperObject*>::const_iterator otherIdIterator = otherClassNameObj->ids.constBegin(); otherIdIterator != otherClassNameObj->ids.constEnd(); otherIdIterator++) {
                    otherIdObj = otherIdIterator.value();
                    idsByAge.insert (otherIdObj->timestampCreated, otherIdObj);
                }
            }
            QMultiMap<qlonglong,AppHelperObject*>::iterator oldestId = idsByAge.begin();
            for (int i = 0; i < AppConstants::AppHelperCacheVacuumSize && i < AppConstants::AppHelperCacheMaxSize; i++) {
                otherIdObj = oldestId.value();
                oldestId = idsByAge.erase (oldestId);
                otherIdObj->objectClass->ids.remove (otherIdObj->objectClass->ids.key (otherIdObj));
                delete (otherIdObj);
                this->cacheSize--;
            }
        }
        classNameObj->ids[id] = idObj = new AppHelperObject ();
        idObj->objectClass = classNameObj;
        this->cacheSize++;
    }
    idObj->data = data;
    idObj->timestampCreated = timestampCreated;
    return (true);
}
