#ifndef DIALECTTESTS_H
#define DIALECTTESTS_H

#include <QtTest/QtTest>

#include <qisqlstatement.h>
#include <qisqlitestatement.h>
#include <qimysqlstatement.h>
#include <qipgstatement.h>
#include <qimodelmetainfo.h>

#include "model1.h"

/// Dialect SQL-generation tests. These check the SQL *text* each backend
/// produces (SQLite / MySQL / Postgres) and need no running database — a live
/// MySQL/Postgres server is only required for the integration tests (see
/// tests/integration/README.md).
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
    void mysqlStringTyping();

    /// float (single precision), QTime, and native JSON / JSONB mappings.
    void extraTypes();

    /// Primary-key auto-increment clause differs per dialect.
    void primaryKey();

    /// CREATE TABLE reflects each dialect's types / auto-increment / table options.
    void createTable();

    /// Upsert: MySQL uses ON DUPLICATE KEY UPDATE; SQLite/Postgres use ON CONFLICT.
    void upsert();

    /// Postgres asks for the new id with RETURNING; others use lastInsertId().
    void pgReturning();

    /// Table-exists probe: SQLite reads sqlite_master; MySQL/Postgres information_schema.
    void tableExists();
};

#endif // DIALECTTESTS_H
