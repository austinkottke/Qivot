#include "dialecttests.h"

static QiModelMetaInfo *model1Info() { return qiMetaInfo<Model1>(); }

void DialectTests::factory() {
    QScopedPointer<QiSqlStatement> lite(QiSqlStatement::forDriver("QSQLITE"));
    QScopedPointer<QiSqlStatement> my(QiSqlStatement::forDriver("QMYSQL"));
    QScopedPointer<QiSqlStatement> maria(QiSqlStatement::forDriver("QMARIADB"));
    QScopedPointer<QiSqlStatement> pg(QiSqlStatement::forDriver("QPSQL"));
    QScopedPointer<QiSqlStatement> mssql(QiSqlStatement::forDriver("QODBC"));
    QScopedPointer<QiSqlStatement> ora(QiSqlStatement::forDriver("QOCI"));
    // QODBC is a real dialect now, so it can no longer stand in for "unknown driver".
    QScopedPointer<QiSqlStatement> unknown(QiSqlStatement::forDriver("QSOMETHINGELSE"));

    QCOMPARE(lite->driverName(),  QString("SQLITE"));
    QCOMPARE(my->driverName(),    QString("MYSQL"));
    QCOMPARE(maria->driverName(), QString("MYSQL"));   // MariaDB shares the MySQL generator
    QCOMPARE(pg->driverName(),    QString("PSQL"));
    QCOMPARE(mssql->driverName(), QString("MSSQL"));
    QCOMPARE(ora->driverName(),   QString("ORACLE"));
    QCOMPARE(unknown->driverName(), QString("SQLITE")); // unknown driver -> SQLite fallback
}

void DialectTests::columnTypes() {
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    // integers
    QCOMPARE(lite.columnTypeName(QMetaType::Int), QString("INTEGER"));
    QCOMPARE(my.columnTypeName(QMetaType::Int),   QString("INT"));
    QCOMPARE(pg.columnTypeName(QMetaType::Int),   QString("INTEGER"));
    QCOMPARE(mssql.columnTypeName(QMetaType::Int),QString("INT"));
    QCOMPARE(ora.columnTypeName(QMetaType::Int),  QString("NUMBER(10)"));

    // strings
    QCOMPARE(lite.columnTypeName(QMetaType::QString), QString("TEXT"));
    QCOMPARE(my.columnTypeName(QMetaType::QString),   QString("VARCHAR(255)"));
    QCOMPARE(pg.columnTypeName(QMetaType::QString),   QString("TEXT"));
    QCOMPARE(mssql.columnTypeName(QMetaType::QString),QString("NVARCHAR(MAX)"));
    QCOMPARE(ora.columnTypeName(QMetaType::QString),  QString("VARCHAR2(255)"));

    // bool + blob
    QCOMPARE(my.columnTypeName(QMetaType::Bool),      QString("TINYINT(1)"));
    QCOMPARE(pg.columnTypeName(QMetaType::Bool),      QString("BOOLEAN"));
    QCOMPARE(pg.columnTypeName(QMetaType::QByteArray),QString("BYTEA"));
    QCOMPARE(mssql.columnTypeName(QMetaType::Bool),      QString("BIT"));
    QCOMPARE(mssql.columnTypeName(QMetaType::QByteArray),QString("VARBINARY(MAX)"));
    QCOMPARE(ora.columnTypeName(QMetaType::Bool),      QString("NUMBER(1)"));
    QCOMPARE(ora.columnTypeName(QMetaType::QByteArray),QString("BLOB"));
}

