#ifndef OBJECTCACHE_H
#define OBJECTCACHE_H

#include "appconfig.h"
#include "appruntime.h"
#include "databasebridge.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMultiMap>
#include <QMutex>
#include <QString>
#include <QtDebug>

enum class CacheStatus {
    CacheMiss = 1,
    CacheOnProgress = 2,
    CacheHit = 3
};

class ObjectCache {
private:
    ObjectCache* lowerCache;
protected:
    QString helperName;
    QString cacheType;
    virtual bool lock () = 0;
    virtual bool unlock () = 0;
    virtual CacheStatus unlockedRead (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated) = 0;
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) = 0;
public:
    inline static bool validateJsonData (const QJsonDocument& data) { return ((! data.isNull()) && (! data.isEmpty()) && (! data.isArray()) && data.isObject ()); }
    ObjectCache (const QString& helperName, ObjectCache* lowerCache = Q_NULLPTR);
    virtual CacheStatus read (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated);
    virtual bool write (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
};

class ObjectCacheDatabase : public ObjectCache {
private:
    QString dbTable;
    QSqlDatabase dbConnection;
    virtual bool lock ();
    virtual bool unlock ();
    virtual CacheStatus unlockedRead (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheDatabase (const QString& helperName, const QString& dbTblPrefix);
    virtual ~ObjectCacheDatabase ();
};

class ObjectCacheMemory : public ObjectCache {
private:
    AppHelperObjectCache* objectCache;
    virtual bool lock ();
    virtual bool unlock ();
    virtual CacheStatus unlockedRead (const QString& className, const QString& id, const qint64 timestampMinimum, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheMemory (const QString& helperName, ObjectCacheDatabase& databaseCache);
    virtual ~ObjectCacheMemory ();
};

#endif // OBJECTCACHE_H
