#include "databasebridge.h"

const QString databaseBridge::placeholderPattern(":val%1");

QSqlDatabase databaseBridge::database () {
    int myDbInstance;
    QString dbDriver;
    QString dbHost;
    QString dbPort;
    QString dbUser;
    QString dbPassword;
    QString dbOptions;
    QString dbName;
    QStringList dbStartupQueries;
    {
        QMutexLocker locker (&AppRuntime::dbSettingsMutex);
        dbDriver = AppRuntime::dbDriver;
        dbHost = AppRuntime::dbHost;
        dbPort = AppRuntime::dbPort;
        dbUser = AppRuntime::dbUser;
        dbPassword = AppRuntime::dbPassword;
        dbOptions = AppRuntime::dbOptions;
        dbStartupQueries = AppRuntime::dbStartupQueries;
        myDbInstance = AppRuntime::dbInstance++;
    }

    QSqlDatabase sqldb (QSqlDatabase::addDatabase (dbDriver, QString("dbconn_%1").arg(myDbInstance)));
    if (sqldb.isValid()) {
        sqldb.setHostName (dbHost);
        sqldb.setPort (dbPort.toInt ());
        sqldb.setUserName (dbUser);
        sqldb.setPassword (dbPassword);
        sqldb.setConnectOptions (dbOptions);
        sqldb.setDatabaseName (dbName);
        if (sqldb.open ()) {
            while (! dbStartupQueries.isEmpty ()) {
                QString queryString(dbStartupQueries.takeFirst());
                QSqlQuery query (sqldb.exec (queryString + ";"));
                if (databaseBridge::warnSqlError (query, "Unable to submit startup queries onto database connection")) {
                    query.clear ();
                    dbStartupQueries.prepend (queryString);
                    break;
                }
            }
            if (dbStartupQueries.isEmpty ()) {
                return (sqldb);
            }
        } else {
            databaseBridge::warnSqlError (sqldb.lastError(), "Unable to open a connection to the database");
        }
        databaseBridge::closeDatabase (sqldb);
    }
    return (sqldb);
}

bool databaseBridge::commitTransaction (QSqlDatabase& db, const QString& msg) {
    for (int ntries = 0; ntries < 10; ntries++) {
        if (ntries) {
            QThread::msleep (500);
        }
        if (db.commit ()) {
            return (true);
        } else {
            databaseBridge::warnSqlError (db.lastError(), QString("Unable to commit %1 transaction").arg(msg.isEmpty() ? "a" : msg));
        }
    }
    if (! db.rollback ()) {
        databaseBridge::warnSqlError (db.lastError(), QString("Unable to rollback %1 transaction").arg(msg.isEmpty() ? "a" : msg));
    }
    return (false);
}

void databaseBridge::closeDatabase (QSqlDatabase& db) {
    QSqlDatabase invalid;
    QString dbConnectionName (db.connectionName ());
    if (db.isOpen()) {
        db.close ();
    }
    db = invalid;
    QSqlDatabase::removeDatabase (dbConnectionName);
}

bool databaseBridge::warnSqlError (const QSqlQuery& query, const QString& prefix) {
    bool status = databaseBridge::warnSqlError (query.lastError(), prefix);
    if (status) {
        QString q_sql (query.lastQuery());
        if (! q_sql.isEmpty ())
            qDebug() << QString("SQL query that failed: %1").arg(q_sql);
    }
    return (status);
}

bool databaseBridge::warnSqlError (const QSqlError& err, const QString& prefix) {
    QString msg;
    if (err.isValid ()) {
        QSqlError::ErrorType et = err.type ();
        if (prefix.isEmpty ()) {
            msg = "There is something wrong with the database or the SQL query: ";
        } else {
            msg = prefix + ": ";
        }
        switch (et) {
            case QSqlError::NoError: msg += "NoError: "; break;
            case QSqlError::ConnectionError: msg += "ConnectionError: "; break;
            case QSqlError::StatementError: msg += "StatementError: "; break;
            case QSqlError::TransactionError: msg += "TransactionError: "; break;
            default: msg += "UnknownError: "; break;
        }
        msg += QString("%1: %2").arg(err.number()).arg(err.text());
        if (et == QSqlError::StatementError)
            qCritical() << msg;
        else
            qWarning() << msg;
        return (true);
    } else {
        return (false);
    }
}

