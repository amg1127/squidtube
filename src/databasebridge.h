#ifndef DATABASEBRIDGE_H
#define DATABASEBRIDGE_H

#include "appruntime.h"

#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>
#include <QThread>

class DatabaseBridge {
private:
    const static QString placeholderPattern;
public:
    static QSqlDatabase database ();
    static bool commitTransaction (QSqlDatabase& db, const QString& msg = QString());
    static void closeDatabase (QSqlDatabase& db);
    static bool warnSqlError (const QSqlQuery& query, const QString& prefix = QString());
    static bool warnSqlError (const QSqlError& err, const QString& prefix = QString());
    static bool genericInsert (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& fields);
    static QHash<QString,QVariant> genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QString& fieldList);
    static QHash<QString,QVariant> genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QStringList& fields);
    static bool genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QString& updateColumnName, const QVariant& updateColumnValue, const bool& callGenericInsert = false);
    static bool genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert = false);
    static bool genericUpdate (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert = false);
};

#endif // DATABASEBRIDGE_H
