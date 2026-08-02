#include <QStringList>
#include <QDebug>

#include "qiduckdbstatement.h"

// Stands in for the real "<table>_id_seq" name inside createTableIfNotExists() — the only
// hook that knows the table name; primaryKeyClause() only sees the column's type.
static const QString kDuckSeqToken = QStringLiteral("__QIVOT_DUCKDB_SEQ__");

QiDuckDbStatement::QiDuckDbStatement()
{
}

QString QiDuckDbStatement::driverName(){
    return QStringLiteral("DUCKDB");
}

QString QiDuckDbStatement::columnTypeName(int type){
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:          return QStringLiteral("INTEGER");
    case QMetaType::LongLong:
    case QMetaType::ULongLong:     return QStringLiteral("BIGINT");
    case QMetaType::Float:         return QStringLiteral("FLOAT");
    case QMetaType::Double:        return QStringLiteral("DOUBLE");
    // DuckDB's VARCHAR is unlimited *and* indexable, so — unlike MySQL/Oracle — plain and
    // keyed strings share one type and no columnTypeForField() override is needed. JSON is
    // stored as text (a serialized string), matching every other dialect.
    case QMetaType::QString:
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:  return QStringLiteral("VARCHAR");
    case QMetaType::QDateTime:     return QStringLiteral("TIMESTAMP");
    case QMetaType::QDate:         return QStringLiteral("DATE");
    case QMetaType::QTime:         return QStringLiteral("TIME");
    case QMetaType::QByteArray:    return QStringLiteral("BLOB");
    case QMetaType::Bool:          return QStringLiteral("BOOLEAN");
    default: break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (QMetaType(type).flags() & QMetaType::IsEnumeration) return QStringLiteral("INTEGER");
#else
    if (QMetaType::typeFlags(static_cast<QMetaType::Type>(type)) & QMetaType::IsEnumeration) return QStringLiteral("INTEGER");
#endif
    return QString();
}

QString QiDuckDbStatement::primaryKeyClause(const QString &typeName){
    // No IDENTITY/SERIAL keyword — an integer key DEFAULTs to the next value of the
    // table's companion sequence. kDuckSeqToken is filled in by createTableIfNotExists().
    if (typeName == QLatin1String("INTEGER") || typeName == QLatin1String("BIGINT"))
        return QString("DEFAULT nextval('%1') PRIMARY KEY").arg(kDuckSeqToken);
    return QStringLiteral("PRIMARY KEY");
}

QString QiDuckDbStatement::sequenceName(QiModelMetaInfo *info){
    return info->name() + QStringLiteral("_id_seq");
}

QString QiDuckDbStatement::createTableIfNotExists(QiModelMetaInfo *info){
    // An auto-increment table needs its companion sequence to exist first. DuckDB has
    // both CREATE SEQUENCE IF NOT EXISTS and CREATE TABLE IF NOT EXISTS and runs multiple
    // ";"-separated statements per query, so both go in one idempotent string.
    const QString seq = sequenceName(info);
    QString ddl = _createTableIfNotExists(info);
    ddl.replace(kDuckSeqToken, seq);
    return QString("CREATE SEQUENCE IF NOT EXISTS \"%1\";\n%2").arg(seq, ddl);
}

QString QiDuckDbStatement::replaceInto(QiModelMetaInfo *info, QStringList fields){
    // DuckDB has no REPLACE INTO — same translation Postgres uses: an upsert whose
    // conflict target is the primary key.
    QString pk = info->primaryKeyName();
    if (pk.isEmpty()) pk = QStringLiteral("id");
    return upsertInto(info, fields, QStringList() << pk);
}

QString QiDuckDbStatement::lastInsertIdQuery(QiModelMetaInfo *info) const {
    // currval() is connection-scoped once nextval has run in the session (like Oracle's
    // CURRVAL / Postgres's lastval()), so it survives the separate id query — but it
    // needs to know which sequence, hence this table-aware overload.
    return QString("SELECT currval('%1')").arg(sequenceName(info));
}

QStringList QiDuckDbStatement::createFtsIndex(const QiBaseFtsIndex &index){
    Q_UNUSED(index);
    qWarning() << "QiDuckDbStatement: full-text search (FTS) is not supported on DuckDB yet; skipping.";
    return QStringList();
}

QStringList QiDuckDbStatement::dropFtsIndex(QString name){
    Q_UNUSED(name);
    return QStringList();
}