bool databaseBridge::genericInsert (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& fields) {
    QString sqlinstr ("INSERT INTO %1 (%2) VALUES (%3);");
    QStringList placeHolders;
    const QStringList fieldKeys (fields.uniqueKeys ());
    int numFields = fieldKeys.count ();
    if (numFields < 1) {
        qCritical() << "Invalid procedure call: 'fields' is empty!";
        return (false);
    }
    int pos;
    for (pos = 0; pos < numFields; pos++) {
        placeHolders.append(databaseBridge::placeholderPattern.arg(pos));
    }
    QSqlQuery query (database);
    if (query.prepare (sqlinstr.arg(table).arg(fieldKeys.join(", ")).arg(placeHolders.join(", ")))) {
        for (pos = 0; pos < numFields; pos++) {
            query.bindValue (placeHolders[pos], fields[fieldKeys[pos]]);
        }
        if (query.exec ()) {
            query.clear ();
            return (true);
        } else {
            databaseBridge::warnSqlError (query, QString("Unable to execute insertion into table '%1'").arg(table));
        }
    } else {
        databaseBridge::warnSqlError (query, QString("Unable to prepare insertion into table '%1'").arg(table));
    }
    query.clear ();
    return (false);
}

QHash<QString,QVariant> databaseBridge::genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QString fieldList) {
    return (databaseBridge::genericSelect (database, table, searchFields, fieldList.split(",")));
}

QHash<QString,QVariant> databaseBridge::genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QStringList fields) {
    QHash<QString,QVariant> returnValue;
    QString sqlinstr ("SELECT %1 FROM %2 WHERE %3;");
    QStringList placeholders;
    const QStringList fieldKeys (searchFields.uniqueKeys ());
    int numFields = fieldKeys.count ();
    if (numFields < 1) {
        qCritical() << "Invalid procedure call: 'searchFields' is empty!";
    } else if (fields.isEmpty ()) {
        qCritical() << "Invalid procedure call: 'fields' is empty!";
    } else {
        int pos;
        for (pos = 0; pos < numFields; pos++) {
            if (searchFields[fieldKeys[pos]].isNull ()) {
                placeholders.append (fieldKeys[pos] + " IS NULL");
            } else {
                placeholders.append (fieldKeys[pos] + " = " + databaseBridge::placeholderPattern.arg(pos));
            }
        }
        QSqlQuery query (database);
        if (query.prepare (sqlinstr.arg(fields.join(", ")).arg(table).arg(placeholders.join(" AND ")))) {
            for (pos = 0; pos < numFields; pos++) {
                if (! searchFields[fieldKeys[pos]].isNull ()) {
                    query.bindValue (databaseBridge::placeholderPattern.arg(pos), searchFields[fieldKeys[pos]]);
                }
            }
            if (query.exec ()) {
                if (query.next ()) {
                    numFields = fields.count ();
                    for (pos = 0; pos < numFields; pos++) {
                        returnValue[fields[pos]] = fields.value(pos);
                    }
                    if (query.next ()) {
                        qWarning() << QString("Data retrieval from table '%1' returned more than a single result").arg(table);
                    }
                } else {
                    qWarning() << QString("Data retrieval from table '%1' returned no result").arg(table);
                }
            } else {
                databaseBridge::warnSqlError (query, QString("Unable to execute data retrieval from table '%1'").arg(table));
            }
        } else {
            databaseBridge::warnSqlError (query, QString("Unable to prepare data retrieval from table '%1'").arg(table));
        }
        query.clear ();
    }
    return (returnValue);
}

bool databaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QString& updateColumnName, const QVariant& updateColumnValue, const bool& callGenericInsert) {
    QHash<QString,QVariant> updateFields;
    updateFields.insert (updateColumnName, updateColumnValue);
    return (databaseBridge::genericUpdate (database, table, searchColumnName, searchColumnValue, updateFields, callGenericInsert));
}

bool databaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert) {
    QHash<QString,QVariant> searchFields;
    searchFields.insert (searchColumnName, searchColumnValue);
    return (databaseBridge::genericUpdate (database, table, searchFields, updateFields, callGenericInsert));
}

