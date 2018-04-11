#include "databasebridge.h"

const QString DatabaseBridge::placeholderPattern(":val%1");

QSqlDatabase DatabaseBridge::database () {
    int myDbInstance;
    QString dbDriver;
    QString dbHost;
    QString dbPort;
    QString dbUser;
    QString dbPassword;
    QString dbOptions;
    QString dbName;
    QStringList dbStartupQueries;
    dbStartupQueries << QString();
    {
        QMutexLocker locker (&AppRuntime::dbSettingsMutex);
        myDbInstance = AppRuntime::dbInstance++;
        // Force a deep copy of the database configuration
        // http://doc.qt.io/qt-5/implicit-sharing.html
        dbDriver = QString("%1").arg(AppRuntime::dbDriver);
        dbHost = QString("%1").arg(AppRuntime::dbHost);
        dbPort = QString("%1").arg(AppRuntime::dbPort);
        dbUser = QString("%1").arg(AppRuntime::dbUser);
        dbPassword = QString("%1").arg(AppRuntime::dbPassword);
        dbName = QString("%1").arg(AppRuntime::dbName);
        dbOptions = QString("%1").arg(AppRuntime::dbOptions);
        dbStartupQueries << AppRuntime::dbStartupQueries;
    }
    dbStartupQueries.removeFirst ();
    qDebug() << QString("Opening a new database connection #%1...").arg(myDbInstance);
    QSqlDatabase sqldb (QSqlDatabase::addDatabase (dbDriver, QString("C%1").arg(myDbInstance)));
    if (sqldb.isValid()) {
        if (! dbHost.isEmpty ()) {
            sqldb.setHostName (dbHost);
        }
        int port = dbPort.toInt ();
        if (port) {
            sqldb.setPort (dbPort.toInt ());
        }
        if (! dbUser.isEmpty ()) {
            sqldb.setUserName (dbUser);
        }
        if (! dbPassword.isEmpty ()) {
            sqldb.setPassword (dbPassword);
        }
        if (! dbName.isEmpty ()) {
            sqldb.setDatabaseName (dbName);
        }
        if (! dbOptions.isEmpty ()) {
            sqldb.setConnectOptions (dbOptions);
        }
        if (! dbName.isEmpty ()) {
            sqldb.setDatabaseName (dbName);
        }
        if (sqldb.open ()) {
            int posQuery = 0;
            while (! dbStartupQueries.isEmpty ()) {
                posQuery++;
                QString queryString(dbStartupQueries.takeFirst());
                qDebug() << QString("Running startup query #%1 against the database...").arg(posQuery);
                QSqlQuery query (sqldb.exec (queryString + ";"));
                if (DatabaseBridge::warnSqlError (query, "Unable to submit startup queries onto database connection")) {
                    query.clear ();
                    dbStartupQueries.prepend (queryString);
                    break;
                } else {
                    QVariant field;
                    QStringList values;
                    int posField = 0;
                    while (query.next ()) {
                        for (field = query.value (posField); field.isValid(); field = query.value (++posField)) {
                            values.append (field.toString());
                        }
                        qDebug() << QString("Startup query #%1 returned the following value(s): '%2'").arg(posQuery).arg(values.join("', '"));
                        posField = 0;
                        values.clear ();
                    }
                    query.clear ();
                }
            }
            if (dbStartupQueries.isEmpty ()) {
                return (sqldb);
            }
        } else {
            DatabaseBridge::warnSqlError (sqldb.lastError(), "Unable to open a connection to the database");
        }
        DatabaseBridge::closeDatabase (sqldb);
    }
    return (sqldb);
}

bool DatabaseBridge::commitTransaction (QSqlDatabase& db, const QString& msg) {
    for (int ntries = 0; ntries < 10; ntries++) {
        if (ntries) {
            QThread::msleep (500);
        }
        if (db.commit ()) {
            return (true);
        } else {
            DatabaseBridge::warnSqlError (db.lastError(), QString("Unable to commit %1 transaction").arg(msg.isEmpty() ? "a" : msg));
        }
    }
    if (! db.rollback ()) {
        DatabaseBridge::warnSqlError (db.lastError(), QString("Unable to rollback %1 transaction").arg(msg.isEmpty() ? "a" : msg));
    }
    return (false);
}

void DatabaseBridge::closeDatabase (QSqlDatabase& db) {
    QSqlDatabase invalid;
    QString dbConnectionName (db.connectionName ());
    if (db.isOpen()) {
        db.close ();
        qDebug() << QString("Database connection '%1' is now closed.").arg(dbConnectionName);
    }
    db = invalid;
    QSqlDatabase::removeDatabase (dbConnectionName);
}

bool DatabaseBridge::warnSqlError (const QSqlQuery& query, const QString& prefix) {
    bool status = DatabaseBridge::warnSqlError (query.lastError(), prefix);
    if (status) {
        QString q_sql (query.lastQuery());
        if (! q_sql.isEmpty ())
            qDebug() << QString("SQL query that failed: %1").arg(q_sql);
    }
    return (status);
}

