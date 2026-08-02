#include <QStringList>
#include <QDebug>

#include "qioraclestatement.h"

// A placeholder substituted with the real "<table>_id_seq" name inside
// createTableIfNotExists() — the only hook here that knows the table name;
// primaryKeyClause() only sees the column's type. Chosen to be extremely
// unlikely to collide with anything a real DEFAULT/CHECK clause would contain.
static const QString kSeqToken = QStringLiteral("__QIVOT_ORACLE_SEQ__");

QiOracleStatement::QiOracleStatement()
{
}

QString QiOracleStatement::driverName(){
    return QStringLiteral("ORACLE");
}

QString QiOracleStatement::columnTypeName(int type){
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:          return QStringLiteral("NUMBER(10)");
    case QMetaType::LongLong:
    case QMetaType::ULongLong:     return QStringLiteral("NUMBER(19)");
    case QMetaType::Float:         return QStringLiteral("BINARY_FLOAT");
    case QMetaType::Double:        return QStringLiteral("BINARY_DOUBLE");
    case QMetaType::QString:       return QStringLiteral("VARCHAR2(255)");
    // JSON stored as text (serialized string), matching every other dialect — Qivot
    // never queries into JSON, so a native JSON type buys nothing. CLOB, not
    // VARCHAR2, since a serialized JSON value can easily exceed 4000 bytes.
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:  return QStringLiteral("CLOB");
    case QMetaType::QDateTime:     return QStringLiteral("TIMESTAMP");
    case QMetaType::QDate:         return QStringLiteral("DATE");
    // Oracle has no native TIME type; stored as a formatted "HH24:MI:SS" string.
    // Untested by the integration suite (which has no QTime field) — lowest-
    // confidence mapping in this file.
    case QMetaType::QTime:         return QStringLiteral("VARCHAR2(8)");
    case QMetaType::QByteArray:    return QStringLiteral("BLOB");
    // No native boolean pre-23c; NUMBER(1) is the conventional 0/1 encoding.
    case QMetaType::Bool:          return QStringLiteral("NUMBER(1)");
    default: break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (QMetaType(type).flags() & QMetaType::IsEnumeration) return QStringLiteral("NUMBER(10)");
#else
    if (QMetaType::typeFlags(static_cast<QMetaType::Type>(type)) & QMetaType::IsEnumeration) return QStringLiteral("NUMBER(10)");
#endif
    return QString();
}

QString QiOracleStatement::columnTypeForField(int type, QiClause clause){
    // A VARCHAR2 can be indexed/keyed; a CLOB can't. So a string that is a key
    // (PRIMARY KEY / UNIQUE) gets a bounded VARCHAR2(255); any other string is a
    // CLOB, so it never truncates.
    if (type == QMetaType::QString) {
        if (clause.testFlag(QiClause::PRIMARY_KEY) || clause.testFlag(QiClause::UNIQUE))
            return QStringLiteral("VARCHAR2(255)");
        return QStringLiteral("CLOB");
    }
    return columnTypeName(type);
}

QString QiOracleStatement::primaryKeyClause(const QString &typeName){
    // Auto-increment key: DEFAULT <table>_id_seq.NEXTVAL. kSeqToken stands in for the
    // real sequence name, filled in by createTableIfNotExists() below.
    if (typeName.startsWith(QLatin1String("NUMBER")))
        return QString("DEFAULT \"%1\".NEXTVAL PRIMARY KEY").arg(kSeqToken);
    return QStringLiteral("PRIMARY KEY");
}

QString QiOracleStatement::sequenceName(QiModelMetaInfo *info){
    return info->name() + QStringLiteral("_id_seq");
}

