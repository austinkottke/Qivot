#include <QStringList>
#include <QDebug>

#include "qimysqlstatement.h"

QiMysqlStatement::QiMysqlStatement()
{
}

QString QiMysqlStatement::driverName(){
    return QStringLiteral("MYSQL");
}

QString QiMysqlStatement::columnTypeName(int type){
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:          return QStringLiteral("INT");
    case QMetaType::LongLong:
    case QMetaType::ULongLong:     return QStringLiteral("BIGINT");
    case QMetaType::Float:         return QStringLiteral("FLOAT");
    case QMetaType::Double:        return QStringLiteral("DOUBLE");
    case QMetaType::QString:       return QStringLiteral("VARCHAR(255)");
    // JSON stored as TEXT (serialized string) for a portable, parameter-safe round-trip
    // — matching Postgres/SQLite. Qivot never queries into JSON, so native JSON buys nothing.
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:  return QStringLiteral("TEXT");
    case QMetaType::QDateTime:     return QStringLiteral("DATETIME");
    case QMetaType::QDate:         return QStringLiteral("DATE");
    case QMetaType::QTime:         return QStringLiteral("TIME");
    case QMetaType::QByteArray:    return QStringLiteral("BLOB");
    case QMetaType::Bool:          return QStringLiteral("TINYINT(1)");
    default: break;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (QMetaType(type).flags() & QMetaType::IsEnumeration) return QStringLiteral("INT");
#else
    if (QMetaType::typeFlags(static_cast<QMetaType::Type>(type)) & QMetaType::IsEnumeration) return QStringLiteral("INT");
#endif
    return QString();
}

QString QiMysqlStatement::columnTypeForField(int type, QiClause clause){
    // MySQL can't index/key a TEXT column without a prefix length, but TEXT is the
    // only type that won't truncate long strings. So: a string that is a key
    // (PRIMARY KEY / UNIQUE) becomes a bounded VARCHAR(255); any other string is TEXT.
    if (type == QMetaType::QString) {
        if (clause.testFlag(QiClause::PRIMARY_KEY) || clause.testFlag(QiClause::UNIQUE))
            return QStringLiteral("VARCHAR(255)");
        return QStringLiteral("TEXT");
    }
    return columnTypeName(type);
}

QString QiMysqlStatement::primaryKeyClause(const QString &typeName){
    // AUTO_INCREMENT is only legal on an integer key.
    if (typeName.startsWith(QLatin1String("INT")) || typeName == QLatin1String("BIGINT"))
        return QStringLiteral("AUTO_INCREMENT PRIMARY KEY");
    return QStringLiteral("PRIMARY KEY");
}

QString QiMysqlStatement::tableSuffix(QiModelMetaInfo *info){
    Q_UNUSED(info);
    return QStringLiteral(" ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
}

QString QiMysqlStatement::upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns){
    QStringList values;
    foreach (QString f, fields)
        values << ":" + f;

    // Update every inserted column except the primary key and the conflict key(s).
    QStringList setList;
    foreach (QString f, fields) {
        if (f == QLatin1String("id") || conflictColumns.contains(f))
            continue;
        setList << QString("%1=VALUES(%1)").arg(f);
    }

    // MySQL keys the upsert on the table's own PRIMARY/UNIQUE indexes, so the
    // conflict columns are informational here. With nothing to update, a harmless
    // self-assignment leaves the existing row untouched (MySQL has no DO NOTHING).
    QString action = setList.isEmpty()
            ? QString("%1=%1").arg(conflictColumns.value(0, QStringLiteral("id")))
            : setList.join(",");

    return QString("INSERT INTO %1 (%2) VALUES (%3) ON DUPLICATE KEY UPDATE %4;")
            .arg(info->name(), fields.join(","), values.join(","), action);
}

QStringList QiMysqlStatement::createFtsIndex(const QiBaseFtsIndex &index){
    Q_UNUSED(index);
    qWarning() << "QiMysqlStatement: full-text search (FTS) is not supported on MySQL yet; skipping.";
    return QStringList();
}

QStringList QiMysqlStatement::dropFtsIndex(QString name){
    Q_UNUSED(name);
    return QStringList();
}
