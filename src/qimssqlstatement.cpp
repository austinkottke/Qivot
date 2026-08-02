#include <QStringList>
#include <QDebug>

#include "qimssqlstatement.h"

QiMsSqlStatement::QiMsSqlStatement()
{
}

QString QiMsSqlStatement::driverName(){
    return QStringLiteral("MSSQL");
}

QString QiMsSqlStatement::columnTypeName(int type){
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:          return QStringLiteral("INT");
    case QMetaType::LongLong:
    case QMetaType::ULongLong:     return QStringLiteral("BIGINT");
    case QMetaType::Float:         return QStringLiteral("REAL");
    case QMetaType::Double:        return QStringLiteral("FLOAT");
    // NVARCHAR (not VARCHAR) so non-ASCII text round-trips correctly.
    case QMetaType::QString:       return QStringLiteral("NVARCHAR(MAX)");
    // JSON stored as text (serialized string) for a portable, parameter-safe round-trip
    // — matching every other dialect. Qivot never queries into JSON, so a native JSON
    // type (SQL Server has none pre-2025 anyway) buys nothing.
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:  return QStringLiteral("NVARCHAR(MAX)");
    case QMetaType::QDateTime:     return QStringLiteral("DATETIME2");
    case QMetaType::QDate:         return QStringLiteral("DATE");
    case QMetaType::QTime:         return QStringLiteral("TIME");
    case QMetaType::QByteArray:    return QStringLiteral("VARBINARY(MAX)");
    // No native boolean; BIT is SQL Server's 0/1 convention.
    case QMetaType::Bool:          return QStringLiteral("BIT");
    default: break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (QMetaType(type).flags() & QMetaType::IsEnumeration) return QStringLiteral("INT");
#else
    if (QMetaType::typeFlags(static_cast<QMetaType::Type>(type)) & QMetaType::IsEnumeration) return QStringLiteral("INT");
#endif
    return QString();
}

QString QiMsSqlStatement::columnTypeForField(int type, QiClause clause){
    // A plain NVARCHAR(MAX) can't be indexed/keyed (SQL Server's index key limit is
    // ~900 bytes), so a string that is a key (PRIMARY KEY / UNIQUE) gets a bounded
    // NVARCHAR(450) instead — any other string stays NVARCHAR(MAX) so it never truncates.
    if (type == QMetaType::QString) {
        if (clause.testFlag(QiClause::PRIMARY_KEY) || clause.testFlag(QiClause::UNIQUE))
            return QStringLiteral("NVARCHAR(450)");
        return QStringLiteral("NVARCHAR(MAX)");
    }
    return columnTypeName(type);
}

QString QiMsSqlStatement::primaryKeyClause(const QString &typeName){
    // IDENTITY is only legal on an integer key.
    if (typeName.startsWith(QLatin1String("INT")) || typeName == QLatin1String("BIGINT"))
        return QStringLiteral("IDENTITY(1,1) PRIMARY KEY");
    return QStringLiteral("PRIMARY KEY");
}

QString QiMsSqlStatement::createTableIfNotExists(QiModelMetaInfo *info){
    // T-SQL has no "CREATE TABLE IF NOT EXISTS" (that clause is MySQL/Postgres/SQLite
    // syntax) — but QiConnection::createTables() already checks exists() before ever
    // calling this, so the "IF NOT EXISTS" the base generator emits is always
    // redundant by the time it would matter; just drop it rather than reach for
    // T-SQL's more verbose "IF NOT EXISTS (SELECT * FROM sys.tables WHERE ...)" wrapper.
    QString ddl = _createTableIfNotExists(info);
    ddl.replace(QLatin1String("CREATE TABLE IF NOT EXISTS"), QLatin1String("CREATE TABLE"));
    return ddl;
}

QString QiMsSqlStatement::addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field){
    // T-SQL: "ALTER TABLE ... ADD col type" — no COLUMN keyword (unlike the ANSI
    // default every other dialect here inherits unmodified).
    QiClause clause = field->clause;   // copy: flag()/testFlag() are non-const
    QString typeName;
    if (clause.testFlag(QiClause::SQL_TYPE)) {
        typeName = clause.flag(QiClause::SQL_TYPE).toString();
    } else {
        typeName = columnTypeForField(field->type, clause);
        if (typeName.isNull())
            return QString();
    }

    QString sql = QString("ALTER TABLE %1 ADD %2 %3")
                    .arg(info->name(), field->name, typeName).trimmed();

    if (clause.testFlag(QiClause::DEFAULT))
        sql += QString(" DEFAULT %1").arg(clause.flag(QiClause::DEFAULT).toString());

    sql += ";";
    return sql;
}

