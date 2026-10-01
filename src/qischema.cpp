#include "qischema.h"

#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
#include <QSet>
#include <QPair>
#include <algorithm>

const QiColumnInfo *QiTableInfo::column(const QString &name) const
{
    for (const QiColumnInfo &c : columns)
        if (c.name.compare(name, Qt::CaseInsensitive) == 0)
            return &c;
    return nullptr;
}

QiSchema::QiSchema(const QSqlDatabase &db)
    : m_db(db)
{
    const QString driver = db.driverName();
    if (driver.startsWith(QLatin1String("QSQLITE")))
        m_reader = Sqlite;
    else if (driver == QLatin1String("QPSQL"))
        m_reader = Postgres;
    else if (driver == QLatin1String("QMYSQL") || driver == QLatin1String("QMARIADB"))
        m_reader = MySql;
    else if (driver == QLatin1String("QDUCKDB"))
        m_reader = DuckDb;
    else if (driver == QLatin1String("QODBC") && db.isOpen()) {
        // ODBC reaches any database; only read the catalog of one we know.
        QSqlQuery q(m_db);
        if (q.exec(QStringLiteral("SELECT @@VERSION")) && q.next()
            && q.value(0).toString().contains(QLatin1String("Microsoft SQL")))
            m_reader = SqlServer;
    }
}

QString QiSchema::dialect() const
{
    switch (m_reader) {
    case Sqlite:   return QStringLiteral("sqlite");
    case Postgres: return QStringLiteral("postgres");
    case MySql:     return QStringLiteral("mysql");
    case SqlServer: return QStringLiteral("sqlserver");
    case DuckDb:    return QStringLiteral("duckdb");
    case Generic:   break;
    }
    return QStringLiteral("generic");
}

bool QiSchema::hasForeignKeyInfo() const
{
    return m_reader != Generic;
}

QString QiSchema::defaultSchema() const
{
    if (m_defaultSchema.isNull() && m_reader != Sqlite && m_reader != Generic) {
        QSqlQuery q(m_db);
        const QString sql = m_reader == MySql     ? QStringLiteral("SELECT DATABASE()")
                          : m_reader == SqlServer ? QStringLiteral("SELECT SCHEMA_NAME()")
                          :                         QStringLiteral("SELECT current_schema()");
        if (q.exec(sql) && q.next())
            m_defaultSchema = q.value(0).toString();
    }
    return m_defaultSchema.isNull() ? QString() : m_defaultSchema;
}

QString QiSchema::lastError() const
{
    return m_error;
}

QString QiSchema::quoted(const QString &identifier) const
{
    if (m_db.driver())
        return m_db.driver()->escapeIdentifier(identifier, QSqlDriver::TableName);
    return QLatin1Char('"') + QString(identifier).replace(QLatin1Char('"'), QLatin1String("\"\""))
         + QLatin1Char('"');
}

QVector<QiSchema::Entry> QiSchema::list() const
{
    m_error.clear();
    switch (m_reader) {
    case Sqlite:   return listSqlite();
    case Postgres: return listPostgres();
    case MySql:     return listMySql();
    case SqlServer: return listSqlServer();
    case DuckDb:    return listDuckDb();
    case Generic:   break;
    }
    return listGeneric();
}

QiTableInfo QiSchema::describe(const Entry &e) const
{
    QiTableInfo t;
    switch (m_reader) {
    case Sqlite:   t = tableSqlite(e); break;
    case Postgres: t = tablePostgres(e); break;
    case MySql:     t = tableMySql(e); break;
    case SqlServer: t = tableSqlServer(e); break;
    case DuckDb:    t = tableDuckDb(e); break;
    case Generic:  t = tableGeneric(e); break;
    }
    if (t.isValid())
        t.schema = e.schema;
    return t;
}

const QiSchema::Entry *QiSchema::find(const QVector<Entry> &entries, const QString &name) const
{
    for (const Entry &e : entries)            // exact first: "Book" and "book" can coexist in Postgres
        if (e.name == name)
            return &e;
    for (const Entry &e : entries)
        if (e.name.compare(name, Qt::CaseInsensitive) == 0)
            return &e;
    return nullptr;
}

QString QiSchema::sqlName(const Entry &e) const
{
    return e.schema.isEmpty() ? quoted(e.table)
                              : quoted(e.schema) + QLatin1Char('.') + quoted(e.table);
}

QString QiSchema::sqlName(const QString &table) const
{
    const QVector<Entry> entries = list();
    const Entry *e = find(entries, table);
    return e ? sqlName(*e) : QString();
}

// Whether tableNames()/tables() should report an entry of this kind.
static bool qiSchemaWanted(QiTableInfo::Kind kind, bool includeViews, bool includeInternal)
{
    switch (kind) {
    case QiTableInfo::Table:    return true;
    case QiTableInfo::View:
    case QiTableInfo::Virtual:  return includeViews;
    case QiTableInfo::Internal: return includeInternal;
    }
    return false;
}

QStringList QiSchema::tableNames(bool includeViews, bool includeInternal) const
{
    QStringList names;
    for (const Entry &e : list())
        if (qiSchemaWanted(e.kind, includeViews, includeInternal))
            names << e.name;
    names.sort(Qt::CaseInsensitive);
    return names;
}