void DialectTests::mysqlStringTyping() {
    // Every dialect that can't index an unbounded text type (MySQL's TEXT, SQL
    // Server's NVARCHAR(MAX), Oracle's CLOB) needs the same split: a keyed/unique
    // string gets a bounded, indexable type; any other string doesn't truncate.
    QiMysqlStatement  my;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;
    QiClause plain;                        // no flags
    QiClause pk(QiClause::PRIMARY_KEY);
    QiClause uniq(QiClause::UNIQUE);

    // Plain text must not truncate -> TEXT.
    QCOMPARE(my.columnTypeForField(QMetaType::QString, plain), QString("TEXT"));
    // A keyed / unique string must be indexable -> bounded VARCHAR.
    QCOMPARE(my.columnTypeForField(QMetaType::QString, pk),   QString("VARCHAR(255)"));
    QCOMPARE(my.columnTypeForField(QMetaType::QString, uniq), QString("VARCHAR(255)"));
    // Non-string types are unaffected by the clause.
    QCOMPARE(my.columnTypeForField(QMetaType::Int, plain),    QString("INT"));

    QCOMPARE(mssql.columnTypeForField(QMetaType::QString, plain), QString("NVARCHAR(MAX)"));
    QCOMPARE(mssql.columnTypeForField(QMetaType::QString, pk),    QString("NVARCHAR(450)"));
    QCOMPARE(mssql.columnTypeForField(QMetaType::QString, uniq),  QString("NVARCHAR(450)"));

    QCOMPARE(ora.columnTypeForField(QMetaType::QString, plain), QString("CLOB"));
    QCOMPARE(ora.columnTypeForField(QMetaType::QString, pk),    QString("VARCHAR2(255)"));
    QCOMPARE(ora.columnTypeForField(QMetaType::QString, uniq),  QString("VARCHAR2(255)"));
}

void DialectTests::extraTypes() {
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    // float (single precision)
    QCOMPARE(my.columnTypeName(QMetaType::Float), QString("FLOAT"));
    QCOMPARE(pg.columnTypeName(QMetaType::Float), QString("REAL"));
    QVERIFY(!lite.columnTypeName(QMetaType::Float).isNull());
    QCOMPARE(mssql.columnTypeName(QMetaType::Float), QString("REAL"));
    QCOMPARE(ora.columnTypeName(QMetaType::Float),   QString("BINARY_FLOAT"));

    // time
    QCOMPARE(my.columnTypeName(QMetaType::QTime),   QString("TIME"));
    QCOMPARE(pg.columnTypeName(QMetaType::QTime),   QString("TIME"));
    QCOMPARE(lite.columnTypeName(QMetaType::QTime), QString("TIME"));
    QCOMPARE(mssql.columnTypeName(QMetaType::QTime),QString("TIME"));
    // Oracle has no native TIME type; stored as a formatted string (see qioraclestatement.cpp).
    QCOMPARE(ora.columnTypeName(QMetaType::QTime),  QString("VARCHAR2(8)"));

    // JSON is stored as text on every backend (portable + parameter-safe round-trip)
    QCOMPARE(lite.columnTypeName(QMetaType::QJsonObject), QString("TEXT"));
    QCOMPARE(my.columnTypeName(QMetaType::QJsonObject),   QString("TEXT"));
    QCOMPARE(pg.columnTypeName(QMetaType::QJsonObject),   QString("TEXT"));
    QCOMPARE(mssql.columnTypeName(QMetaType::QJsonObject),QString("NVARCHAR(MAX)"));
    QCOMPARE(ora.columnTypeName(QMetaType::QJsonObject),  QString("CLOB"));
}

void DialectTests::primaryKey() {
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    QCOMPARE(my.primaryKeyClause("INT"),         QString("AUTO_INCREMENT PRIMARY KEY"));
    QCOMPARE(my.primaryKeyClause("VARCHAR(255)"),QString("PRIMARY KEY"));   // no auto-inc on text

    QVERIFY(pg.primaryKeyClause("INTEGER").contains("GENERATED BY DEFAULT AS IDENTITY"));
    QCOMPARE(pg.primaryKeyClause("TEXT"),        QString("PRIMARY KEY"));

    QCOMPARE(mssql.primaryKeyClause("INT"),            QString("IDENTITY(1,1) PRIMARY KEY"));
    QCOMPARE(mssql.primaryKeyClause("NVARCHAR(450)"),  QString("PRIMARY KEY"));

    // Oracle's key clause references a companion sequence by a placeholder that only
    // createTableIfNotExists() (which knows the table name) fills in — see createTable().
    QVERIFY(ora.primaryKeyClause("NUMBER(10)").contains("NEXTVAL PRIMARY KEY"));
    QCOMPARE(ora.primaryKeyClause("VARCHAR2(255)"), QString("PRIMARY KEY"));
}

