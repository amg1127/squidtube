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

#ifndef DATABASEBRIDGE_H
#define DATABASEBRIDGE_H

#include "appruntime.h"

#include <QByteArray>
#include <QByteArrayList>
#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

class DatabaseBridge {
private:
    const static QByteArray placeholderPattern;
public:
    static QSqlDatabase database ();
    static bool commitTransaction (QSqlDatabase& db, const QByteArray& msg = QByteArray());
    static void closeDatabase (QSqlDatabase& db);
    static bool warnSqlError (const QSqlQuery& query, const QByteArray& prefix = QByteArray());
    static bool warnSqlError (const QSqlError& err, const QByteArray& prefix = QByteArray());
    static bool genericInsert (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& fields);
    static QHash<QByteArray,QVariant> genericSelect (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QByteArray& fieldList);
    static QHash<QByteArray,QVariant> genericSelect (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QByteArrayList& fields);
    static bool genericUpdate (QSqlDatabase& database, const QByteArray& table, const QByteArray& searchColumnName, const QVariant& searchColumnValue, const QByteArray& updateColumnName, const QVariant& updateColumnValue, const bool& callGenericInsert = false);
    static bool genericUpdate (QSqlDatabase& database, const QByteArray& table, const QByteArray& searchColumnName, const QVariant& searchColumnValue, const QHash<QByteArray,QVariant>& updateFields, const bool& callGenericInsert = false);
    static bool genericUpdate (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QHash<QByteArray,QVariant>& updateFields, const bool& callGenericInsert = false);
};

#endif // DATABASEBRIDGE_H