QiTableInfo QiSchema::table(const QString &name) const
{
    const QVector<Entry> entries = list();
    if (const Entry *e = find(entries, name))
        return describe(*e);
    m_error = QStringLiteral("no table or view named '%1'").arg(name);
    return QiTableInfo();
}

QVector<QiTableInfo> QiSchema::tables(bool includeViews, bool includeInternal) const
{
    QVector<Entry> entries = list();
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    QVector<QiTableInfo> out;
    for (const Entry &e : entries)
        if (qiSchemaWanted(e.kind, includeViews, includeInternal))
            out << describe(e);
    return out;
}

qint64 QiSchema::rowCount(const QString &table) const
{
    const QString from = sqlName(table);
    m_error.clear();
    QSqlQuery q(m_db);
    if (from.isEmpty() || !q.exec(QStringLiteral("SELECT COUNT(*) FROM ") + from) || !q.next()) {
        m_error = from.isEmpty() ? QStringLiteral("no table or view named '%1'").arg(table)
                                 : q.lastError().text();
        return -1;
    }
    return q.value(0).toLongLong();
}

// Sorts (position, name) pairs and returns just the names, in position order.
static QStringList qiSchemaOrdered(QVector<QPair<int, QString>> items)
{
    std::sort(items.begin(), items.end());
    QStringList out;
    for (const auto &p : items)
        out << p.second;
    return out;
}

// ---------------------------------------------------------------------------
//  SQLite: read the catalog directly
// ---------------------------------------------------------------------------

// PRAGMA arguments are identifiers, not bindable values, so they're quoted inline.
static QString qiSchemaPragma(const QString &pragma, const QString &arg)
{
    return QStringLiteral("PRAGMA %1(\"%2\")")
        .arg(pragma, QString(arg).replace(QLatin1Char('"'), QLatin1String("\"\"")));
}

QVector<QiSchema::Entry> QiSchema::listSqlite() const
{
    QVector<Entry> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT name, type, sql FROM sqlite_master "
            "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' "
            "ORDER BY name"))) {
        m_error = q.lastError().text();
        return out;
    }

    QStringList virtualTables;
    while (q.next()) {
        Entry e;
        e.name = e.table = q.value(0).toString();
        const QString type = q.value(1).toString();
        const QString sql  = q.value(2).toString();
        if (type == QLatin1String("view"))
            e.kind = QiTableInfo::View;
        else if (sql.startsWith(QLatin1String("CREATE VIRTUAL TABLE"), Qt::CaseInsensitive))
            e.kind = QiTableInfo::Virtual;
        else
            e.kind = QiTableInfo::Table;
        if (e.kind == QiTableInfo::Virtual)
            virtualTables << e.name;
        out << e;
    }

    // A virtual table keeps its data in ordinary "shadow" tables named
    // <vtable>_<suffix> (FTS5: _data, _idx, _content, _docsize, _config).
    for (Entry &e : out) {
        if (e.kind != QiTableInfo::Table)
            continue;
        for (const QString &vt : virtualTables) {
            if (e.name.startsWith(vt + QLatin1Char('_'), Qt::CaseInsensitive)) {
                e.kind = QiTableInfo::Internal;
                break;
            }
        }
    }
    return out;
}