QString QiMsSqlStatement::renameColumn(QiModelMetaInfo *info, const QString &from, const QString &to){
    // T-SQL has no RENAME COLUMN — sp_rename is the equivalent system procedure.
    return QString("EXEC sp_rename '%1.%2', '%3', 'COLUMN';")
            .arg(info->name(), from, to);
}

QString QiMsSqlStatement::upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns){
    // No ON CONFLICT / ON DUPLICATE KEY in T-SQL — MERGE is the equivalent, and
    // (unlike everything else this library emits) a syntax error without its
    // trailing ";" — see keepsStatementTerminator().
    QStringList sourceCols, onList, setList, insertCols, insertVals;
    foreach (QString f, fields)
        sourceCols << QString(":%1 AS %1").arg(f);

    // A conflict column only belongs in the ON clause if it's actually one of the
    // inserted fields — "source" is built solely from `fields`, so referencing
    // source.id when id was deliberately omitted (a fresh insert, letting IDENTITY
    // assign it) is an invalid-column error, not merely a match that never fires.
    // When none of the conflict columns have a value to compare, there's nothing
    // to conflict on either way, so fall back to a condition that never matches —
    // every row lands in WHEN NOT MATCHED, i.e. a plain insert.
    foreach (QString f, conflictColumns) {
        if (fields.contains(f))
            onList << QString("target.%1 = source.%1").arg(f);
    }
    if (onList.isEmpty())
        onList << QStringLiteral("1 = 0");

    foreach (QString f, fields) {
        if (f == QLatin1String("id") || conflictColumns.contains(f))
            continue;
        setList << QString("%1 = source.%1").arg(f);
    }

    foreach (QString f, fields) {
        insertCols << f;
        insertVals << QString("source.%1").arg(f);
    }

    QStringList sql;
    sql << QString("MERGE INTO %1 AS target").arg(info->name());
    sql << QString("USING (SELECT %1) AS source").arg(sourceCols.join(", "));
    sql << QString("ON (%1)").arg(onList.join(" AND "));
    // With nothing to update (every field is a conflict column), SQL Server allows
    // omitting WHEN MATCHED entirely rather than requiring a no-op SET like MySQL does.
    if (!setList.isEmpty())
        sql << QString("WHEN MATCHED THEN UPDATE SET %1").arg(setList.join(", "));
    sql << QString("WHEN NOT MATCHED THEN INSERT (%1) VALUES (%2);")
               .arg(insertCols.join(", "), insertVals.join(", "));

    return sql.join(" ");
}

QString QiMsSqlStatement::replaceInto(QiModelMetaInfo *info, QStringList fields){
    // T-SQL has no REPLACE INTO either — same translation Postgres uses: an
    // upsert whose conflict target is the primary key.
    QString pk = info->primaryKeyName();
    if (pk.isEmpty()) pk = QStringLiteral("id");
    return upsertInto(info, fields, QStringList() << pk);
}

QString QiMsSqlStatement::lastInsertIdQuery() const {
    // SCOPE_IDENTITY() is scoped to the current session and stored procedure/batch
    // (unlike @@IDENTITY, which would also report an identity inserted by a trigger).
    return QStringLiteral("SELECT SCOPE_IDENTITY()");
}

QString QiMsSqlStatement::select(QiSharedQuery query) {
    // T-SQL has no LIMIT/OFFSET — paging needs OFFSET...FETCH, which is only legal
    // with an ORDER BY. When the query didn't ask for one, synthesize a no-op
    // ordering so paging still works (the row order is then merely unspecified,
    // same as it effectively already was without an ORDER BY on any dialect).
    QiQueryRules rules;
    rules = query;
    QStringList sql;
    sql << selectCore(rules);

    const bool paging = rules.limit() > 0 || rules.offset() > 0;
    if (rules.orderBy().size() > 0)
        sql << orderBy(rules);
    else if (paging)
        sql << QStringLiteral("ORDER BY (SELECT NULL)");

    if (paging) {
        sql << QString("OFFSET %1 ROWS").arg(rules.offset() > 0 ? rules.offset() : 0);
        if (rules.limit() > 0)
            sql << QString("FETCH NEXT %1 ROWS ONLY").arg(rules.limit());
    }

    sql << ";";
    return sql.join(" ");
}

QStringList QiMsSqlStatement::createFtsIndex(const QiBaseFtsIndex &index){
    Q_UNUSED(index);
    qWarning() << "QiMsSqlStatement: full-text search (FTS) is not supported on SQL Server yet; skipping.";
    return QStringList();
}

QStringList QiMsSqlStatement::dropFtsIndex(QString name){
    Q_UNUSED(name);
    return QStringList();
}