void DialectTests::createTable() {
    QiModelMetaInfo *info = model1Info();
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    const QString liteS = lite.createTableIfNotExists(info);
    QVERIFY(liteS.contains("CREATE TABLE IF NOT EXISTS model1"));
    QVERIFY(liteS.contains("AUTOINCREMENT"));

    const QString myS = my.createTableIfNotExists(info);
    QVERIFY(myS.contains("CREATE TABLE IF NOT EXISTS model1"));
    QVERIFY(myS.contains("AUTO_INCREMENT PRIMARY KEY"));
    QVERIFY(myS.contains("ENGINE=InnoDB"));
    QVERIFY(myS.contains("TEXT"));              // plain (non-key) strings are TEXT, not truncated

    const QString pgS = pg.createTableIfNotExists(info);
    QVERIFY(pgS.contains("CREATE TABLE IF NOT EXISTS model1"));
    QVERIFY(pgS.contains("GENERATED BY DEFAULT AS IDENTITY"));
    QVERIFY(pgS.contains("TEXT"));

    // T-SQL has no "IF NOT EXISTS" on CREATE TABLE — QiMsSqlStatement drops it
    // (QiConnection::createTables() already checked exists() before calling this).
    const QString mssqlS = mssql.createTableIfNotExists(info);
    QVERIFY(mssqlS.contains("CREATE TABLE model1"));
    QVERIFY(!mssqlS.contains("IF NOT EXISTS"));
    QVERIFY(mssqlS.contains("IDENTITY(1,1) PRIMARY KEY"));
    QVERIFY(mssqlS.contains("NVARCHAR(MAX)"));

    // Oracle: one PL/SQL block creating the id column's companion sequence, then the
    // table itself — see the design note in qioraclestatement.cpp.
    const QString oraS = ora.createTableIfNotExists(info);
    QVERIFY(oraS.startsWith("BEGIN"));
    QVERIFY(oraS.trimmed().endsWith("END;"));
    QVERIFY(oraS.contains("CREATE SEQUENCE"));
    QVERIFY(oraS.contains("model1_id_seq"));
    QVERIFY(oraS.contains("CREATE TABLE model1"));
    QVERIFY(!oraS.contains("IF NOT EXISTS"));
    QVERIFY(oraS.contains("NEXTVAL PRIMARY KEY"));
    QVERIFY(oraS.contains("CLOB"));             // plain (non-key) strings
}

void DialectTests::upsert() {
    QiModelMetaInfo *info = model1Info();
    const QStringList fields   = QStringList() << "id" << "key" << "value";
    const QStringList conflict = QStringList() << "key";

    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    const QString myS = my.upsertInto(info, fields, conflict);
    QVERIFY(myS.contains("ON DUPLICATE KEY UPDATE"));
    QVERIFY(myS.contains("value=VALUES(value)"));

    const QString pgS = pg.upsertInto(info, fields, conflict);
    QVERIFY(pgS.contains("ON CONFLICT(key)"));
    QVERIFY(pgS.contains("value=excluded.value"));

    const QString liteS = lite.upsertInto(info, fields, conflict);
    QVERIFY(liteS.contains("ON CONFLICT(key)"));

    // Neither SQL Server nor Oracle has ON CONFLICT/ON DUPLICATE KEY — both need MERGE.
    const QString mssqlS = mssql.upsertInto(info, fields, conflict);
    QVERIFY(mssqlS.contains("MERGE INTO model1"));
    QVERIFY(mssqlS.contains("ON (target.key = source.key)"));
    QVERIFY(mssqlS.contains("WHEN MATCHED THEN UPDATE SET value = source.value"));
    QVERIFY(mssqlS.contains("WHEN NOT MATCHED THEN INSERT (id, key, value) VALUES (source.id, source.key, source.value)"));
    QVERIFY(mssqlS.trimmed().endsWith(";"));   // T-SQL's MERGE requires the terminator

    const QString oraS = ora.upsertInto(info, fields, conflict);
    QVERIFY(oraS.contains("MERGE INTO model1"));
    QVERIFY(oraS.contains("FROM DUAL"));
    QVERIFY(oraS.contains("ON (target.key = source.key)"));
    QVERIFY(oraS.contains("WHEN MATCHED THEN UPDATE SET value = source.value"));
    QVERIFY(!oraS.trimmed().endsWith(";"));    // Oracle's MERGE doesn't need one

    // save() uses REPLACE INTO. SQLite/MySQL support it; Postgres/SQL Server/Oracle
    // have no REPLACE, so it must become an upsert on the primary key instead.
    QVERIFY(lite.replaceInto(info, fields).contains("REPLACE INTO"));
    QVERIFY(my.replaceInto(info, fields).contains("REPLACE INTO"));
    const QString pgReplace = pg.replaceInto(info, fields);
    QVERIFY(!pgReplace.contains("REPLACE"));
    QVERIFY(pgReplace.contains("ON CONFLICT(id)"));
    const QString mssqlReplace = mssql.replaceInto(info, fields);
    QVERIFY(!mssqlReplace.contains("REPLACE"));
    QVERIFY(mssqlReplace.contains("ON (target.id = source.id)"));
    const QString oraReplace = ora.replaceInto(info, fields);
    QVERIFY(!oraReplace.contains("REPLACE"));
    QVERIFY(oraReplace.contains("ON (target.id = source.id)"));
}