QiTableInfo QiSchema::tableSqlite(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;
    QSqlQuery q(m_db);

    // Columns: cid | name | type | notnull | dflt_value | pk
    if (!q.exec(qiSchemaPragma(QStringLiteral("table_info"), e.table))) {
        m_error = q.lastError().text();
        return QiTableInfo();
    }
    QVector<QPair<int, QString>> pk;
    while (q.next()) {
        QiColumnInfo c;
        c.name         = q.value(1).toString();
        c.type         = q.value(2).toString();
        c.nullable     = q.value(3).toInt() == 0;
        c.defaultValue = q.value(4);
        c.primaryKeyOrder = q.value(5).toInt();
        c.primaryKey   = c.primaryKeyOrder > 0;
        if (c.primaryKey)
            pk << qMakePair(c.primaryKeyOrder, c.name);
        t.columns << c;
    }
    t.primaryKey = qiSchemaOrdered(pk);

    // A single INTEGER PRIMARY KEY column is the rowid: SQLite assigns it.
    if (t.primaryKey.size() == 1) {
        for (QiColumnInfo &c : t.columns)
            if (c.primaryKey && c.type.compare(QLatin1String("INTEGER"), Qt::CaseInsensitive) == 0)
                c.autoIncrement = true;
    }

    if (e.kind == QiTableInfo::View || e.kind == QiTableInfo::Virtual)
        return t;                              // no keys or indexes of their own

    // Foreign keys: id | seq | table | from | to | on_update | on_delete | match.
    // A composite key is several rows sharing an id.
    if (q.exec(qiSchemaPragma(QStringLiteral("foreign_key_list"), e.table))) {
        QMap<int, QiForeignKeyInfo> byId;
        while (q.next()) {
            QiForeignKeyInfo &fk = byId[q.value(0).toInt()];
            fk.refTable = q.value(2).toString();
            fk.columns    << q.value(3).toString();
            // `to` is empty when the reference names no column: it means the parent's primary key.
            fk.refColumns << q.value(4).toString();
            fk.onUpdate = q.value(5).toString();
            fk.onDelete = q.value(6).toString();
        }
        for (QiForeignKeyInfo fk : byId) {
            if (fk.refColumns.join(QString()).isEmpty()) {
                QSqlQuery pq(m_db);
                QVector<QPair<int, QString>> refPk;
                if (pq.exec(qiSchemaPragma(QStringLiteral("table_info"), fk.refTable)))
                    while (pq.next())
                        if (pq.value(5).toInt() > 0)
                            refPk << qMakePair(pq.value(5).toInt(), pq.value(1).toString());
                fk.refColumns = qiSchemaOrdered(refPk);
            }
            t.foreignKeys << fk;
        }
    }

    // Indexes: seq | name | unique | origin ('c' = CREATE INDEX, 'u' = UNIQUE, 'pk') | partial
    if (q.exec(qiSchemaPragma(QStringLiteral("index_list"), e.table))) {
        QVector<QiIndexInfo> found;
        while (q.next()) {
            QiIndexInfo ix;
            ix.name     = q.value(1).toString();
            ix.unique   = q.value(2).toInt() != 0;
            ix.implicit = q.value(3).toString() != QLatin1String("c");
            found << ix;
        }
        for (QiIndexInfo &ix : found) {
            QSqlQuery iq(m_db);
            // seqno | cid | name   (name is NULL for an expression column)
            if (iq.exec(qiSchemaPragma(QStringLiteral("index_info"), ix.name)))
                while (iq.next())
                    ix.columns << iq.value(2).toString();
        }
        std::sort(found.begin(), found.end(),
                  [](const QiIndexInfo &a, const QiIndexInfo &b) { return a.name < b.name; });
        t.indexes = found;
    }
    return t;
}

// ---------------------------------------------------------------------------
//  PostgreSQL: pg_catalog
// ---------------------------------------------------------------------------

// Column lists come back joined by the ASCII unit separator, which can't
// appear in an identifier, rather than as a Postgres array literal to parse.
static QStringList qiSchemaSplitList(const QVariant &joined)
{
    if (joined.isNull())
        return QStringList();
    return joined.toString().split(QChar(0x1F));
}

// pg_constraint.confdeltype / confupdtype codes.
static QString qiSchemaPgAction(const QString &code)
{
    const char c = code.isEmpty() ? '\0' : code.at(0).toLatin1();
    switch (c) {
    case 'c': return QStringLiteral("CASCADE");
    case 'n': return QStringLiteral("SET NULL");
    case 'd': return QStringLiteral("SET DEFAULT");
    case 'r': return QStringLiteral("RESTRICT");
    default:  return QStringLiteral("NO ACTION");
    }
}

QVector<QiSchema::Entry> QiSchema::listPostgres() const
{
    QVector<Entry> out;
    const QString home = defaultSchema();
    QSqlQuery q(m_db);
    // Ordinary and partitioned tables, views and materialized views, foreign
    // tables. Partitions are left out: their parent stands for them.
    if (!q.exec(QStringLiteral(
            "SELECT n.nspname, c.relname, c.relkind "
            "FROM pg_class c JOIN pg_namespace n ON n.oid = c.relnamespace "
            "WHERE c.relkind IN ('r','p','v','m','f') AND NOT c.relispartition "
            "  AND n.nspname <> 'information_schema' AND n.nspname NOT LIKE 'pg\\_%' "
            "ORDER BY n.nspname, c.relname"))) {
        m_error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Entry e;
        e.schema = q.value(0).toString();
        e.table  = q.value(1).toString();
        e.name   = e.schema == home ? e.table : e.schema + QLatin1Char('.') + e.table;
        const QString kind = q.value(2).toString();
        e.kind = kind == QLatin1String("v") || kind == QLatin1String("m") ? QiTableInfo::View
               : kind == QLatin1String("f") ? QiTableInfo::Virtual
               : QiTableInfo::Table;
        out << e;
    }
    return out;
}

