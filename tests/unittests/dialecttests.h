#ifndef DIALECTTESTS_H
#define DIALECTTESTS_H

#include <QtTest/QtTest>

#include <qisqlstatement.h>
#include <qisqlitestatement.h>
#include <qimysqlstatement.h>
#include <qipgstatement.h>
#include <qimssqlstatement.h>
#include <qioraclestatement.h>
#include <qimodelmetainfo.h>

#include "model1.h"

/// Dialect SQL-generation tests. These check the SQL *text* each backend
/// produces (SQLite / MySQL / Postgres / SQL Server / Oracle) and need no
/// running database — a live server is only required for the integration
/// tests (see tests/integration/README.md), and only MySQL/Postgres/SQL
/// Server are wired into that live suite so far (Oracle's QOCI driver isn't
/// reliably available in CI yet).
class DialectTests : public QObject
{
    Q_OBJECT

public:
    explicit DialectTests(QObject *parent = nullptr) : QObject(parent) {}

private Q_SLOTS:
    /// forDriver() maps Qt driver names to the right generator (with SQLite fallback).
    void factory();

    /// Column type mapping differs per dialect (INTEGER / INT, TEXT / VARCHAR, ...).
    void columnTypes();

    /// MySQL keyed strings become VARCHAR (indexable); plain strings become TEXT (no truncation).
    /// SQL Server (NVARCHAR/NVARCHAR(MAX)) and Oracle (VARCHAR2/CLOB) split the same way.
    void mysqlStringTyping();

    /// float (single precision), QTime, and native JSON / JSONB mappings.
    void extraTypes();

    /// Primary-key auto-increment clause differs per dialect.
    void primaryKey();

    /// CREATE TABLE reflects each dialect's types / auto-increment / table options.
    void createTable();

    /// Upsert: MySQL uses ON DUPLICATE KEY UPDATE; SQLite/Postgres use ON CONFLICT;
    /// SQL Server/Oracle use MERGE.
    void upsert();

    /// How each dialect reports the id of the row just inserted: SQLite/MySQL use
    /// QSqlQuery::lastInsertId(), Postgres asks with "SELECT lastval()", SQL Server
    /// with "SELECT SCOPE_IDENTITY()", and Oracle — the one dialect where a session-
    /// scoped id doesn't exist — with its per-table companion sequence's CURRVAL.
    void lastInsertIdStrategy();

    /// Table-exists probe: SQLite reads sqlite_master; MySQL/Postgres/SQL Server read
    /// information_schema; Oracle (no information_schema) reads user_tables.
    void tableExists();

    /// SQL Server's MERGE-based upsert needs its trailing ";" kept; every other
    /// dialect (including Oracle's own MERGE) doesn't care either way.
    void statementTerminator();
};

#endif // DIALECTTESTS_H