void DialectTests::lastInsertIdStrategy() {
    QiModelMetaInfo *info = model1Info();
    QiPgStatement     pg;
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    // Postgres/SQL Server fetch the new id with a session-scoped follow-up query;
    // SQLite/MySQL use lastInsertId() instead (an empty lastInsertIdQuery() means that).
    QCOMPARE(pg.lastInsertIdQuery(),    QString("SELECT lastval()"));
    QCOMPARE(mssql.lastInsertIdQuery(), QString("SELECT SCOPE_IDENTITY()"));
    QVERIFY(lite.lastInsertIdQuery().isEmpty());
    QVERIFY(my.lastInsertIdQuery().isEmpty());

    // Oracle has no session-scoped "last id" — CURRVAL is per-sequence, so it needs
    // to know which table, hence the table-aware overload rather than the no-arg one.
    QVERIFY(ora.lastInsertIdQuery().isEmpty());
    QCOMPARE(ora.lastInsertIdQuery(info), QString("SELECT \"model1_id_seq\".CURRVAL FROM DUAL"));

    // Every other dialect's table-aware overload just forwards to its no-arg one —
    // exercised through the QiSqlStatement base pointer, the same way qisql.cpp always
    // calls it (a concrete instance's own scope hides one overload behind the other;
    // see the "using QiSqlStatement::lastInsertIdQuery;" comment in the headers).
    QiSqlStatement *pgBase = &pg;
    QiSqlStatement *mssqlBase = &mssql;
    QCOMPARE(pgBase->lastInsertIdQuery(info),    pg.lastInsertIdQuery());
    QCOMPARE(mssqlBase->lastInsertIdQuery(info), mssql.lastInsertIdQuery());
}

void DialectTests::tableExists() {
    QiModelMetaInfo *info = model1Info();
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    QVERIFY(lite.exists(info).contains("sqlite_master"));
    QVERIFY(my.exists(info).contains("information_schema"));
    QVERIFY(pg.exists(info).contains("information_schema"));
    QVERIFY(mssql.exists(info).contains("information_schema"));   // inherited default; SQL Server has it too
    QVERIFY(ora.exists(info).contains("user_tables"));            // no information_schema in Oracle
}

void DialectTests::statementTerminator() {
    QiSqliteStatement lite;
    QiMysqlStatement  my;
    QiPgStatement     pg;
    QiMsSqlStatement  mssql;
    QiOracleStatement ora;

    QVERIFY(!lite.keepsStatementTerminator());
    QVERIFY(!my.keepsStatementTerminator());
    QVERIFY(!pg.keepsStatementTerminator());
    QVERIFY(mssql.keepsStatementTerminator());   // MERGE is a syntax error without it
    QVERIFY(!ora.keepsStatementTerminator());    // Oracle's MERGE doesn't need one
}