QString QiOracleStatement::createTableIfNotExists(QiModelMetaInfo *info){
    // Oracle has no CREATE TABLE IF NOT EXISTS, and QiConnection execs exactly one
    // statement per table (see QiSql::createTableIfNotExists) — but an auto-increment
    // table here needs two DDL statements: its companion sequence, then the table
    // itself (see primaryKeyClause() above). Both problems are solved the same way:
    // wrap everything in one PL/SQL anonymous block — a single statement as far as
    // QSqlQuery::exec() is concerned — with each half independently idempotent via an
    // exception trap on ORA-00955 ("name is already used by an existing object"), so
    // this is safe to re-run whether the sequence, the table, both, or neither exist.
    //
    // The generated CREATE TABLE text is embedded via q'[...]', PL/SQL's alternative
    // quoting operator, so embedded single quotes (e.g. inside a DEFAULT clause's
    // string literal) don't need doubling.
    const QString seq = sequenceName(info);

    QString ddl = _createTableIfNotExists(info);
    ddl.replace(QLatin1String("CREATE TABLE IF NOT EXISTS"), QLatin1String("CREATE TABLE"));
    ddl.replace(kSeqToken, seq);
    if (ddl.endsWith(QLatin1Char(';'))) ddl.chop(1);

    return QStringLiteral(
        "BEGIN\n"
        "  BEGIN EXECUTE IMMEDIATE 'CREATE SEQUENCE \"%1\" START WITH 1 INCREMENT BY 1 NOCACHE';\n"
        "  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;\n"
        "  BEGIN EXECUTE IMMEDIATE q'[%2]';\n"
        "  EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;\n"
        "END;"
    ).arg(seq, ddl);
}

QString QiOracleStatement::exists(QiModelMetaInfo *info){
    // No information_schema in Oracle; user_tables is the per-schema catalog view.
    // Unquoted identifiers fold to uppercase, so compare uppercased.
    return QString("SELECT table_name FROM user_tables WHERE table_name = UPPER('%1')")
            .arg(info->name());
}

QString QiOracleStatement::upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns){
    // No ON CONFLICT / ON DUPLICATE KEY in Oracle either — MERGE is the equivalent
    // (unlike SQL Server's, Oracle's MERGE doesn't require a trailing ";").
    QStringList sourceCols, onList, setList, insertCols, insertVals;
    foreach (QString f, fields)
        sourceCols << QString(":%1 AS %1").arg(f);

    foreach (QString f, conflictColumns)
        onList << QString("target.%1 = source.%1").arg(f);

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
    sql << QString("MERGE INTO %1 target").arg(info->name());
    sql << QString("USING (SELECT %1 FROM DUAL) source").arg(sourceCols.join(", "));
    sql << QString("ON (%1)").arg(onList.join(" AND "));
    // With nothing to update (every field is a conflict column), Oracle — like SQL
    // Server — allows omitting WHEN MATCHED entirely.
    if (!setList.isEmpty())
        sql << QString("WHEN MATCHED THEN UPDATE SET %1").arg(setList.join(", "));
    sql << QString("WHEN NOT MATCHED THEN INSERT (%1) VALUES (%2)")
               .arg(insertCols.join(", "), insertVals.join(", "));

    return sql.join(" ");
}

QString QiOracleStatement::replaceInto(QiModelMetaInfo *info, QStringList fields){
    // Oracle has no REPLACE INTO either — same translation Postgres/SQL Server use:
    // an upsert whose conflict target is the primary key.
    QString pk = info->primaryKeyName();
    if (pk.isEmpty()) pk = QStringLiteral("id");
    return upsertInto(info, fields, QStringList() << pk);
}

QString QiOracleStatement::lastInsertIdQuery(QiModelMetaInfo *info) const {
    // Unlike Postgres's lastval()/SQL Server's SCOPE_IDENTITY() (both session-scoped —
    // any insert in the session counts), Oracle's CURRVAL is per-sequence: it needs to
    // know which sequence, hence this table-aware overload rather than the base
    // no-argument one.
    return QString("SELECT \"%1\".CURRVAL FROM DUAL").arg(sequenceName(info));
}

QStringList QiOracleStatement::createFtsIndex(const QiBaseFtsIndex &index){
    Q_UNUSED(index);
    qWarning() << "QiOracleStatement: full-text search (FTS) is not supported on Oracle yet; skipping.";
    return QStringList();
}

QStringList QiOracleStatement::dropFtsIndex(QString name){
    Q_UNUSED(name);
    return QStringList();
}