bool databaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert) {
    int pos;
    bool returnValue = false;
    QString fieldName;
    const QStringList searchFieldsKeys (searchFields.uniqueKeys ());
    const QStringList updateFieldsKeys (updateFields.uniqueKeys ());
    int numSearchFields = searchFieldsKeys.count ();
    int numUpdateFields = updateFieldsKeys.count ();
    if (numSearchFields < 1) {
        qCritical() << "Invalid procedure call: 'searchFields' is empty!";
    } else if (numUpdateFields < 1) {
        qCritical() << "Invalid procedure call: 'updateFields' is empty!";
    } else {
        QString sqlinstr("UPDATE %1 SET %2 WHERE %3;");
        QStringList placeholdersSearch, placeholdersUpdate;
        for (pos = 0; pos < numSearchFields; pos++) {
            if (searchFields[searchFieldsKeys[pos]].isNull()) {
                placeholdersSearch.append (searchFieldsKeys[pos] + " IS NULL");
            } else {
                placeholdersSearch.append (searchFieldsKeys[pos] + " = " + databaseBridge::placeholderPattern.arg(pos + numUpdateFields));
            }
        }
        for (pos = 0; pos < numUpdateFields; pos++) {
            placeholdersUpdate.append (updateFieldsKeys[pos] + " = " + databaseBridge::placeholderPattern.arg(pos));
        }

        QSqlQuery query (database);
        if (query.prepare (sqlinstr.arg(table).arg(placeholdersUpdate.join(", ").arg(placeholdersSearch.join(" AND "))))) {
            for (pos = 0; pos < numSearchFields; pos++) {
                if (! searchFields[searchFieldsKeys[pos]].isNull()) {
                    query.bindValue(databaseBridge::placeholderPattern.arg(pos + numUpdateFields), searchFields[searchFieldsKeys[pos]]);
                }
            }
            for (pos = 0; pos < numUpdateFields; pos++) {
                query.bindValue(databaseBridge::placeholderPattern.arg(pos), updateFields[updateFieldsKeys[pos]]);
            }
            if (query.exec ()) {
                returnValue = true;
                int numRowsAffected = query.numRowsAffected ();
                if (numRowsAffected < 0) {
                    qWarning() << QString("Unable to retrieve how many rows were affeted by the update against table '%1'!").arg(table);
                } else if (numRowsAffected > 1) {
                    qWarning() << QString("The update against table '%1' affected #%2 rows!").arg(table).arg(numRowsAffected);
                } else if (numRowsAffected == 0 && (! callGenericInsert)) {
                    qWarning() << QString("The update against table '%1' affected no rows!").arg(table);
                }
                if (numRowsAffected <= 0 && callGenericInsert) {
                    query.clear ();
                    QHash<QString,QVariant> fields;
                    for (pos = 0; pos < numSearchFields; pos++) {
                        fields[searchFieldsKeys[pos]] = searchFields[searchFieldsKeys[pos]];
                    }
                    for (pos = 0; pos < numUpdateFields; pos++) {
                        fields[updateFieldsKeys[pos]] = updateFields[updateFieldsKeys[pos]];
                    }
                    return (databaseBridge::genericInsert (database, table, fields));
                }
            } else {
                databaseBridge::warnSqlError (query, QString("Unable to execute data update from table '%1'").arg(table));
            }
        } else {
            databaseBridge::warnSqlError (query, QString("Unable to prepare data update from table '%1'").arg(table));
        }
        query.clear ();
    }
    return (returnValue);
}

bool databaseBridge::createTables (QSqlDatabase& database, QString helper) {
    bool returnValue = true;
    QString table(QString("helper_") + helper);
    if (! database.tables().contains (table, Qt::CaseInsensitive)) {
        returnValue = false;
        QSqlQuery query (database);
        if (query.exec (QString("CREATE TABLE %1 (ktype VARCHAR(255) NOT NULL, kname VARCHAR(255) NOT NULL, value TEXT NULL, timestamp INTEGER NOT NULL, PRIMARY KEY (ktype, kname));").arg(table))) {
            returnValue = true;
        } else {
            databaseBridge::warnSqlError (query, QString("Unable to create table '%1'").arg(table));
        }
        query.clear ();
    }
    return (returnValue);
}