QiTableInfo QiSchema::tablePostgres(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;
    const QString rel = sqlName(e);           // bound as text, cast to regclass server-side
    const QString home = defaultSchema();

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT a.attname, format_type(a.atttypid, a.atttypmod), NOT a.attnotnull, "
        "       pg_get_expr(d.adbin, d.adrelid), a.attidentity "
        "FROM pg_attribute a "
        "LEFT JOIN pg_attrdef d ON d.adrelid = a.attrelid AND d.adnum = a.attnum "
        "WHERE a.attrelid = CAST(? AS regclass) AND a.attnum > 0 AND NOT a.attisdropped "
        "ORDER BY a.attnum"));
    q.addBindValue(rel);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return QiTableInfo();
    }
    while (q.next()) {
        QiColumnInfo c;
        c.name     = q.value(0).toString();
        c.type     = q.value(1).toString();
        c.nullable = q.value(2).toBool();
        c.defaultValue = q.value(3);
        const QString identity = q.value(4).toString();
        // GENERATED {ALWAYS|BY DEFAULT} AS IDENTITY ('a'/'d'), or the older
        // SERIAL, which is a nextval() default.
        c.autoIncrement = identity == QLatin1String("a") || identity == QLatin1String("d")
                       || c.defaultValue.toString().startsWith(QLatin1String("nextval("));
        t.columns << c;
    }

    if (e.kind != QiTableInfo::Table)
        return t;

    // Primary key, in key order.
    q.prepare(QStringLiteral(
        "SELECT a.attname, k.ord "
        "FROM pg_index i "
        "CROSS JOIN LATERAL unnest(i.indkey) WITH ORDINALITY AS k(attnum, ord) "
        "JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = k.attnum "
        "WHERE i.indrelid = CAST(? AS regclass) AND i.indisprimary"));
    q.addBindValue(rel);
    if (q.exec()) {
        QVector<QPair<int, QString>> pk;
        while (q.next())
            pk << qMakePair(q.value(1).toInt(), q.value(0).toString());
        t.primaryKey = qiSchemaOrdered(pk);
        for (QiColumnInfo &c : t.columns) {
            c.primaryKeyOrder = t.primaryKey.indexOf(c.name) + 1;
            c.primaryKey = c.primaryKeyOrder > 0;
        }
    }

    // Foreign keys, with both column lists in constraint order.
    q.prepare(QStringLiteral(
        "SELECT con.conname, rn.nspname, rc.relname, con.confdeltype, con.confupdtype, "
        "  (SELECT string_agg(a.attname, chr(31) ORDER BY k.ord) "
        "     FROM unnest(con.conkey) WITH ORDINALITY AS k(n, ord) "
        "     JOIN pg_attribute a ON a.attrelid = con.conrelid AND a.attnum = k.n), "
        "  (SELECT string_agg(a.attname, chr(31) ORDER BY k.ord) "
        "     FROM unnest(con.confkey) WITH ORDINALITY AS k(n, ord) "
        "     JOIN pg_attribute a ON a.attrelid = con.confrelid AND a.attnum = k.n) "
        "FROM pg_constraint con "
        "JOIN pg_class rc ON rc.oid = con.confrelid "
        "JOIN pg_namespace rn ON rn.oid = rc.relnamespace "
        "WHERE con.conrelid = CAST(? AS regclass) AND con.contype = 'f' "
        "ORDER BY con.conname"));
    q.addBindValue(rel);
    if (q.exec()) {
        while (q.next()) {
            QiForeignKeyInfo fk;
            fk.name = q.value(0).toString();
            const QString refSchema = q.value(1).toString();
            fk.refTable = refSchema == home ? q.value(2).toString()
                                            : refSchema + QLatin1Char('.') + q.value(2).toString();
            fk.onDelete   = qiSchemaPgAction(q.value(3).toString());
            fk.onUpdate   = qiSchemaPgAction(q.value(4).toString());
            fk.columns    = qiSchemaSplitList(q.value(5));
            fk.refColumns = qiSchemaSplitList(q.value(6));
            t.foreignKeys << fk;
        }
    }

    // Indexes. Key columns only (not INCLUDE columns); an expression is "".
    // "implicit" = it backs a PRIMARY KEY, UNIQUE or EXCLUDE constraint.
    q.prepare(QStringLiteral(
        "SELECT ic.relname, i.indisunique, "
        "  EXISTS (SELECT 1 FROM pg_constraint c WHERE c.conindid = i.indexrelid "
        "          AND c.contype IN ('p','u','x')), "
        "  (SELECT string_agg(COALESCE(a.attname, ''), chr(31) ORDER BY k.ord) "
        "     FROM unnest(i.indkey) WITH ORDINALITY AS k(n, ord) "
        "     LEFT JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = k.n AND k.n <> 0 "
        "     WHERE k.ord <= i.indnkeyatts) "
        "FROM pg_index i JOIN pg_class ic ON ic.oid = i.indexrelid "
        "WHERE i.indrelid = CAST(? AS regclass) "
        "ORDER BY ic.relname"));
    q.addBindValue(rel);
    if (q.exec()) {
        while (q.next()) {
            QiIndexInfo ix;
            ix.name     = q.value(0).toString();
            ix.unique   = q.value(1).toBool();
            ix.implicit = q.value(2).toBool();
            ix.columns  = qiSchemaSplitList(q.value(3));
            t.indexes << ix;
        }
    }
    return t;
}

// ---------------------------------------------------------------------------
//  MySQL / MariaDB: information_schema, for the current database
// ---------------------------------------------------------------------------

QVector<QiSchema::Entry> QiSchema::listMySql() const
{
    QVector<Entry> out;
    const QString database = defaultSchema();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT TABLE_NAME, TABLE_TYPE FROM information_schema.TABLES "
        "WHERE TABLE_SCHEMA = ? ORDER BY TABLE_NAME"));
    q.addBindValue(database);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Entry e;
        e.name = e.table = q.value(0).toString();
        e.schema = database;          // one database = one schema; names stay plain
        e.kind = q.value(1).toString().contains(QLatin1String("VIEW")) ? QiTableInfo::View
                                                                       : QiTableInfo::Table;
        out << e;
    }
    return out;
}

