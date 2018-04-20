#include "databasebridge.h"

const QByteArray DatabaseBridge::placeholderPattern(":val%1");

QSqlDatabase DatabaseBridge::database () {
    unsigned int myDbInstance;
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
        // Also force a deep copy of the database configuration
        // http://doc.qt.io/qt-5/implicit-sharing.html
        dbDriver = QStringLiteral("%1").arg(AppRuntime::dbDriver);
        dbHost = QStringLiteral("%1").arg(AppRuntime::dbHost);
        dbPort = QStringLiteral("%1").arg(AppRuntime::dbPort);
        dbUser = QStringLiteral("%1").arg(AppRuntime::dbUser);
        dbPassword = QStringLiteral("%1").arg(AppRuntime::dbPassword);
        dbName = QStringLiteral("%1").arg(AppRuntime::dbName);
        dbOptions = QStringLiteral("%1").arg(AppRuntime::dbOptions);
        dbStartupQueries << AppRuntime::dbStartupQueries;
    }
    dbStartupQueries.removeFirst ();
    qDebug ("Opening a new database connection #%d...", myDbInstance);
    QSqlDatabase sqldb (QSqlDatabase::addDatabase (dbDriver, QStringLiteral("C%1").arg(myDbInstance)));
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
                qDebug ("Running startup query #%d against the database...", posQuery);
                QSqlQuery query (sqldb.exec (queryString + QStringLiteral(";")));
                if (DatabaseBridge::warnSqlError (query, "Unable to submit startup queries onto database connection")) {
                    query.clear ();
                    dbStartupQueries.prepend (queryString);
                    break;
                } else {
                    QVariant field;
                    QByteArrayList values;
                    int posField = 0;
                    while (query.next ()) {
                        for (field = query.value (posField); field.isValid(); field = query.value (++posField)) {
                            values.append (field.toByteArray());
                        }
                        qDebug ("Startup query #%d returned the following value(s): ('%s')", posQuery, values.join("', '").constData());
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

bool DatabaseBridge::commitTransaction (QSqlDatabase& db, const QByteArray& msg) {
    for (int ntries = 0; ntries < 10; ntries++) {
        if (ntries) {
            QThread::msleep (500);
        }
        if (db.commit ()) {
            return (true);
        } else {
            DatabaseBridge::warnSqlError (db.lastError(), QByteArray("Unable to commit ") + (msg.isEmpty() ? "a" : msg) + " transaction");
        }
    }
    if (! db.rollback ()) {
        DatabaseBridge::warnSqlError (db.lastError(), QByteArray("Unable to rollback ") + (msg.isEmpty() ? "a" : msg) + " transaction");
    }
    return (false);
}

void DatabaseBridge::closeDatabase (QSqlDatabase& db) {
    QSqlDatabase invalid;
    QString dbConnectionName (db.connectionName ());
    if (db.isOpen()) {
        db.close ();
        qDebug ("Database connection '%s' is now closed.", dbConnectionName.toLatin1().constData());
    }
    db = invalid;
    QSqlDatabase::removeDatabase (dbConnectionName);
}

bool DatabaseBridge::warnSqlError (const QSqlQuery& query, const QByteArray& prefix) {
    bool status = DatabaseBridge::warnSqlError (query.lastError(), prefix);
    if (status) {
        QString q_sql (query.lastQuery());
        if (! q_sql.isEmpty ())
            qDebug ("SQL query that failed: %s", q_sql.toLatin1().constData());
    }
    return (status);
}

bool DatabaseBridge::warnSqlError (const QSqlError& err, const QByteArray& prefix) {
    QByteArray msg;
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
        msg += err.nativeErrorCode().toLatin1() + ": " + err.text().toLatin1();
        if (et == QSqlError::StatementError)
            qCritical ("%s", msg.constData());
        else
            qWarning ("%s", msg.constData());
        return (true);
    } else {
        return (false);
    }
}

bool DatabaseBridge::genericInsert (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& fields) {
    QByteArrayList placeHolders;
    const QByteArrayList fieldKeys (fields.uniqueKeys ());
    int numFields = fieldKeys.count ();
    if (numFields < 1) {
        qFatal ("Invalid procedure call: 'fields' is empty!");
    }
    int pos;
    for (pos = 0; pos < numFields; pos++) {
        QByteArray placeholderName (DatabaseBridge::placeholderPattern);
        placeholderName.replace ("%1", QByteArray::number(pos));
        placeHolders.append(placeholderName);
    }
    QSqlQuery query (database);
    // INSERT INTO %1 (%2) VALUES (%3);
    QByteArray sqlinstr ("INSERT INTO ");
    sqlinstr += table + " (" + fieldKeys.join(", ") + ") VALUES (" + placeHolders.join(", ") + ");";
    if (query.prepare (QString::fromLocal8Bit (sqlinstr))) {
        for (pos = 0; pos < numFields; pos++) {
            query.bindValue (QString::fromLocal8Bit (placeHolders[pos]), fields[fieldKeys[pos]]);
        }
        if (query.exec ()) {
            query.clear ();
            return (true);
        } else {
            DatabaseBridge::warnSqlError (query, QByteArray("Unable to execute insertion into table '") + table + "'");
        }
    } else {
        DatabaseBridge::warnSqlError (query, QByteArray("Unable to prepare insertion into table '") + table + "'");
    }
    query.clear ();
    return (false);
}

QHash<QByteArray,QVariant> DatabaseBridge::genericSelect (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QByteArray& fieldList) {
    return (DatabaseBridge::genericSelect (database, table, searchFields, fieldList.split(',')));
}

QHash<QByteArray,QVariant> DatabaseBridge::genericSelect (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QByteArrayList& fields) {
    QHash<QByteArray,QVariant> returnValue;
    QByteArrayList placeholders;
    const QByteArrayList fieldKeys (searchFields.uniqueKeys ());
    int numFields = fieldKeys.count ();
    if (numFields < 1) {
        qFatal ("Invalid procedure call: 'searchFields' is empty!");
    } else if (fields.isEmpty ()) {
        qFatal ("Invalid procedure call: 'fields' is empty!");
    } else {
        int pos;
        for (pos = 0; pos < numFields; pos++) {
            if (searchFields[fieldKeys[pos]].isNull ()) {
                placeholders.append (fieldKeys[pos] + " IS NULL");
            } else {
                QByteArray placeholderName (DatabaseBridge::placeholderPattern);
                placeholderName.replace ("%1", QByteArray::number(pos));
                placeholders.append (fieldKeys[pos] + " = " + placeholderName);
            }
        }
        // SELECT %1 FROM %2 WHERE %3;
        QByteArray sqlinstr ("SELECT ");
        sqlinstr += fields.join(", ") + " FROM " + table + " WHERE " + placeholders.join(" AND ") + ";";
        QSqlQuery query (database);
        if (query.prepare (QString::fromLocal8Bit (sqlinstr))) {
            for (pos = 0; pos < numFields; pos++) {
                if (! searchFields[fieldKeys[pos]].isNull ()) {
                    query.bindValue (QString::fromLocal8Bit (DatabaseBridge::placeholderPattern).arg(pos), searchFields[fieldKeys[pos]]);
                }
            }
            if (query.exec ()) {
                if (query.next ()) {
                    numFields = fields.count ();
                    for (pos = 0; pos < numFields; pos++) {
                        returnValue[fields[pos]] = query.value(pos);
                    }
                    if (query.next ()) {
                        qInfo ("Data retrieval from table '%s' returned more than a single result.", table.constData());
                    }
                } else {
                    qDebug ("Data retrieval from table '%s' returned no result.", table.constData());
                }
            } else {
                DatabaseBridge::warnSqlError (query, QByteArray("Unable to execute data retrieval from table '") + table + "'");
            }
        } else {
            DatabaseBridge::warnSqlError (query, QByteArray("Unable to prepare data retrieval from table '") + table + "'");
        }
        query.clear ();
    }
    return (returnValue);
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QByteArray& table, const QByteArray& searchColumnName, const QVariant& searchColumnValue, const QByteArray& updateColumnName, const QVariant& updateColumnValue, const bool& callGenericInsert) {
    QHash<QByteArray,QVariant> updateFields;
    updateFields.insert (updateColumnName, updateColumnValue);
    return (DatabaseBridge::genericUpdate (database, table, searchColumnName, searchColumnValue, updateFields, callGenericInsert));
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QByteArray& table, const QByteArray& searchColumnName, const QVariant& searchColumnValue, const QHash<QByteArray,QVariant>& updateFields, const bool& callGenericInsert) {
    QHash<QByteArray,QVariant> searchFields;
    searchFields.insert (searchColumnName, searchColumnValue);
    return (DatabaseBridge::genericUpdate (database, table, searchFields, updateFields, callGenericInsert));
}

bool DatabaseBridge::genericUpdate (QSqlDatabase& database, const QByteArray& table, const QHash<QByteArray,QVariant>& searchFields, const QHash<QByteArray,QVariant>& updateFields, const bool& callGenericInsert) {
    int pos;
    bool returnValue = false;
    const QByteArrayList searchFieldsKeys (searchFields.uniqueKeys ());
    const QByteArrayList updateFieldsKeys (updateFields.uniqueKeys ());
    int numSearchFields = searchFieldsKeys.count ();
    int numUpdateFields = updateFieldsKeys.count ();
    if (numSearchFields < 1) {
        qFatal ("Invalid procedure call: 'searchFields' is empty!");
    } else if (numUpdateFields < 1) {
        qFatal ("Invalid procedure call: 'updateFields' is empty!");
    } else {
        QByteArrayList placeholdersSearch, placeholdersUpdate;
        for (pos = 0; pos < numSearchFields; pos++) {
            if (searchFields[searchFieldsKeys[pos]].isNull()) {
                placeholdersSearch.append (searchFieldsKeys[pos] + " IS NULL");
            } else {
                QByteArray placeholderName (DatabaseBridge::placeholderPattern);
                placeholderName.replace ("%1", QByteArray::number (pos + numUpdateFields));
                placeholdersSearch.append (searchFieldsKeys[pos] + " = " + placeholderName);
            }
        }
        for (pos = 0; pos < numUpdateFields; pos++) {
            QByteArray placeholderName (DatabaseBridge::placeholderPattern);
            placeholderName.replace ("%1", QByteArray::number (pos));
            placeholdersUpdate.append (updateFieldsKeys[pos] + " = " + placeholderName);
        }
        // UPDATE %1 SET %2 WHERE %3;
        QByteArray sqlinstr("UPDATE ");
        sqlinstr += table + " SET " + placeholdersUpdate.join(", ") + " WHERE " + placeholdersSearch.join(" AND ") + ";";
        QSqlQuery query (database);
        if (query.prepare (QString::fromLocal8Bit (sqlinstr))) {
            for (pos = 0; pos < numSearchFields; pos++) {
                if (! searchFields[searchFieldsKeys[pos]].isNull()) {
                    query.bindValue(QString::fromLocal8Bit(DatabaseBridge::placeholderPattern).arg(pos + numUpdateFields), searchFields[searchFieldsKeys[pos]]);
                }
            }
            for (pos = 0; pos < numUpdateFields; pos++) {
                query.bindValue(QString::fromLocal8Bit(DatabaseBridge::placeholderPattern).arg(pos), updateFields[updateFieldsKeys[pos]]);
            }
            if (query.exec ()) {
                returnValue = true;
                int numRowsAffected = query.numRowsAffected ();
                if (numRowsAffected < 0) {
                    qWarning ("Unable to retrieve how many rows were affeted by the update against table '%s'!", table.constData());
                } else if (numRowsAffected > 1) {
                    qWarning ("The update against table '%s' affected #%d rows!", table.constData(), numRowsAffected);
                } else if (numRowsAffected == 0 && (! callGenericInsert)) {
                    qWarning ("The update against table '%s' affected no rows!", table.constData());
                }
                if (numRowsAffected <= 0 && callGenericInsert) {
                    query.clear ();
                    QHash<QByteArray,QVariant> fields;
                    for (pos = 0; pos < numSearchFields; pos++) {
                        fields[searchFieldsKeys[pos]] = searchFields[searchFieldsKeys[pos]];
                    }
                    for (pos = 0; pos < numUpdateFields; pos++) {
                        fields[updateFieldsKeys[pos]] = updateFields[updateFieldsKeys[pos]];
                    }
                    return (DatabaseBridge::genericInsert (database, table, fields));
                }
            } else {
                DatabaseBridge::warnSqlError (query, QByteArray("Unable to execute data update from table '") + table + "'");
            }
        } else {
            DatabaseBridge::warnSqlError (query, QByteArray("Unable to prepare data update from table '") + table + "'");
        }
        query.clear ();
    }
    return (returnValue);
}
