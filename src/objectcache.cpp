#include "objectcache.h"

ObjectCache::~ObjectCache () {
}

ObjectCache::ObjectCache (const QString& helperName, ObjectCache* lowerCache) :
    lowerCache (lowerCache),
    helperName (helperName),
    cacheType (QStringLiteral("(unknown)")) {
}

/*
 * A reminder:
 *
 * * An invalid JSON Document means a failure during object data retrieval (negativeTTL).
 * * A valid and empty JSON Document means another thread or process is retrieving data already.
 * * A valid and non empty JSON Document means valid cached data (positiveTTL).
 *
 */

CacheStatus ObjectCache::read (const unsigned int requestId, const QString& className, const QString& id, const qint64 timestampNow, QJsonDocument& data, qint64& timestampCreated) {
    bool cacheHit = false;
    bool fromLower = false;
    CacheStatus returnValue = CacheMiss;
    QJsonDocument _data;
    qint64 _timestampCreated;
    qDebug ("[%s#%u] Trying to search %s cache for information concerning (className='%s', id='%s')...", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
    bool lockStatus = this->lock (requestId);
    if (lockStatus) {
        cacheHit = this->unlockedRead (requestId, className, id, _data, _timestampCreated);
    } else {
        qDebug ("[%s#%u] Error locking %s cache for information retrieval concerning (className='%s', id='%s')!", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
    }
    if ((! cacheHit) && this->lowerCache != Q_NULLPTR) {
        returnValue = this->lowerCache->read (requestId, className, id, timestampNow, _data, _timestampCreated);
        cacheHit = (returnValue != CacheMiss);
        fromLower = true;
    }
    if (cacheHit && returnValue == CacheMiss) {
        returnValue = ObjectCache::jsonDocumentIsFresh (_data, _timestampCreated, timestampNow);
        cacheHit = (returnValue != CacheMiss);
    }
    if (cacheHit) {
        if (returnValue == CacheHitNegative || ObjectCache::jsonDocumentHasData (_data)) {
            if (lockStatus && fromLower) {
                this->unlockedWrite (requestId, className, id, _data, _timestampCreated);
            }
            data = _data;
            timestampCreated = _timestampCreated;
        } else if ((_timestampCreated + AppConstants::AppHelperMaxWait) > timestampNow) {
            returnValue = CacheOnProgress;
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
            returnValue = CacheMiss;
        }
    }
    if (lockStatus) {
        if (! this->unlock (requestId)) {
            qDebug ("[%s#%u] Error unlocking %s cache while retrieving information concerning (className='%s', id='%s')", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
        }
    }
    return (returnValue);
}

bool ObjectCache::write (const unsigned int requestId, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    bool returnValue = true;
    qDebug ("[%s#%u] Trying to write into %s cache information concerning (className='%s', id='%s')...", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
    if (this->lock (requestId)) {
        returnValue = this->unlockedWrite (requestId, className, id, data, timestampCreated);
        if (! this->unlock (requestId)) {
            qDebug ("[%s#%u] Error unlocking %s cache after storing information concerning (className='%s', id=%s')!", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
            returnValue = false;
        }
    } else {
        qDebug ("[%s#%u] Error locking %s cache to store information concerning (className='%s', id='%s'!", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData(), className.toLatin1().constData(), id.toLatin1().constData());
        returnValue = false;
    }
    if (this->lowerCache != Q_NULLPTR) {
        if (! this->lowerCache->write (requestId, className, id, data, timestampCreated)) {
            returnValue = false;
        }
    }
    return (returnValue);
}

bool ObjectCache::jsonDocumentIsValid (const QJsonDocument& data) {
    return ((data.isObject() || data.isArray()) && (! data.isNull()) && (! data.isEmpty ()));
}

bool ObjectCache::jsonDocumentHasData (const QJsonDocument& data) {
    if (! (data.isNull() || data.isEmpty ())) {
        if (data.isObject ()) {
            return (data.object().count() > 0);
        } else if (data.isArray ()) {
            return (data.array().count() > 0);
        }
    }
    return (false);
}

CacheStatus ObjectCache::jsonDocumentIsFresh (const QJsonDocument& data, const qint64 timestampCreated, const qint64 timestampNow) {
    if (jsonDocumentIsValid (data)) {
        if ((timestampCreated + AppRuntime::positiveTTLint) > timestampNow) {
            return (CacheHitPositive);
        }
    } else {
        if ((timestampCreated + AppRuntime::negativeTTLint) > timestampNow) {
            return (CacheHitNegative);
        }
    }
    return (CacheMiss);
}

//////////////////////////////////////////////////////////////////

ObjectCacheDatabase::ObjectCacheDatabase (const QString& helperName, const QString& dbTblPrefix) :
    ObjectCache (helperName, Q_NULLPTR) {
    this->cacheType = QStringLiteral("database");
    QString helperNameModified (helperName);
    helperNameModified.replace (QRegExp(QStringLiteral("\\W")), QStringLiteral("_"));
    this->dbTable = dbTblPrefix + helperNameModified;
}

ObjectCacheDatabase::~ObjectCacheDatabase () {
    DatabaseBridge::closeDatabase (this->dbConnection);
}

bool ObjectCacheDatabase::lock (const unsigned int requestId) {
    // I try to connect to the database twice, because a server may have closed
    // an opened connection due to inactivity.
    for (int step = 0; step < 2; step++) {
        if (this->dbConnection.isOpen ()) {
            if (this->dbConnection.tables().contains (this->dbTable, Qt::CaseInsensitive)) {
                if (this->dbConnection.transaction ()) {
                    return (true);
                } else {
                    DatabaseBridge::warnSqlError (this->dbConnection.lastError(), QString(QStringLiteral("Unable to start a transaction on the table '%1'")).arg(this->dbTable));
                }
            } else {
                QSqlQuery query (this->dbConnection);
                if (query.exec (QString(QStringLiteral("CREATE TABLE %1 ("
                    "className        VARCHAR(48) NOT NULL, "
                    "id               VARCHAR(168) NOT NULL, "
                    "data             TEXT             NULL, "
                    "timestampCreated INTEGER      NOT NULL, "
                    "PRIMARY KEY (className, id)"
                    ");")).arg(this->dbTable))) {
                    step--;
                    continue;
                } else {
                    DatabaseBridge::warnSqlError (query, QString(QStringLiteral("[%1#%2] Unable to create table '%3' within the database")).arg(this->helperName, QString::number(requestId), this->dbTable));
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

bool ObjectCacheDatabase::unlock (const unsigned int) {
    return (DatabaseBridge::commitTransaction (this->dbConnection, this->helperName));
}

bool ObjectCacheDatabase::unlockedRead (const unsigned int requestId, const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) {
    static const QByteArrayList fields (QByteArrayList() << "data" << "timestampCreated");
    QHash<QByteArray,QVariant> searchFields;
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
                    qWarning ("[%s#%u] Data fetched from the database could not be parsed as JSON: (className='%s', id='%s', offset=%d, errorString='%s')", this->helperName.toLatin1().constData(), requestId, className.toLatin1().constData(), id.toLatin1().constData(), jsonParseError.offset, jsonParseError.errorString().toLatin1().constData());
                }
            }
            data = _data;
            timestampCreated = _timestampCreated;
            return (true);
        } else {
            qWarning ("[%s#%u] Timestamp fetched from the database could not be parsed as integer: (className='%s', id='%s', value='%s')", this->helperName.toLatin1().constData(), requestId, className.toLatin1().constData(), id.toLatin1().constData(), result.value("timestampCreated").toByteArray().constData());
        }
    }
    return (false);
}

bool ObjectCacheDatabase::unlockedWrite (const unsigned int, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
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

AppHelperObject* ObjectCacheMemory::moveObjectIdAhead (const QHash<QString,AppHelperObject*>::iterator& idIterator) {
    AppHelperObject* idObj = (*idIterator);
    this->objectCache->cachedIds.erase (idObj->cachePosition);
    idObj->cachePosition = this->objectCache->cachedIds.insert (this->objectCache->cachedIds.begin(), idObj);
    return (idObj);
}

ObjectCacheMemory::ObjectCacheMemory (const QString& helperName, ObjectCacheDatabase& databaseCache) :
    ObjectCache (helperName, &databaseCache) {
    this->cacheType = "memory";
    AppRuntime::helperMemoryCacheMutex.lock ();
    int index = AppRuntime::helperNames.indexOf (helperName);
    if (index >= 0) {
        this->objectCache = AppRuntime::helperMemoryCache.at (index);
    } else {
        qFatal ("QHash 'AppRuntime::helperMemoryCache' is not properly initialized!");
    }
    AppRuntime::helperMemoryCacheMutex.unlock ();
}

ObjectCacheMemory::~ObjectCacheMemory () {
}

bool ObjectCacheMemory::lock (const unsigned int requestId) {
    bool locked = this->objectCache->mutex.tryLock (AppConstants::AppHelperMutexTimeout);
    if (! locked) {
        qWarning ("[%s#%u] Timeout reached while locking %s cache for exclusive access. Thread congestion?", this->helperName.toLatin1().constData(), requestId, this->cacheType.toLatin1().constData());
    }
    return (locked);
}

bool ObjectCacheMemory::unlock (const unsigned int) {
    this->objectCache->mutex.unlock();
    return (true);
}

bool ObjectCacheMemory::unlockedRead (const unsigned int, const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) {
    QHash<QString,AppHelperClass*>::const_iterator classNameIterator = this->objectCache->classNames.find (className);
    if (classNameIterator != this->objectCache->classNames.constEnd()) {
        AppHelperClass* classNameObj = (*classNameIterator);
        QHash<QString,AppHelperObject*>::iterator idIterator = classNameObj->ids.find (id);
        if (idIterator != classNameObj->ids.constEnd()) {
            AppHelperObject* idObj = this->moveObjectIdAhead (idIterator);
            data = idObj->data;
            timestampCreated = idObj->timestampCreated;
            return (true);
        }
    }
    return (false);
}

bool ObjectCacheMemory::unlockedWrite (const unsigned int requestId, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) {
    QHash<QString,AppHelperClass*>::iterator classNameIterator = this->objectCache->classNames.find (className);
    if (classNameIterator == this->objectCache->classNames.end()) {
        classNameIterator = this->objectCache->classNames.insert (className, new AppHelperClass ());
        (*classNameIterator)->objectCache = this->objectCache;
    }
    AppHelperClass* classNameObj = (*classNameIterator);
    QHash<QString,AppHelperObject*>::iterator idIterator = classNameObj->ids.find (id);
    AppHelperObject* idObj;
    if (idIterator != classNameObj->ids.end()) {
        idObj = this->moveObjectIdAhead (idIterator);
    } else {
        idObj = new AppHelperObject ();
        idObj->objectClass = classNameObj;
        idObj->cachePosition = this->objectCache->cachedIds.insert (this->objectCache->cachedIds.begin(), idObj);
        idObj->classPosition = classNameObj->ids.insert (id, idObj);
        this->objectCache->cacheSize++;
        if (this->objectCache->cacheSize > AppConstants::AppHelperCacheMaxSize) {
            AppHelperObject* removedIdObj;
            int i;
            for (i = 0; i < AppConstants::AppHelperCacheVacuumSize && this->objectCache->cacheSize > 1; i++, this->objectCache->cacheSize--) {
                removedIdObj = this->objectCache->cachedIds.takeLast ();
                removedIdObj->objectClass->ids.erase (removedIdObj->classPosition);
                delete (removedIdObj);
            }
            qDebug ("[%s#%u] %d entries were deleted from %s cache because the maximum number of entries (%d) had been reached.", this->helperName.toLatin1().constData(), requestId, i, this->cacheType.toLatin1().constData(), AppConstants::AppHelperCacheMaxSize);
        }
    }
    idObj->data = data;
    idObj->timestampCreated = timestampCreated;
    return (true);
}
