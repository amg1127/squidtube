#ifndef OBJECTCACHE_H
#define OBJECTCACHE_H

#include "appconfig.h"
#include "appruntime.h"
#include "databasebridge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMultiMap>
#include <QMutex>
#include <QString>
#include <QtDebug>

enum CacheStatus {
    CacheMiss = 1,
    CacheOnProgress = 2,
    CacheHitNegative = 3,
    CacheHitPositive = 4
};

class ObjectCache {
private:
    ObjectCache* lowerCache;
protected:
    ~ObjectCache ();
    QString helperName;
    QString cacheType;
    virtual bool lock () = 0;
    virtual bool unlock () = 0;
    virtual bool unlockedRead (const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) = 0;
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) = 0;
public:
    ObjectCache (const QString& helperName, ObjectCache* lowerCache = Q_NULLPTR);
    virtual CacheStatus read (const QString& className, const QString& id, const qint64 timestampNow, QJsonDocument& data, qint64& timestampCreated);
    virtual bool write (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
    static bool jsonDocumentIsValid (const QJsonDocument& data);
    static bool jsonDocumentHasData (const QJsonDocument& data);
    static CacheStatus jsonDocumentIsFresh (const QJsonDocument& data, const qint64 timestampCreated, const qint64 timestampNow);
};

class ObjectCacheDatabase : public ObjectCache {
private:
    QString dbTable;
    QSqlDatabase dbConnection;
    virtual bool lock ();
    virtual bool unlock ();
    virtual bool unlockedRead (const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheDatabase (const QString& helperName, const QString& dbTblPrefix);
    virtual ~ObjectCacheDatabase ();
};

class ObjectCacheMemory : public ObjectCache {
private:
    AppHelperObjectCache* objectCache;
    AppHelperObject* moveObjectIdAhead (const QHash<QString,AppHelperObject*>::iterator& idIterator);
    virtual bool lock ();
    virtual bool unlock ();
    virtual bool unlockedRead (const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheMemory (const QString& helperName, ObjectCacheDatabase& databaseCache);
    virtual ~ObjectCacheMemory ();
};

#endif // OBJECTCACHE_H
