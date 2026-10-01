#include "qischema.h"

#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
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
    , m_sqlite(db.driverName().startsWith(QLatin1String("QSQLITE")))
{
}

QString QiSchema::dialect() const
{
    return m_sqlite ? QStringLiteral("sqlite") : QStringLiteral("generic");
}

bool QiSchema::hasForeignKeyInfo() const
{
    return m_sqlite;
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
    return m_sqlite ? listSqlite() : listGeneric();
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
    for (const Entry &e : list()) {
        if (e.name.compare(name, Qt::CaseInsensitive) == 0)
            return m_sqlite ? tableSqlite(e.name, e.kind) : tableGeneric(e.name, e.kind);
    }
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
            out << (m_sqlite ? tableSqlite(e.name, e.kind) : tableGeneric(e.name, e.kind));
    return out;
}

qint64 QiSchema::rowCount(const QString &table) const
{
    m_error.clear();
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoted(table))) || !q.next()) {
        m_error = q.lastError().text();
        return -1;
    }
    return q.value(0).toLongLong();
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
        e.name = q.value(0).toString();
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

QiTableInfo QiSchema::tableSqlite(const QString &name, QiTableInfo::Kind kind) const
{
    QiTableInfo t;
    t.name = name;
    t.kind = kind;
    QSqlQuery q(m_db);

    // Columns: cid | name | type | notnull | dflt_value | pk
    if (!q.exec(qiSchemaPragma(QStringLiteral("table_info"), name))) {
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
    std::sort(pk.begin(), pk.end());
    for (const auto &p : pk)
        t.primaryKey << p.second;

    // A single INTEGER PRIMARY KEY column is the rowid: SQLite assigns it.
    if (t.primaryKey.size() == 1) {
        for (QiColumnInfo &c : t.columns)
            if (c.primaryKey && c.type.compare(QLatin1String("INTEGER"), Qt::CaseInsensitive) == 0)
                c.autoIncrement = true;
    }

    if (kind == QiTableInfo::View || kind == QiTableInfo::Virtual)
        return t;                              // no keys or indexes of their own

    // Foreign keys: id | seq | table | from | to | on_update | on_delete | match.
    // A composite key is several rows sharing an id.
    if (q.exec(qiSchemaPragma(QStringLiteral("foreign_key_list"), name))) {
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
                fk.refColumns.clear();
                if (pq.exec(qiSchemaPragma(QStringLiteral("table_info"), fk.refTable))) {
                    QVector<QPair<int, QString>> refPk;
                    while (pq.next())
                        if (pq.value(5).toInt() > 0)
                            refPk << qMakePair(pq.value(5).toInt(), pq.value(1).toString());
                    std::sort(refPk.begin(), refPk.end());
                    for (const auto &p : refPk)
                        fk.refColumns << p.second;
                }
            }
            t.foreignKeys << fk;
        }
    }

    // Indexes: seq | name | unique | origin ('c' = CREATE INDEX, 'u' = UNIQUE, 'pk') | partial
    if (q.exec(qiSchemaPragma(QStringLiteral("index_list"), name))) {
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
        out << Entry{ n, QiTableInfo::Table };
    for (const QString &n : m_db.tables(QSql::Views))
        out << Entry{ n, QiTableInfo::View };
    return out;
}

QiTableInfo QiSchema::tableGeneric(const QString &name, QiTableInfo::Kind kind) const
{
    QiTableInfo t;
    t.name = name;
    t.kind = kind;

    const QSqlRecord rec = m_db.record(name);
    const QSqlIndex  pk  = m_db.primaryIndex(name);
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
