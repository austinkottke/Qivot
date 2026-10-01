#include "schematests.h"

#include <QSqlError>
#include <QSqlQuery>
#include <qischema.h>

static const char *kSchemaConnection = "qi_schema_tests";

static bool execAll(QSqlDatabase db, const QStringList &statements)
{
    QSqlQuery q(db);
    for (const QString &sql : statements) {
        if (!q.exec(sql)) {
            qWarning() << "schema setup failed:" << sql << q.lastError().text();
            return false;
        }
    }
    return true;
}

void SchemaTests::initTestCase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", kSchemaConnection);
    m_db.setDatabaseName(":memory:");
    QVERIFY(m_db.open());

    QVERIFY(execAll(m_db, {
        "PRAGMA foreign_keys = ON",
        "CREATE TABLE author ("
        "  id   INTEGER PRIMARY KEY,"
        "  name VARCHAR(80) NOT NULL,"
        "  born DATE DEFAULT '1900-01-01')",
        "CREATE TABLE book ("
        "  id        INTEGER PRIMARY KEY,"
        "  author_id INTEGER NOT NULL REFERENCES author(id) ON DELETE CASCADE,"
        "  editor_id INTEGER REFERENCES author ON UPDATE SET NULL,"   // no column named
        "  title     TEXT NOT NULL,"
        "  isbn      TEXT,"
        "  price     REAL)",
        "CREATE UNIQUE INDEX idx_book_isbn ON book(isbn)",
        "CREATE INDEX idx_book_author_title ON book(author_id, title)",
        "CREATE TABLE book_tag ("
        "  book_id INTEGER NOT NULL REFERENCES book(id),"
        "  tag     TEXT NOT NULL,"
        "  PRIMARY KEY (tag, book_id))",                   // key order differs from column order
        "CREATE VIEW v_titles AS SELECT b.title, a.name FROM book b JOIN author a ON a.id = b.author_id",
        "INSERT INTO author (name) VALUES ('Le Guin'), ('Pratchett')",
        "INSERT INTO book (author_id, title, isbn) VALUES (1, 'The Dispossessed', '978-0'),"
        "  (2, 'Small Gods', '978-1'), (2, 'Mort', '978-2')",
    }));

    // FTS5 is in Qt's bundled SQLite, but a system SQLite might lack it.
    QSqlQuery q(m_db);
    m_hasFts5 = q.exec("CREATE VIRTUAL TABLE book_fts USING fts5(title)");
}

void SchemaTests::cleanupTestCase()
{
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(kSchemaConnection);
}

void SchemaTests::listsTablesViewsAndInternal()
{
    QiSchema schema(m_db);
    QCOMPARE(schema.dialect(), QString("sqlite"));
    QVERIFY(schema.hasForeignKeyInfo());

    QCOMPARE(schema.tableNames(), QStringList({ "author", "book", "book_tag" }));

    const QStringList withViews = schema.tableNames(true);
    QVERIFY(withViews.contains("v_titles"));
    QVERIFY(!withViews.contains("sqlite_sequence"));

    if (m_hasFts5) {
        QVERIFY(withViews.contains("book_fts"));
        QCOMPARE(schema.table("book_fts").kind, QiTableInfo::Virtual);
        // Its shadow tables are storage, not data a user should browse.
        QVERIFY(!withViews.contains("book_fts_data"));
        QVERIFY(schema.tableNames(true, true).contains("book_fts_data"));
        QCOMPARE(schema.table("book_fts_data").kind, QiTableInfo::Internal);
    }

    const QVector<QiTableInfo> all = schema.tables();
    QCOMPARE(all.size(), 3);
    QCOMPARE(all.at(0).name, QString("author"));
}

void SchemaTests::columns()
{
    QiSchema schema(m_db);
    const QiTableInfo author = schema.table("author");
    QVERIFY(author.isValid());
    QCOMPARE(author.kind, QiTableInfo::Table);
    QCOMPARE(author.columns.size(), 3);

    const QiColumnInfo *id = author.column("id");
    QVERIFY(id);
    QCOMPARE(id->type, QString("INTEGER"));
    QVERIFY(id->primaryKey);
    QVERIFY(id->autoIncrement);                       // the rowid alias

    const QiColumnInfo *name = author.column("NAME");  // case-insensitive lookup
    QVERIFY(name);
    QCOMPARE(name->type, QString("VARCHAR(80)"));
    QVERIFY(!name->nullable);
    QVERIFY(!name->autoIncrement);
    QVERIFY(name->defaultValue.isNull());

    const QiColumnInfo *born = author.column("born");
    QVERIFY(born);
    QVERIFY(born->nullable);
    QCOMPARE(born->defaultValue.toString(), QString("'1900-01-01'"));

    QCOMPARE(author.primaryKey, QStringList({ "id" }));
    QVERIFY(!author.column("nope"));
}

