#ifndef QiSCHEMA_H
#define QiSCHEMA_H

#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

/// One column of a table, as the database reports it.
struct QiColumnInfo {
    QString  name;
    QString  type;               ///< declared type as written in the schema ("VARCHAR(80)", "INTEGER")
    bool     nullable = true;
    bool     primaryKey = false;
    int      primaryKeyOrder = 0; ///< 1-based position in a composite key; 0 if not part of it
    bool     autoIncrement = false;
    QVariant defaultValue;       ///< the default *expression* as text, or null if none
};

/// A foreign key: `columns` of this table reference `refColumns` of `refTable`.
struct QiForeignKeyInfo {
    QString     name;            ///< constraint name (empty where the database doesn't name them)
    QStringList columns;
    QString     refTable;
    QStringList refColumns;
    QString     onDelete;        ///< "CASCADE", "SET NULL", "RESTRICT", "NO ACTION", ...
    QString     onUpdate;
};

/// An index on a table.
struct QiIndexInfo {
    QString     name;
    QStringList columns;         ///< in index order; an expression column is reported as ""
    bool        unique = false;
    bool        implicit = false; ///< created by a PRIMARY KEY or UNIQUE constraint, not by CREATE INDEX
};

/// Everything QiSchema knows about one table or view.
struct QiTableInfo {
    enum Kind {
        Table,                   ///< an ordinary table
        View,
        Virtual,                 ///< e.g. an SQLite FTS5 table
        Internal                 ///< storage behind a virtual table (FTS shadow tables); usually hidden
    };

    QString                   name;
    Kind                      kind = Table;
    QVector<QiColumnInfo>     columns;
    QStringList               primaryKey;   ///< column names, in key order
    QVector<QiForeignKeyInfo> foreignKeys;
    QVector<QiIndexInfo>      indexes;

    bool isValid() const { return !name.isEmpty(); }

    /// The column called `name`, or nullptr. Case-insensitive, as SQL identifiers usually are.
    const QiColumnInfo *column(const QString &name) const;
};

/// Read the structure of an existing database at runtime.
/**
  Qivot's models are declared in C++ and known at compile time. QiSchema is the
  other direction: point it at any open database and it reports the tables,
  views, columns, keys, foreign keys and indexes that are actually there — the
  basis for browsers, diagram tools, code generators and migration diffs.

  SQLite is read from its own catalog (the `PRAGMA table_info` family), so
  everything is reported. Other drivers currently go through Qt's generic driver
  API, which yields tables, views, columns and primary keys; their foreign keys
  and indexes come back empty until a dialect-specific reader is added
  (hasForeignKeyInfo() says which applies).

\code
    QiSchema schema(QSqlDatabase::database());
    for (const QString &name : schema.tableNames()) {
        QiTableInfo t = schema.table(name);
        qDebug() << t.name << t.columns.size() << "columns," << schema.rowCount(name) << "rows";
        for (const QiForeignKeyInfo &fk : t.foreignKeys)
            qDebug() << "  " << fk.columns << "->" << fk.refTable << fk.refColumns;
    }
\endcode
 */
class QiSchema {
public:
    explicit QiSchema(const QSqlDatabase &db = QSqlDatabase::database());

    /// Which reader is in use: "sqlite", or "generic" for any other driver.
    QString dialect() const;

    /// True if foreign keys and indexes are read for this database (not just columns and keys).
    bool hasForeignKeyInfo() const;

    /// Names of the user's tables, sorted. Views, virtual and internal tables are
    /// included only when asked for — a browser wants them, a code generator usually doesn't.
    QStringList tableNames(bool includeViews = false, bool includeInternal = false) const;

    /// Full description of one table or view; an invalid QiTableInfo if it doesn't exist.
    QiTableInfo table(const QString &name) const;

    /// table() for every name tableNames() returns.
    QVector<QiTableInfo> tables(bool includeViews = false, bool includeInternal = false) const;

    /// `SELECT COUNT(*)` on the table; -1 if it fails.
    qint64 rowCount(const QString &table) const;

    /// The driver's quoted form of an identifier, for building SQL safely.
    QString quoted(const QString &identifier) const;

    /// Why the last call failed (empty on success).
    QString lastError() const;

private:
    struct Entry { QString name; QiTableInfo::Kind kind; };
    QVector<Entry> listSqlite() const;
    QVector<Entry> listGeneric() const;
    QVector<Entry> list() const;
    QiTableInfo tableSqlite(const QString &name, QiTableInfo::Kind kind) const;
    QiTableInfo tableGeneric(const QString &name, QiTableInfo::Kind kind) const;

    QSqlDatabase    m_db;
    bool            m_sqlite = false;
    mutable QString m_error;
};

#endif // QiSCHEMA_H
