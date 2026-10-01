#ifndef SCHEMATESTS_H
#define SCHEMATESTS_H

#include <QtTest/QtTest>
#include <QSqlDatabase>

/// QiSchema against a real SQLite catalog: a small bookstore schema built
/// with raw SQL (not Qivot models), so every case is exactly what an
/// arbitrary existing database could contain.
class SchemaTests : public QObject
{
    Q_OBJECT

public:
    explicit SchemaTests(QObject *parent = nullptr) : QObject(parent) {}

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    /// Tables by default; views/virtual and FTS shadow tables only on request.
    void listsTablesViewsAndInternal();

    /// Declared types, NOT NULL, defaults, and the rowid alias as auto-increment.
    void columns();

    /// Composite primary key, reported in key order.
    void compositePrimaryKey();

    /// Foreign keys with actions; a reference naming no column resolves to the parent's key.
    void foreignKeys();

    /// Explicit CREATE INDEX vs the implicit index behind a PRIMARY KEY / UNIQUE.
    void indexes();

    /// Views report columns but no keys or indexes.
    void views();

    void rowCount();

    /// Unknown table: invalid result and an error message, never a crash.
    void missingTable();

    /// Identifiers with spaces / reserved words are quoted correctly throughout.
    void awkwardNames();

private:
    QSqlDatabase m_db;
    bool         m_hasFts5 = false;
};

#endif // SCHEMATESTS_H
