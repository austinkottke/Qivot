#include <QStringList>
#include <QtCore>
#include "qisqlitestatement.h"
#include <QSqlDriver>
#include <QSqlField>

QiSqliteStatement::QiSqliteStatement()
{
}

QString QiSqliteStatement::_createTableIfNotExists(QiModelMetaInfo *info) {
    QString statement = QString("%1 (\n%2\n)%3;");
    QString createTable = QString("CREATE TABLE IF NOT EXISTS %1 ");

    QStringList columnDefList;

    // A composite primary key (more than one QiPrimary field) becomes a
    // table-level constraint, so we suppress the per-column PRIMARY KEY.
    const QStringList pkFields = info->primaryKeyFields();
    const bool composite = pkFields.size() > 1;

    int n = info->size();

    for (int i = 0 ; i < n;i++){
        const QiModelMetaInfoField *f = info->at(i);
        QiClause clause = f->clause;   // copy: flag()/testFlag() are non-const

        // A QI_FIELD_AS override wins over the inferred type. An empty override
        // ("") means a typeless column — the type slot is skipped entirely.
        QString typeName;
        if (clause.testFlag(QiClause::SQL_TYPE)) {
            typeName = clause.flag(QiClause::SQL_TYPE).toString();
        } else {
            typeName = columnTypeName(f->type);
            if (typeName.isNull()) {
                qWarning() << QString("%1::%3 - QiField<%2> is not supported yet")
                            .arg(info->name()).arg(QString::fromUtf8(QMetaType(f->type).name())).arg(f->name);
                continue;
            }
        }

        QString cons = columnConstraint(clause, typeName, !composite);
        QString columnDef;
        if (typeName.isEmpty()) {
            // Typeless column: name directly followed by its constraints (if any).
            columnDef = cons.isEmpty() ? f->name
                                       : QString("%1 %2").arg(f->name).arg(cons);
        } else {
            columnDef = QString("%1 %2 %3").arg(f->name).arg(typeName).arg(cons);
        }
        columnDefList << columnDef;
    }

    if (composite)
        columnDefList << QString("PRIMARY KEY (%1)").arg(pkFields.join(", "));

    QList<QiModelMetaInfoField> foreignKeyList = info->foreignKeyList();
    n = foreignKeyList.size();

    for (int i = 0; i < n ;i++){
        QiModelMetaInfoField f = foreignKeyList.at(i);
        QVariant v = f.clause.flag(QiClause::FOREIGN_KEY);
        QiModelMetaInfo * targetInfo = (QiModelMetaInfo*) v.value<void *>();
        Q_ASSERT(targetInfo);

        // Reference the target's actual primary-key column (not always "id").
        QString targetKey = targetInfo->primaryKeyName();
        if (targetKey.isEmpty())
            targetKey = QStringLiteral("id");

        QString columnDef = QString("FOREIGN KEY(%1) REFERENCES %2(%3)")
                            .arg(f.name)
                            .arg(targetInfo->name())
                            .arg(targetKey);

        // Optional ON DELETE referential action.
        QVariant action = f.clause.flag(QiClause::FK_ON_DELETE);
        if (action.isValid()) {
            switch (action.toInt()) {
            case QiFkCascade:    columnDef += " ON DELETE CASCADE";     break;
            case QiFkRestrict:   columnDef += " ON DELETE RESTRICT";    break;
            case QiFkSetNull:    columnDef += " ON DELETE SET NULL";    break;
            case QiFkSetDefault: columnDef += " ON DELETE SET DEFAULT"; break;
            default: break;
            }
        }

        columnDefList << columnDef;
    }

    QString sql;
    sql = statement
          .arg(createTable.arg(info->name()))
          .arg(columnDefList.join(",\n"))
          .arg(info->isWithoutRowid() ? QStringLiteral(" WITHOUT ROWID") : QString());

    return sql;
}

QString QiSqliteStatement::addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field) {
    QiClause clause = field->clause;   // copy: flag()/testFlag() are non-const
    QString typeName;
    if (clause.testFlag(QiClause::SQL_TYPE)) {
        typeName = clause.flag(QiClause::SQL_TYPE).toString();
    } else {
        typeName = columnTypeName(field->type);
        if (typeName.isNull())
            return QString();
    }

    QString sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                    .arg(info->name())
                    .arg(field->name)
                    .arg(typeName).trimmed();   // typeName may be "" (typeless)

    // SQLite's ADD COLUMN only allows a constant DEFAULT (no NOT NULL without a
    // default, no UNIQUE, no PRIMARY KEY). Carry a DEFAULT across if present;
    // otherwise the new column is simply nullable.
    if (clause.testFlag(QiClause::DEFAULT)) {
        QVariant value = clause.flag(QiClause::DEFAULT);
        sql += QString(" DEFAULT %1").arg(value.toString());
    }

    sql += ";";
    return sql;
}

QString QiSqliteStatement::columnTypeName(int type) {
    QString res;
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
        res = "INTEGER";
        break;
    case QMetaType::Double:
        res = "DOUBLE";
        break;
    case QMetaType::QString:
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:   // nested JSON, stored as a JSON string
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:
        res = "TEXT";
        break;
    case QMetaType::QDateTime:
        res = "DATETIME";
        break;
    case QMetaType::QDate:
        res = "DATE";
        break;
    case QMetaType::QByteArray:
        res = "BLOB";
        break;
    case QMetaType::Bool:
        res = "BOOLEAN";
        break;
    default:
        break;
    }

    if (res.isNull()) {
        // Enum fields (QiField<MyEnum>) store their underlying integer value.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        if (QMetaType(type).flags() & QMetaType::IsEnumeration)
#else
        if (QMetaType::typeFlags(static_cast<QMetaType::Type>(type)) & QMetaType::IsEnumeration)
#endif
            res = "INTEGER";
    }

    return res;
}

QString QiSqliteStatement::columnConstraint(QiClause clause, const QString &typeName, bool emitPrimaryKey){
    QStringList res;
    if (clause.testFlag(QiClause::NOT_NULL)) {
        res << "NOT NULL";
    }

    if (clause.testFlag(QiClause::UNIQUE)) {
        res << "UNIQUE";
    }

    if (clause.testFlag(QiClause::DEFAULT)) {
        QVariant value = clause.flag(QiClause::DEFAULT);
//        QString fvalue = formatValue(value,true); // User should format the value by themself.
        QString fvalue = value.toString();

        res << QString("DEFAULT %1 ")
                .arg(fvalue);
    }

    if (clause.testFlag(QiClause::CHECK)) {
        res << QString("CHECK (%1)").arg(clause.flag(QiClause::CHECK).toString());
    }

    if (emitPrimaryKey && clause.testFlag(QiClause::PRIMARY_KEY)) {
        // AUTOINCREMENT is only legal on an INTEGER PRIMARY KEY. For text/other
        // primary keys (ULIDs, userIds, ...) emit a plain PRIMARY KEY instead.
        if (typeName == QLatin1String("INTEGER"))
            res << QString("PRIMARY KEY AUTOINCREMENT");
        else
            res << QString("PRIMARY KEY");
    }

    return res.join(" ");
}

QString QiSqliteStatement::driverName(){
    return "SQLITE";
}

QString QiSqliteStatement::exists(QiModelMetaInfo *info) {
    return QString("SELECT name FROM sqlite_master WHERE type='table' and name ='%1'").arg(info->name());
}