QiTableInfo QiSchema::tableMySql(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;
    QSqlQuery q(m_db);

    // COLUMN_TYPE is the full declared type ("varchar(80)", "int unsigned"),
    // unlike DATA_TYPE. COLUMN_KEY 'PRI' marks primary key columns.
    q.prepare(QStringLiteral(
        "SELECT COLUMN_NAME, COLUMN_TYPE, IS_NULLABLE, COLUMN_DEFAULT, EXTRA "
        "FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? "
        "ORDER BY ORDINAL_POSITION"));
    q.addBindValue(e.schema);
    q.addBindValue(e.table);
    if (!q.exec()) {
        m_error = q.lastError().text();
        return QiTableInfo();
    }
    while (q.next()) {
        QiColumnInfo c;
        c.name     = q.value(0).toString();
        c.type     = q.value(1).toString();
        c.nullable = q.value(2).toString() == QLatin1String("YES");
        c.defaultValue = q.value(3);
        c.autoIncrement = q.value(4).toString().contains(QLatin1String("auto_increment"), Qt::CaseInsensitive);
        t.columns << c;
    }
    if (t.columns.isEmpty()) {
        m_error = QStringLiteral("no columns for '%1'").arg(e.name);
        return QiTableInfo();
    }
    if (e.kind != QiTableInfo::Table)
        return t;

    // Primary key, in key order.
    q.prepare(QStringLiteral(
        "SELECT COLUMN_NAME, ORDINAL_POSITION FROM information_schema.KEY_COLUMN_USAGE "
        "WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? AND CONSTRAINT_NAME = 'PRIMARY'"));
    q.addBindValue(e.schema);
    q.addBindValue(e.table);
    if (q.exec()) {
        QVector<QPair<int, QString>> pk;
        while (q.next())
            pk << qMakePair(q.value(1).toInt(), q.value(0).toString());
        t.primaryKey = qiSchemaOrdered(pk);
        for (QiColumnInfo &c : t.columns) {
            c.primaryKeyOrder = t.primaryKey.indexOf(c.name) + 1;
            c.primaryKey = c.primaryKeyOrder > 0;
        }
    }

    // Foreign keys: one row per column, grouped by constraint.
    q.prepare(QStringLiteral(
        "SELECT k.CONSTRAINT_NAME, k.COLUMN_NAME, k.REFERENCED_TABLE_SCHEMA, k.REFERENCED_TABLE_NAME, "
        "       k.REFERENCED_COLUMN_NAME, r.DELETE_RULE, r.UPDATE_RULE "
        "FROM information_schema.KEY_COLUMN_USAGE k "
        "JOIN information_schema.REFERENTIAL_CONSTRAINTS r "
        "  ON r.CONSTRAINT_SCHEMA = k.CONSTRAINT_SCHEMA AND r.CONSTRAINT_NAME = k.CONSTRAINT_NAME "
        " AND r.TABLE_NAME = k.TABLE_NAME "
        "WHERE k.TABLE_SCHEMA = ? AND k.TABLE_NAME = ? AND k.REFERENCED_TABLE_NAME IS NOT NULL "
        "ORDER BY k.CONSTRAINT_NAME, k.ORDINAL_POSITION"));
    q.addBindValue(e.schema);
    q.addBindValue(e.table);
    QSet<QString> fkNames;
    if (q.exec()) {
        while (q.next()) {
            const QString name = q.value(0).toString();
            if (t.foreignKeys.isEmpty() || t.foreignKeys.last().name != name) {
                QiForeignKeyInfo fk;
                fk.name = name;
                const QString refSchema = q.value(2).toString();
                fk.refTable = refSchema == e.schema ? q.value(3).toString()
                                                    : refSchema + QLatin1Char('.') + q.value(3).toString();
                fk.onDelete = q.value(5).toString();
                fk.onUpdate = q.value(6).toString();
                t.foreignKeys << fk;
                fkNames << name;
            }
            t.foreignKeys.last().columns    << q.value(1).toString();
            t.foreignKeys.last().refColumns << q.value(4).toString();
        }
    }

    // Indexes. "implicit" here means the PRIMARY index and the ones InnoDB
    // creates for foreign keys. MySQL stores a UNIQUE constraint and a unique
    // index as the same object — information_schema lists either as both — so,
    // unlike the other databases, a UNIQUE constraint's index can't be told
    // apart from CREATE UNIQUE INDEX and is reported as explicit.
    QSet<QString> constraintIndexes = fkNames;
    constraintIndexes << QStringLiteral("PRIMARY");

    q.prepare(QStringLiteral(
        "SELECT INDEX_NAME, NON_UNIQUE, COLUMN_NAME FROM information_schema.STATISTICS "
        "WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? ORDER BY INDEX_NAME, SEQ_IN_INDEX"));
    q.addBindValue(e.schema);
    q.addBindValue(e.table);
    if (q.exec()) {
        while (q.next()) {
            const QString name = q.value(0).toString();
            if (t.indexes.isEmpty() || t.indexes.last().name != name) {
                QiIndexInfo ix;
                ix.name = name;
                ix.unique = q.value(1).toInt() == 0;
                ix.implicit = constraintIndexes.contains(name);
                t.indexes << ix;
            }
            t.indexes.last().columns << q.value(2).toString();   // NULL (functional part) -> ""
        }
    }
    return t;
}

