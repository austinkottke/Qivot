#ifndef SCHEMASUITE_H
#define SCHEMASUITE_H

/** QiSchema against a live database.

    Builds the same small bookstore schema in each backend's own SQL dialect,
    then checks QiSchema reports it the same way everywhere: auto-increment
    keys, a composite key in key order, foreign keys with their actions,
    unique / composite / constraint-backed indexes, a view, and — where the
    database has schemas — a table outside the default one.

    Where a database genuinely can't express something the checks say so
    (see Expect): DuckDB has no ON DELETE/UPDATE actions and drops VARCHAR
    lengths; MySQL has no schemas beyond the database itself.

    Every object is prefixed `qs_` so it can't collide with the round-trip suite.
 */
#include <functional>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <qischema.h>

namespace SchemaSuite {

using Check = std::function<void(bool, const QString &)>;

/// What each backend can express.
struct Expect {
    bool    actions = true;          ///< ON DELETE / ON UPDATE rules
    bool    typeLength = true;       ///< VARCHAR(80) keeps its 80
    QString defaultSchema;           ///< "" = no schemas to test
    bool    crossSchemaForeignKey = true;
};

static Expect expectFor(const QString &backend)
{
    Expect x;
    if (backend == "postgres")  { x.defaultSchema = "public"; }
    if (backend == "sqlserver") { x.defaultSchema = "dbo"; }
    if (backend == "duckdb")    { x.defaultSchema = "main"; x.actions = false; x.typeLength = false;
                                  x.crossSchemaForeignKey = false; }
    return x;
}

static QStringList ddl(const QString &backend)
{
    // Shared tail: the view and the rows. Every dialect accepts these as written.
    const QStringList common = {
        "CREATE VIEW qs_v_titles AS SELECT b.title, a.name FROM qs_book b JOIN qs_author a ON a.id = b.author_id",
        "INSERT INTO qs_author (name) VALUES ('Le Guin')",
        "INSERT INTO qs_author (name) VALUES ('Pratchett')",
        "INSERT INTO qs_book (author_id, title, isbn) VALUES (1, 'The Dispossessed', '978-0')",
        "INSERT INTO qs_book (author_id, title, isbn) VALUES (2, 'Small Gods', '978-1')",
        "INSERT INTO qs_book (author_id, title, isbn) VALUES (2, 'Mort', '978-2')",
    };

    if (backend == "sqlite") return QStringList{
        "PRAGMA foreign_keys = ON",
        "CREATE TABLE qs_author (id INTEGER PRIMARY KEY, name VARCHAR(80) NOT NULL,"
        "  born DATE DEFAULT '1900-01-01')",
        "CREATE TABLE qs_book (id INTEGER PRIMARY KEY,"
        "  author_id INTEGER NOT NULL REFERENCES qs_author(id) ON DELETE CASCADE,"
        "  editor_id INTEGER REFERENCES qs_author ON UPDATE SET NULL,"     // no column named
        "  title TEXT NOT NULL, isbn TEXT, price NUMERIC(8,2))",
        "CREATE UNIQUE INDEX qs_idx_book_isbn ON qs_book(isbn)",
        "CREATE INDEX qs_idx_book_author_title ON qs_book(author_id, title)",
        "CREATE TABLE qs_book_tag (book_id INTEGER NOT NULL REFERENCES qs_book(id),"
        "  tag TEXT NOT NULL, PRIMARY KEY (tag, book_id))",
    } + common;

    if (backend == "postgres") return QStringList{
        "DROP SCHEMA IF EXISTS qs_sales CASCADE",
        "DROP VIEW IF EXISTS qs_v_titles",
        "DROP TABLE IF EXISTS qs_book_tag, qs_book, qs_author CASCADE",
        "CREATE TABLE qs_author (id SERIAL PRIMARY KEY, name VARCHAR(80) NOT NULL,"
        "  born DATE DEFAULT '1900-01-01')",
        "CREATE TABLE qs_book (id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,"
        "  author_id INTEGER NOT NULL REFERENCES qs_author(id) ON DELETE CASCADE,"
        "  editor_id INTEGER REFERENCES qs_author ON UPDATE SET NULL,"
        "  title TEXT NOT NULL, isbn TEXT, price NUMERIC(8,2))",
        "CREATE UNIQUE INDEX qs_idx_book_isbn ON qs_book(isbn)",
        "CREATE INDEX qs_idx_book_author_title ON qs_book(author_id, title)",
        "CREATE TABLE qs_book_tag (book_id INTEGER NOT NULL REFERENCES qs_book(id),"
        "  tag TEXT NOT NULL, PRIMARY KEY (tag, book_id))",
        "CREATE SCHEMA qs_sales",
        "CREATE TABLE qs_sales.invoice (id SERIAL PRIMARY KEY, book_id INTEGER REFERENCES qs_book(id))",
    } + common;

    if (backend == "mysql") return QStringList{
        // InnoDB ignores REFERENCES written on a column; foreign keys must be table-level.
        "DROP VIEW IF EXISTS qs_v_titles",
        "DROP TABLE IF EXISTS qs_book_tag, qs_book, qs_author",
        "CREATE TABLE qs_author (id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(80) NOT NULL,"
        "  born DATE DEFAULT '1900-01-01') ENGINE=InnoDB",
        "CREATE TABLE qs_book (id INT AUTO_INCREMENT PRIMARY KEY, author_id INT NOT NULL,"
        "  editor_id INT, title VARCHAR(200) NOT NULL, isbn VARCHAR(20), price DECIMAL(8,2),"
        "  CONSTRAINT qs_fk_book_author FOREIGN KEY (author_id) REFERENCES qs_author(id) ON DELETE CASCADE,"
        "  CONSTRAINT qs_fk_book_editor FOREIGN KEY (editor_id) REFERENCES qs_author(id) ON UPDATE SET NULL"
        ") ENGINE=InnoDB",
        "CREATE UNIQUE INDEX qs_idx_book_isbn ON qs_book(isbn)",
        "CREATE INDEX qs_idx_book_author_title ON qs_book(author_id, title)",
        "CREATE TABLE qs_book_tag (book_id INT NOT NULL, tag VARCHAR(50) NOT NULL,"
        "  PRIMARY KEY (tag, book_id), FOREIGN KEY (book_id) REFERENCES qs_book(id)) ENGINE=InnoDB",
    } + common;

    if (backend == "sqlserver") return QStringList{
        "IF OBJECT_ID('qs_sales.invoice', 'U') IS NOT NULL DROP TABLE qs_sales.invoice",
        "IF SCHEMA_ID('qs_sales') IS NOT NULL DROP SCHEMA qs_sales",
        "DROP VIEW IF EXISTS qs_v_titles",
        "DROP TABLE IF EXISTS qs_book_tag",
        "DROP TABLE IF EXISTS qs_book",
        "DROP TABLE IF EXISTS qs_author",
        "CREATE TABLE qs_author (id INT IDENTITY(1,1) PRIMARY KEY, name VARCHAR(80) NOT NULL,"
        "  born DATE DEFAULT '1900-01-01')",
        "CREATE TABLE qs_book (id INT IDENTITY(1,1) PRIMARY KEY,"
        "  author_id INT NOT NULL REFERENCES qs_author(id) ON DELETE CASCADE,"
        "  editor_id INT REFERENCES qs_author ON UPDATE SET NULL,"          // no column named
        "  title NVARCHAR(200) NOT NULL, isbn VARCHAR(20), price DECIMAL(8,2))",
        "CREATE UNIQUE INDEX qs_idx_book_isbn ON qs_book(isbn)",
        "CREATE INDEX qs_idx_book_author_title ON qs_book(author_id, title)",
        "CREATE TABLE qs_book_tag (book_id INT NOT NULL REFERENCES qs_book(id),"
        "  tag VARCHAR(50) NOT NULL, PRIMARY KEY (tag, book_id))",
        "CREATE SCHEMA qs_sales",
        "CREATE TABLE qs_sales.invoice (id INT IDENTITY PRIMARY KEY, book_id INT REFERENCES dbo.qs_book(id))",
    } + common;

    if (backend == "duckdb") return QStringList{
        // No ON DELETE/UPDATE actions in DuckDB; auto-increment is a sequence default.
        "CREATE SEQUENCE qs_author_seq",
        "CREATE SEQUENCE qs_book_seq",
        "CREATE TABLE qs_author (id INTEGER PRIMARY KEY DEFAULT nextval('qs_author_seq'),"
        "  name VARCHAR(80) NOT NULL, born DATE DEFAULT '1900-01-01')",
        "CREATE TABLE qs_book (id INTEGER PRIMARY KEY DEFAULT nextval('qs_book_seq'),"
        "  author_id INTEGER NOT NULL REFERENCES qs_author(id),"
        "  editor_id INTEGER REFERENCES qs_author(id),"
        "  title VARCHAR NOT NULL, isbn VARCHAR, price DECIMAL(8,2))",
        "CREATE UNIQUE INDEX qs_idx_book_isbn ON qs_book(isbn)",
        "CREATE INDEX qs_idx_book_author_title ON qs_book(author_id, title)",
        "CREATE TABLE qs_book_tag (book_id INTEGER NOT NULL REFERENCES qs_book(id),"
        "  tag VARCHAR NOT NULL, PRIMARY KEY (tag, book_id))",
        "CREATE SCHEMA qs_sales",
        "CREATE TABLE qs_sales.invoice (id INTEGER PRIMARY KEY, book_id INTEGER)",
    } + common;

    return {};
}

static const QiForeignKeyInfo *fkOn(const QiTableInfo &t, const QString &column)
{
    for (const QiForeignKeyInfo &fk : t.foreignKeys)
        if (fk.columns == QStringList{ column })
            return &fk;
    return nullptr;
}

static const QiIndexInfo *indexNamed(const QiTableInfo &t, const QString &name)
{
    for (const QiIndexInfo &ix : t.indexes)
        if (ix.name == name)
            return &ix;
    return nullptr;
}

/// Returns false if this backend has no schema suite yet (nothing was checked).
static bool run(QSqlDatabase db, const QString &backend, const Check &check)
{
    const QStringList statements = ddl(backend);
    QiSchema schema(db);
    if (statements.isEmpty() || !schema.hasForeignKeyInfo()) {
        qInfo().noquote() << "  SKIP  schema suite: no catalog reader for" << backend << "yet";
        return false;
    }
    const Expect x = expectFor(backend);
    qInfo().noquote() << "\n--- QiSchema (" + schema.dialect() + ") ---";

    QSqlQuery q(db);
    for (const QString &sql : statements) {
        if (!q.exec(sql)) {
            check(false, "schema setup: " + sql.left(60) + " — " + q.lastError().text());
            return true;
        }
    }

    const QStringList names = schema.tableNames();
    check(names.contains("qs_author") && names.contains("qs_book") && names.contains("qs_book_tag"),
          "tableNames lists the tables");
    check(!names.contains("qs_v_titles") && schema.tableNames(true).contains("qs_v_titles"),
          "views only when asked for");

    // Columns
    const QiTableInfo author = schema.table("qs_author");
    check(author.isValid() && author.columns.size() == 3, "qs_author has 3 columns");
    const QiColumnInfo *aid = author.column("id");
    check(aid && aid->primaryKey && aid->autoIncrement, "author.id: auto-increment primary key");
    const QiColumnInfo *aname = author.column("name");
    check(aname && !aname->nullable && aname->type.contains("char", Qt::CaseInsensitive)
              && (!x.typeLength || aname->type.contains("80")),
          "author.name: NOT NULL, declared as a VARCHAR(80) — got \"" + (aname ? aname->type : QString()) + "\"");
    const QiColumnInfo *born = author.column("born");
    check(born && born->nullable && born->defaultValue.toString().contains("1900-01-01"),
          "author.born: nullable, default 1900-01-01 — got \"" + (born ? born->defaultValue.toString() : QString()) + "\"");

    const QiTableInfo book = schema.table("qs_book");
    check(book.column("id") && book.column("id")->autoIncrement, "book.id: auto-increment");
    check(book.primaryKey == QStringList{ "id" }, "book primary key is id");

    // Composite primary key, in key order (not column order)
    const QiTableInfo tag = schema.table("qs_book_tag");
    check(tag.primaryKey == (QStringList{ "tag", "book_id" }),
          "book_tag primary key in key order: " + tag.primaryKey.join(", "));
    check(tag.column("tag") && tag.column("tag")->primaryKeyOrder == 1, "book_tag.tag is key column 1");

    // Foreign keys
    check(book.foreignKeys.size() == 2, QString("book has 2 foreign keys (got %1)").arg(book.foreignKeys.size()));
    const QiForeignKeyInfo *toAuthor = fkOn(book, "author_id");
    check(toAuthor && toAuthor->refTable == "qs_author" && toAuthor->refColumns == QStringList{ "id" }
              && toAuthor->onDelete == (x.actions ? "CASCADE" : "NO ACTION"),
          QString("book.author_id -> qs_author(id) ON DELETE %1 — got %2")
              .arg(x.actions ? "CASCADE" : "NO ACTION", toAuthor ? toAuthor->onDelete : QString("none")));
    const QiForeignKeyInfo *toEditor = fkOn(book, "editor_id");
    check(toEditor && toEditor->refColumns == QStringList{ "id" }
              && toEditor->onUpdate == (x.actions ? "SET NULL" : "NO ACTION"),
          "book.editor_id -> qs_author's key, ON UPDATE " + QString(x.actions ? "SET NULL" : "NO ACTION"));

    // Indexes
    const QiIndexInfo *isbn = indexNamed(book, "qs_idx_book_isbn");
    check(isbn && isbn->unique && !isbn->implicit && isbn->columns == QStringList{ "isbn" },
          "unique index on isbn (explicit)");
    const QiIndexInfo *composite = indexNamed(book, "qs_idx_book_author_title");
    check(composite && !composite->unique && composite->columns == (QStringList{ "author_id", "title" }),
          "composite index in column order — got " + (composite ? composite->columns.join(", ") : QString("none")));
    bool pkIndex = false;
    for (const QiIndexInfo &ix : tag.indexes)
        pkIndex |= ix.implicit && ix.unique && ix.columns == (QStringList{ "tag", "book_id" });
    check(pkIndex, "book_tag's primary key is backed by an implicit unique index");

    // Views
    const QiTableInfo view = schema.table("qs_v_titles");
    check(view.kind == QiTableInfo::View && view.columns.size() == 2 && view.indexes.isEmpty(),
          "view: 2 columns, no indexes");

    // Row counts
    check(schema.rowCount("qs_book") == 3 && schema.rowCount("qs_v_titles") == 3, "rowCount on a table and a view");
    check(schema.rowCount("qs_no_such_table") == -1, "rowCount of a missing table is -1");

    // Schemas
    if (!x.defaultSchema.isEmpty()) {
        check(schema.defaultSchema() == x.defaultSchema,
              "default schema is " + x.defaultSchema + " — got " + schema.defaultSchema());
        check(schema.tableNames().contains("qs_sales.invoice"), "a table in another schema is listed as schema.table");
        // Literal, not schema.quoted(): a driver that forgets to quote must fail here.
        const QString expected = "\"qs_sales\".\"invoice\"";
        check(schema.sqlName("qs_sales.invoice") == expected,
              "sqlName quotes schema and table: " + schema.sqlName("qs_sales.invoice"));
        const QiTableInfo inv = schema.table("qs_sales.invoice");
        check(inv.schema == "qs_sales", "QiTableInfo::schema is set");
        if (x.crossSchemaForeignKey)
            check(fkOn(inv, "book_id") && fkOn(inv, "book_id")->refTable == "qs_book",
                  "cross-schema foreign key names the default-schema table plainly");
        check(schema.rowCount("qs_sales.invoice") == 0, "rowCount works on a qualified name");
    }
    return true;
}

} // namespace SchemaSuite

#endif // SCHEMASUITE_H