bool DatabaseBridge::warnSqlError (const QSqlError& err, const QString& prefix) {
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
        msg += QString("%1: %2").arg(err.nativeErrorCode()).arg(err.text());
        if (et == QSqlError::StatementError)
            qCritical() << msg;
        else
            qWarning() << msg;
        return (true);
    } else {
        return (false);
    }
}

bool DatabaseBridge::genericInsert (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& fields) {
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
        placeHolders.append(DatabaseBridge::placeholderPattern.arg(pos));
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
            DatabaseBridge::warnSqlError (query, QString("Unable to execute insertion into table '%1'").arg(table));
        }
    } else {
        DatabaseBridge::warnSqlError (query, QString("Unable to prepare insertion into table '%1'").arg(table));
    }
    query.clear ();
    return (false);
}

QHash<QString,QVariant> DatabaseBridge::genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QString& fieldList) {
    return (DatabaseBridge::genericSelect (database, table, searchFields, fieldList.split(",")));
}

QHash<QString,QVariant> DatabaseBridge::genericSelect (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QStringList& fields) {
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
                placeholders.append (fieldKeys[pos] + " = " + DatabaseBridge::placeholderPattern.arg(pos));
            }
        }
        QSqlQuery query (database);
        if (query.prepare (sqlinstr.arg(fields.join(", ")).arg(table).arg(placeholders.join(" AND ")))) {
            for (pos = 0; pos < numFields; pos++) {
                if (! searchFields[fieldKeys[pos]].isNull ()) {
                    query.bindValue (DatabaseBridge::placeholderPattern.arg(pos), searchFields[fieldKeys[pos]]);
                }
            }
            if (query.exec ()) {
                if (query.next ()) {
                    numFields = fields.count ();
                    for (pos = 0; pos < numFields; pos++) {
                        returnValue[fields[pos]] = query.value(pos);
                    }
                    if (query.next ()) {
                        qInfo() << QString("Data retrieval from table '%1' returned more than a single result").arg(table);
                    }
                } else {
                    qDebug() << QString("Data retrieval from table '%1' returned no result").arg(table);
                }
            } else {
                DatabaseBridge::warnSqlError (query, QString("Unable to execute data retrieval from table '%1'").arg(table));
            }
        } else {
            DatabaseBridge::warnSqlError (query, QString("Unable to prepare data retrieval from table '%1'").arg(table));
        }
        query.clear ();
    }
    return (returnValue);
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QString& updateColumnName, const QVariant& updateColumnValue, const bool& callGenericInsert) {
    QHash<QString,QVariant> updateFields;
    updateFields.insert (updateColumnName, updateColumnValue);
    return (DatabaseBridge::genericUpdate (database, table, searchColumnName, searchColumnValue, updateFields, callGenericInsert));
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QString& searchColumnName, const QVariant& searchColumnValue, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert) {
    QHash<QString,QVariant> searchFields;
    searchFields.insert (searchColumnName, searchColumnValue);
    return (DatabaseBridge::genericUpdate (database, table, searchFields, updateFields, callGenericInsert));
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QString& table, const QHash<QString,QVariant>& searchFields, const QHash<QString,QVariant>& updateFields, const bool& callGenericInsert) {
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
                placeholdersSearch.append (searchFieldsKeys[pos] + " = " + DatabaseBridge::placeholderPattern.arg(pos + numUpdateFields));
            }
        }
        for (pos = 0; pos < numUpdateFields; pos++) {
            placeholdersUpdate.append (updateFieldsKeys[pos] + " = " + DatabaseBridge::placeholderPattern.arg(pos));
        }

        QSqlQuery query (database);
        if (query.prepare (sqlinstr.arg(table).arg(placeholdersUpdate.join(", ")).arg(placeholdersSearch.join(" AND ")))) {
            for (pos = 0; pos < numSearchFields; pos++) {
                if (! searchFields[searchFieldsKeys[pos]].isNull()) {
                    query.bindValue(DatabaseBridge::placeholderPattern.arg(pos + numUpdateFields), searchFields[searchFieldsKeys[pos]]);
                }
            }
            for (pos = 0; pos < numUpdateFields; pos++) {
                query.bindValue(DatabaseBridge::placeholderPattern.arg(pos), updateFields[updateFieldsKeys[pos]]);
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
                    return (DatabaseBridge::genericInsert (database, table, fields));
                }
            } else {
                DatabaseBridge::warnSqlError (query, QString("Unable to execute data update from table '%1'").arg(table));
            }
        } else {
            DatabaseBridge::warnSqlError (query, QString("Unable to prepare data update from table '%1'").arg(table));
        }
        query.clear ();
    }
    return (returnValue);
}