// ---------------------------------------------------------------------------
//  SQL Server: sys.* catalog views
// ---------------------------------------------------------------------------

// The object_id of one table or view, from its schema and name (bound in that order).
static const char *const kQiSchemaMsObject =
    "(SELECT o.object_id FROM sys.objects o JOIN sys.schemas s ON s.schema_id = o.schema_id "
    " WHERE s.name = ? AND o.name = ?)";

// sys.columns stores lengths in bytes, -1 for (max); rebuild the declared type.
static QString qiSchemaMsType(const QString &type, int maxLength, int precision, int scale)
{
    const QString t = type.toLower();
    if (t == QLatin1String("varchar") || t == QLatin1String("char") || t == QLatin1String("varbinary")
        || t == QLatin1String("binary"))
        return maxLength < 0 ? t + QLatin1String("(max)") : t + QStringLiteral("(%1)").arg(maxLength);
    if (t == QLatin1String("nvarchar") || t == QLatin1String("nchar"))      // UTF-16: 2 bytes a character
        return maxLength < 0 ? t + QLatin1String("(max)") : t + QStringLiteral("(%1)").arg(maxLength / 2);
    if (t == QLatin1String("decimal") || t == QLatin1String("numeric"))
        return t + QStringLiteral("(%1,%2)").arg(precision).arg(scale);
    if (t == QLatin1String("datetime2") || t == QLatin1String("time") || t == QLatin1String("datetimeoffset"))
        return t + QStringLiteral("(%1)").arg(scale);
    return t;
}

QVector<QiSchema::Entry> QiSchema::listSqlServer() const
{
    QVector<Entry> out;
    const QString home = defaultSchema();
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT s.name, o.name, o.type FROM sys.objects o "
            "JOIN sys.schemas s ON s.schema_id = o.schema_id "
            "WHERE o.type IN ('U','V') AND o.is_ms_shipped = 0 ORDER BY s.name, o.name"))) {
        m_error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Entry e;
        e.schema = q.value(0).toString();
        e.table  = q.value(1).toString();
        e.name   = e.schema == home ? e.table : e.schema + QLatin1Char('.') + e.table;
        e.kind   = q.value(2).toString().trimmed() == QLatin1String("V") ? QiTableInfo::View
                                                                         : QiTableInfo::Table;
        out << e;
    }
    return out;
}

