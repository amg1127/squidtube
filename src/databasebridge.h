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
    const static QString placeholderPattern;
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
