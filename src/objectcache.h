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
    QByteArray helperName;
    QByteArray cacheType;
    virtual bool lock (const unsigned int requestId) = 0;
    virtual bool unlock (const unsigned int requestId) = 0;
    virtual bool unlockedRead (const unsigned int requestId, const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated) = 0;
    virtual bool unlockedWrite (const unsigned int requestId, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated) = 0;
public:
    ObjectCache (const QByteArray& _helperName, ObjectCache* _lowerCache = Q_NULLPTR);
    virtual CacheStatus read (const unsigned int requestId, const QString& className, const QString& id, const qint64 timestampNow, QJsonDocument& data, qint64& timestampCreated);
    virtual bool write (const unsigned int requestId, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
    static bool jsonDocumentIsValid (const QJsonDocument& data);
    static bool jsonDocumentHasData (const QJsonDocument& data);
    static CacheStatus jsonDocumentIsFresh (const QJsonDocument& data, const qint64 timestampCreated, const qint64 timestampNow);
};

class ObjectCacheDatabase : public ObjectCache {
private:
    QByteArray dbTable;
    QSqlDatabase dbConnection;
    virtual bool lock (const unsigned int requestId);
    virtual bool unlock (const unsigned int);
    virtual bool unlockedRead (const unsigned int requestId, const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const unsigned int, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheDatabase (const QByteArray& helperName, const QByteArray& dbTblPrefix);
    virtual ~ObjectCacheDatabase ();
};

class ObjectCacheMemory : public ObjectCache {
private:
    AppHelperObjectCache* objectCache;
    AppHelperObject* moveObjectIdAhead (const QHash<QString,AppHelperObject*>::iterator& idIterator);
    virtual bool lock (const unsigned int requestId);
    virtual bool unlock (const unsigned int);
    virtual bool unlockedRead (const unsigned int, const QString& className, const QString& id, QJsonDocument& data, qint64& timestampCreated);
    virtual bool unlockedWrite (const unsigned int requestId, const QString& className, const QString& id, const QJsonDocument& data, const qint64 timestampCreated);
public:
    ObjectCacheMemory (const QByteArray& helperName, ObjectCacheDatabase& databaseCache);
    virtual ~ObjectCacheMemory ();
};

#endif // OBJECTCACHE_H