QiTableInfo QiSchema::tableSqlServer(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;
    const QString home = defaultSchema();
    const QString object = QLatin1String(kQiSchemaMsObject);
    QSqlQuery q(m_db);
    auto bindObject = [&] { q.addBindValue(e.schema); q.addBindValue(e.table); };

    q.prepare(QStringLiteral(
        "SELECT c.name, TYPE_NAME(c.user_type_id), c.max_length, c.precision, c.scale, "
        "       c.is_nullable, OBJECT_DEFINITION(c.default_object_id), c.is_identity "
        "FROM sys.columns c WHERE c.object_id = %1 ORDER BY c.column_id").arg(object));
    bindObject();
    if (!q.exec()) {
        m_error = q.lastError().text();
        return QiTableInfo();
    }
    while (q.next()) {
        QiColumnInfo c;
        c.name     = q.value(0).toString();
        c.type     = qiSchemaMsType(q.value(1).toString(), q.value(2).toInt(),
                                    q.value(3).toInt(), q.value(4).toInt());
        c.nullable = q.value(5).toBool();
        c.defaultValue  = q.value(6);
        c.autoIncrement = q.value(7).toBool();
        t.columns << c;
    }
    if (e.kind != QiTableInfo::Table)
        return t;

    q.prepare(QStringLiteral(
        "SELECT c.name, ic.key_ordinal FROM sys.indexes i "
        "JOIN sys.index_columns ic ON ic.object_id = i.object_id AND ic.index_id = i.index_id "
        "JOIN sys.columns c ON c.object_id = ic.object_id AND c.column_id = ic.column_id "
        "WHERE i.object_id = %1 AND i.is_primary_key = 1").arg(object));
    bindObject();
    if (q.exec()) {
        QVector<QPair<int, QString>> pk;
        while (q.next())
            pk << qMakePair(q.value(1).toInt(), q.value(0).toString());
        t.primaryKey = qiSchemaOrdered(pk);
        for (QiColumnInfo &c : t.columns) {
            c.primaryKeyOrder = t.primaryKey.indexOf(c.name) + 1;
            c.primaryKey = c.primaryKeyOrder > 0;
        }
    }

    q.prepare(QStringLiteral(
        "SELECT fk.name, rs.name, rt.name, fk.delete_referential_action_desc, "
        "       fk.update_referential_action_desc, pc.name, rc.name "
        "FROM sys.foreign_keys fk "
        "JOIN sys.foreign_key_columns fkc ON fkc.constraint_object_id = fk.object_id "
        "JOIN sys.columns pc ON pc.object_id = fkc.parent_object_id AND pc.column_id = fkc.parent_column_id "
        "JOIN sys.columns rc ON rc.object_id = fkc.referenced_object_id AND rc.column_id = fkc.referenced_column_id "
        "JOIN sys.objects rt ON rt.object_id = fk.referenced_object_id "
        "JOIN sys.schemas rs ON rs.schema_id = rt.schema_id "
        "WHERE fk.parent_object_id = %1 ORDER BY fk.name, fkc.constraint_column_id").arg(object));
    bindObject();
    if (q.exec()) {
        while (q.next()) {
            const QString name = q.value(0).toString();
            if (t.foreignKeys.isEmpty() || t.foreignKeys.last().name != name) {
                QiForeignKeyInfo fk;
                fk.name = name;
                const QString refSchema = q.value(1).toString();
                fk.refTable = refSchema == home ? q.value(2).toString()
                                                : refSchema + QLatin1Char('.') + q.value(2).toString();
                // NO_ACTION, CASCADE, SET_NULL, SET_DEFAULT
                fk.onDelete = q.value(3).toString().replace(QLatin1Char('_'), QLatin1Char(' '));
                fk.onUpdate = q.value(4).toString().replace(QLatin1Char('_'), QLatin1Char(' '));
                t.foreignKeys << fk;
            }
            t.foreignKeys.last().columns    << q.value(5).toString();
            t.foreignKeys.last().refColumns << q.value(6).toString();
        }
    }

    // Key columns only (not INCLUDE columns); type 0 is the heap, not an index.
    q.prepare(QStringLiteral(
        "SELECT i.name, i.is_unique, CAST(i.is_primary_key | i.is_unique_constraint AS int), c.name "
        "FROM sys.indexes i "
        "JOIN sys.index_columns ic ON ic.object_id = i.object_id AND ic.index_id = i.index_id "
        "JOIN sys.columns c ON c.object_id = ic.object_id AND c.column_id = ic.column_id "
        "WHERE i.object_id = %1 AND i.type > 0 AND ic.is_included_column = 0 "
        "ORDER BY i.name, ic.key_ordinal").arg(object));
    bindObject();
    if (q.exec()) {
        while (q.next()) {
            const QString name = q.value(0).toString();
            if (t.indexes.isEmpty() || t.indexes.last().name != name) {
                QiIndexInfo ix;
                ix.name     = name;
                ix.unique   = q.value(1).toBool();
                ix.implicit = q.value(2).toInt() != 0;
                t.indexes << ix;
            }
            t.indexes.last().columns << q.value(3).toString();
        }
    }
    return t;
}

// ---------------------------------------------------------------------------
//  DuckDB: duckdb_*() catalog functions
// ---------------------------------------------------------------------------

// duckdb_indexes().expressions is a list rendered as text: ['"isbn"'] or [isbn].
static QStringList qiSchemaDuckExpressions(const QString &rendered)
{
    QString s = rendered.trimmed();
    if (s.startsWith(QLatin1Char('[')) && s.endsWith(QLatin1Char(']')))
        s = s.mid(1, s.size() - 2);
    QStringList out;
    for (QString part : s.split(QStringLiteral(", "), Qt::SkipEmptyParts)) {
        part = part.trimmed();
        if (part.size() >= 2 && part.startsWith(QLatin1Char('\'')) && part.endsWith(QLatin1Char('\'')))
            part = part.mid(1, part.size() - 2);
        if (part.size() >= 2 && part.startsWith(QLatin1Char('"')) && part.endsWith(QLatin1Char('"')))
            part = part.mid(1, part.size() - 2).replace(QLatin1String("\"\""), QLatin1String("\""));
        out << part;
    }
    return out;
}

QVector<QiSchema::Entry> QiSchema::listDuckDb() const
{
    QVector<Entry> out;
    const QString home = defaultSchema();
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT schema_name, table_name, 'T' FROM duckdb_tables() "
            "  WHERE NOT internal AND database_name = current_database() "
            "UNION ALL "
            "SELECT schema_name, view_name, 'V' FROM duckdb_views() "
            "  WHERE NOT internal AND database_name = current_database() "
            "ORDER BY 1, 2"))) {
        m_error = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Entry e;
        e.schema = q.value(0).toString();
        e.table  = q.value(1).toString();
        e.name   = e.schema == home ? e.table : e.schema + QLatin1Char('.') + e.table;
        e.kind   = q.value(2).toString() == QLatin1String("V") ? QiTableInfo::View : QiTableInfo::Table;
        out << e;
    }
    return out;
}