void SchemaTests::compositePrimaryKey()
{
    QiSchema schema(m_db);
    const QiTableInfo bt = schema.table("book_tag");
    QCOMPARE(bt.primaryKey, QStringList({ "tag", "book_id" }));
    QCOMPARE(bt.column("tag")->primaryKeyOrder, 1);
    QCOMPARE(bt.column("book_id")->primaryKeyOrder, 2);
    // A composite key is never the rowid, even with an INTEGER column in it.
    QVERIFY(!bt.column("book_id")->autoIncrement);
}

void SchemaTests::foreignKeys()
{
    QiSchema schema(m_db);
    const QiTableInfo book = schema.table("book");
    QCOMPARE(book.foreignKeys.size(), 2);

    const QiForeignKeyInfo *toAuthor = nullptr, *toEditor = nullptr;
    for (const QiForeignKeyInfo &fk : book.foreignKeys) {
        if (fk.columns == QStringList({ "author_id" })) toAuthor = &fk;
        if (fk.columns == QStringList({ "editor_id" })) toEditor = &fk;
    }
    QVERIFY(toAuthor);
    QCOMPARE(toAuthor->refTable, QString("author"));
    QCOMPARE(toAuthor->refColumns, QStringList({ "id" }));
    QCOMPARE(toAuthor->onDelete, QString("CASCADE"));

    // `REFERENCES author` with no column means author's primary key.
    QVERIFY(toEditor);
    QCOMPARE(toEditor->refColumns, QStringList({ "id" }));
    QCOMPARE(toEditor->onUpdate, QString("SET NULL"));

    QCOMPARE(schema.table("author").foreignKeys.size(), 0);
}

void SchemaTests::indexes()
{
    QiSchema schema(m_db);
    const QiTableInfo book = schema.table("book");
    QCOMPARE(book.indexes.size(), 2);                  // sorted by name

    const QiIndexInfo &composite = book.indexes.at(0);
    QCOMPARE(composite.name, QString("idx_book_author_title"));
    QCOMPARE(composite.columns, QStringList({ "author_id", "title" }));
    QVERIFY(!composite.unique);
    QVERIFY(!composite.implicit);

    const QiIndexInfo &isbn = book.indexes.at(1);
    QCOMPARE(isbn.name, QString("idx_book_isbn"));
    QVERIFY(isbn.unique);
    QVERIFY(!isbn.implicit);

    // A non-rowid composite PRIMARY KEY is backed by an automatic unique index.
    const QiTableInfo bt = schema.table("book_tag");
    QCOMPARE(bt.indexes.size(), 1);
    QVERIFY(bt.indexes.at(0).implicit);
    QVERIFY(bt.indexes.at(0).unique);
    QCOMPARE(bt.indexes.at(0).columns, QStringList({ "tag", "book_id" }));
}

void SchemaTests::views()
{
    QiSchema schema(m_db);
    const QiTableInfo v = schema.table("v_titles");
    QVERIFY(v.isValid());
    QCOMPARE(v.kind, QiTableInfo::View);
    QCOMPARE(v.columns.size(), 2);
    QVERIFY(v.primaryKey.isEmpty());
    QVERIFY(v.foreignKeys.isEmpty());
    QVERIFY(v.indexes.isEmpty());
    QCOMPARE(schema.rowCount("v_titles"), qint64(3));
}

void SchemaTests::rowCount()
{
    QiSchema schema(m_db);
    QCOMPARE(schema.rowCount("author"), qint64(2));
    QCOMPARE(schema.rowCount("book"), qint64(3));
    QCOMPARE(schema.rowCount("book_tag"), qint64(0));
    QVERIFY(schema.lastError().isEmpty());

    QCOMPARE(schema.rowCount("no_such_table"), qint64(-1));
    QVERIFY(!schema.lastError().isEmpty());
}

void SchemaTests::missingTable()
{
    QiSchema schema(m_db);
    const QiTableInfo t = schema.table("no_such_table");
    QVERIFY(!t.isValid());
    QVERIFY(t.columns.isEmpty());
    QVERIFY(schema.lastError().contains("no_such_table"));
}

void SchemaTests::awkwardNames()
{
    QVERIFY(execAll(m_db, {
        "CREATE TABLE \"order items\" (\"select\" INTEGER PRIMARY KEY, \"qty \"\"x\"\"\" INTEGER)",
        "CREATE INDEX \"by qty\" ON \"order items\"(\"qty \"\"x\"\"\")",
        "INSERT INTO \"order items\" (\"qty \"\"x\"\"\") VALUES (4)",
    }));

    QiSchema schema(m_db);
    QVERIFY(schema.tableNames().contains("order items"));
    const QiTableInfo t = schema.table("order items");
    QVERIFY(t.isValid());
    QCOMPARE(t.primaryKey, QStringList({ "select" }));
    QVERIFY(t.column("qty \"x\""));
    QCOMPARE(t.indexes.size(), 1);
    QCOMPARE(t.indexes.at(0).columns, QStringList({ "qty \"x\"" }));
    QCOMPARE(schema.rowCount("order items"), qint64(1));

    QVERIFY(execAll(m_db, { "DROP TABLE \"order items\"" }));
}