QiTableInfo QiSchema::tableDuckDb(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;
    const QString scope = QStringLiteral(
        " database_name = current_database() AND schema_name = ? AND table_name = ? ");
    QSqlQuery q(m_db);
    auto bindObject = [&] { q.addBindValue(e.schema); q.addBindValue(e.table); };

    q.prepare(QStringLiteral(
        "SELECT column_name, data_type, is_nullable, column_default FROM duckdb_columns() WHERE")
        + scope + QStringLiteral("ORDER BY column_index"));
    bindObject();
    if (!q.exec()) {
        m_error = q.lastError().text();
        return QiTableInfo();
    }
    while (q.next()) {
        QiColumnInfo c;
        c.name     = q.value(0).toString();
        c.type     = q.value(1).toString();
        c.nullable = q.value(2).toBool();
        c.defaultValue  = q.value(3);
        c.autoIncrement = c.defaultValue.toString().startsWith(QLatin1String("nextval("));
        t.columns << c;
    }
    if (t.columns.isEmpty()) {
        m_error = QStringLiteral("no columns for '%1'").arg(e.name);
        return QiTableInfo();
    }
    if (e.kind != QiTableInfo::Table)
        return t;

    // Constraints, with column lists flattened by the unit separator. DuckDB
    // keeps the indexes behind PRIMARY KEY / UNIQUE out of duckdb_indexes(), so
    // they are reported here as implicit unique indexes, like the other databases.
    q.prepare(QStringLiteral(
        "SELECT constraint_type, constraint_name, "
        "       array_to_string(constraint_column_names, chr(31)), "
        "       referenced_table, array_to_string(referenced_column_names, chr(31)) "
        "FROM duckdb_constraints() WHERE") + scope
        + QStringLiteral("AND constraint_type IN ('PRIMARY KEY','UNIQUE','FOREIGN KEY') "
                         "ORDER BY constraint_index"));
    bindObject();
    if (q.exec()) {
        while (q.next()) {
            const QString type = q.value(0).toString();
            const QStringList cols = qiSchemaSplitList(q.value(2));
            if (type == QLatin1String("FOREIGN KEY")) {
                QiForeignKeyInfo fk;
                fk.name       = q.value(1).toString();
                fk.columns    = cols;
                fk.refTable   = q.value(3).toString();      // DuckDB references within one schema
                fk.refColumns = qiSchemaSplitList(q.value(4));
                fk.onDelete = fk.onUpdate = QStringLiteral("NO ACTION");   // the only action DuckDB has
                t.foreignKeys << fk;
                continue;
            }
            if (type == QLatin1String("PRIMARY KEY"))
                t.primaryKey = cols;
            QiIndexInfo ix;
            ix.name     = q.value(1).toString();
            ix.columns  = cols;
            ix.unique   = true;
            ix.implicit = true;
            t.indexes << ix;
        }
        for (QiColumnInfo &c : t.columns) {
            c.primaryKeyOrder = t.primaryKey.indexOf(c.name) + 1;
            c.primaryKey = c.primaryKeyOrder > 0;
        }
    }

    // CREATE INDEX indexes.
    q.prepare(QStringLiteral(
        "SELECT index_name, is_unique, CAST(expressions AS VARCHAR) FROM duckdb_indexes() WHERE")
        + scope + QStringLiteral("ORDER BY index_name"));
    bindObject();
    if (q.exec()) {
        while (q.next()) {
            QiIndexInfo ix;
            ix.name    = q.value(0).toString();
            ix.unique  = q.value(1).toBool();
            ix.columns = qiSchemaDuckExpressions(q.value(2).toString());
            t.indexes << ix;
        }
    }
    std::sort(t.indexes.begin(), t.indexes.end(),
              [](const QiIndexInfo &a, const QiIndexInfo &b) { return a.name < b.name; });
    return t;
}

// ---------------------------------------------------------------------------
//  Any other driver: Qt's generic driver API
// ---------------------------------------------------------------------------

QVector<QiSchema::Entry> QiSchema::listGeneric() const
{
    QVector<Entry> out;
    if (!m_db.isOpen()) {
        m_error = QStringLiteral("database is not open");
        return out;
    }
    for (const QString &n : m_db.tables(QSql::Tables))
        out << Entry{ n, QString(), n, QiTableInfo::Table };
    for (const QString &n : m_db.tables(QSql::Views))
        out << Entry{ n, QString(), n, QiTableInfo::View };
    return out;
}

QiTableInfo QiSchema::tableGeneric(const Entry &e) const
{
    QiTableInfo t;
    t.name = e.name;
    t.kind = e.kind;

    const QSqlRecord rec = m_db.record(e.table);
    const QSqlIndex  pk  = m_db.primaryIndex(e.table);
    for (int i = 0; i < pk.count(); ++i)
        t.primaryKey << pk.fieldName(i);

    for (int i = 0; i < rec.count(); ++i) {
        const QSqlField f = rec.field(i);
        QiColumnInfo c;
        c.name     = f.name();
        // The driver's native type name isn't exposed generically; report the Qt type.
        c.type     = QString::fromLatin1(f.value().typeName());
        c.nullable = f.requiredStatus() != QSqlField::Required;
        c.autoIncrement = f.isAutoValue();
        if (!f.defaultValue().isNull())
            c.defaultValue = f.defaultValue();
        c.primaryKeyOrder = t.primaryKey.indexOf(c.name) + 1;
        c.primaryKey = c.primaryKeyOrder > 0;
        t.columns << c;
    }
    return t;
}
