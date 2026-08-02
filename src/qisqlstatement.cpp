#include <QStringList>
#include <QDebug>

#include "qisqlstatement.h"
#include "qisqlitestatement.h"
#include "qimysqlstatement.h"
#include "qipgstatement.h"
#include "qimssqlstatement.h"
#include "qioraclestatement.h"
#include "qiduckdbstatement.h"
#include "qiexpression.h"
#include "qijoin.h"

QiSqlStatement::QiSqlStatement()
{
}

QiSqlStatement *QiSqlStatement::forDriver(const QString &driverName){
    if (driverName == QLatin1String("QMYSQL") || driverName == QLatin1String("QMARIADB"))
        return new QiMysqlStatement();
    if (driverName == QLatin1String("QPSQL"))
        return new QiPgStatement();
    // QODBC is Qt's generic ODBC driver name — also used for Access, generic ODBC
    // DSNs, etc. — but SQL Server via ODBC is this library's only intended use of it.
    if (driverName == QLatin1String("QODBC"))
        return new QiMsSqlStatement();
    if (driverName == QLatin1String("QOCI"))
        return new QiOracleStatement();
    // DuckDB has no Qt-bundled driver; a QDUCKDB plugin around DuckDB's C API is the
    // intended runtime (see docs). "DUCKDB" is accepted too for embedders that register
    // the driver under the bare name.
    if (driverName == QLatin1String("QDUCKDB") || driverName == QLatin1String("DUCKDB"))
        return new QiDuckDbStatement();
    // QSQLITE and anything else fall back to SQLite.
    return new QiSqliteStatement();
}

QString QiSqlStatement::dropTable(QiModelMetaInfo *info) {
    // IF EXISTS makes dropTable()/dropTables() idempotent: dropping a table that
    // isn't there is a no-op success, not an error.
    QString sql = QString("drop table if exists %1;").arg(info->name());
    return sql;
}

QString QiSqlStatement::createTableIfNotExists(QiModelMetaInfo *info){
    return _createTableIfNotExists(info);
}

QString QiSqlStatement::addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field){
    // Portable "ALTER TABLE ... ADD COLUMN". SQLite overrides this with its own
    // (more restrictive) variant; MySQL / Postgres use this generic one.
    QiClause clause = field->clause;   // copy: flag()/testFlag() are non-const
    QString typeName;
    if (clause.testFlag(QiClause::SQL_TYPE)) {
        typeName = clause.flag(QiClause::SQL_TYPE).toString();
    } else {
        typeName = columnTypeForField(field->type, clause);
        if (typeName.isNull())
            return QString();
    }

    QString sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                    .arg(info->name(), field->name, typeName).trimmed();

    if (clause.testFlag(QiClause::DEFAULT))
        sql += QString(" DEFAULT %1").arg(clause.flag(QiClause::DEFAULT).toString());

    sql += ";";
    return sql;
}

QString QiSqlStatement::columnTypeName(int type){
    // A portable ANSI-ish default. SQLite / MySQL / Postgres each override this.
    switch (type){
    case QMetaType::Int:
    case QMetaType::UInt:          return QStringLiteral("INTEGER");
    case QMetaType::LongLong:
    case QMetaType::ULongLong:     return QStringLiteral("BIGINT");
    case QMetaType::Float:         return QStringLiteral("REAL");
    case QMetaType::Double:        return QStringLiteral("DOUBLE PRECISION");
    case QMetaType::QString:
    case QMetaType::QStringList:
    case QMetaType::QJsonObject:
    case QMetaType::QJsonArray:
    case QMetaType::QVariantMap:
    case QMetaType::QVariantList:  return QStringLiteral("TEXT");
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

QString QiSqlStatement::columnTypeForField(int type, QiClause clause){
    Q_UNUSED(clause);
    return columnTypeName(type);
}

QString QiSqlStatement::primaryKeyClause(const QString &typeName){
    Q_UNUSED(typeName);
    return QStringLiteral("PRIMARY KEY");
}

QString QiSqlStatement::columnConstraint(QiClause clause, const QString &typeName, bool emitPrimaryKey){
    QStringList res;
    if (clause.testFlag(QiClause::NOT_NULL))
        res << "NOT NULL";
    if (clause.testFlag(QiClause::UNIQUE))
        res << "UNIQUE";
    if (clause.testFlag(QiClause::DEFAULT))
        res << QString("DEFAULT %1 ").arg(clause.flag(QiClause::DEFAULT).toString());
    if (clause.testFlag(QiClause::CHECK))
        res << QString("CHECK (%1)").arg(clause.flag(QiClause::CHECK).toString());
    if (emitPrimaryKey && clause.testFlag(QiClause::PRIMARY_KEY))
        res << primaryKeyClause(typeName);
    return res.join(" ");
}

QString QiSqlStatement::tableSuffix(QiModelMetaInfo *info){
    Q_UNUSED(info);
    return QString();
}

QString QiSqlStatement::exists(QiModelMetaInfo *info){
    // information_schema is shared by MySQL and Postgres; SQLite overrides this.
    return QString("SELECT table_name FROM information_schema.tables WHERE table_name = '%1'")
            .arg(info->name());
}

QString QiSqlStatement::_createTableIfNotExists(QiModelMetaInfo *info){
    // Portable CREATE TABLE generator built from the dialect hooks. SQLite keeps
    // its own override; MySQL / Postgres use this one.
    QString statement   = QStringLiteral("%1 (\n%2\n)%3;");
    QString createTable = QStringLiteral("CREATE TABLE IF NOT EXISTS %1 ");

    QStringList columnDefList;

    const QStringList pkFields = info->primaryKeyFields();
    const bool composite = pkFields.size() > 1;

    int n = info->size();
    for (int i = 0; i < n; i++){
        const QiModelMetaInfoField *f = info->at(i);
        QiClause clause = f->clause;

        QString typeName;
        if (clause.testFlag(QiClause::SQL_TYPE)) {
            typeName = clause.flag(QiClause::SQL_TYPE).toString();
        } else {
            typeName = columnTypeForField(f->type, clause);
            if (typeName.isNull()) {
                qWarning() << QString("%1::%3 - QiField<%2> is not supported yet")
                            .arg(info->name()).arg(QString::fromUtf8(QMetaType(f->type).name())).arg(f->name);
                continue;
            }
        }

        QString cons = columnConstraint(clause, typeName, !composite);
        QString columnDef;
        if (typeName.isEmpty()) {
            columnDef = cons.isEmpty() ? f->name : QString("%1 %2").arg(f->name).arg(cons);
        } else {
            columnDef = QString("%1 %2 %3").arg(f->name).arg(typeName).arg(cons);
        }
        columnDefList << columnDef;
    }

    if (composite)
        columnDefList << QString("PRIMARY KEY (%1)").arg(pkFields.join(", "));

    QList<QiModelMetaInfoField> foreignKeyList = info->foreignKeyList();
    n = foreignKeyList.size();
    for (int i = 0; i < n; i++){
        QiModelMetaInfoField f = foreignKeyList.at(i);
        QVariant v = f.clause.flag(QiClause::FOREIGN_KEY);
        QiModelMetaInfo *targetInfo = (QiModelMetaInfo*) v.value<void *>();
        Q_ASSERT(targetInfo);
        QString targetKey = targetInfo->primaryKeyName();
        if (targetKey.isEmpty()) targetKey = QStringLiteral("id");
        QString columnDef = QString("FOREIGN KEY(%1) REFERENCES %2(%3)")
                            .arg(f.name).arg(targetInfo->name()).arg(targetKey);
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

    return statement
          .arg(createTable.arg(info->name()))
          .arg(columnDefList.join(",\n"))
          .arg(tableSuffix(info));
}

QString QiSqlStatement::renameColumn(QiModelMetaInfo *info, const QString &from, const QString &to){
    return QString("ALTER TABLE %1 RENAME COLUMN %2 TO %3;")
            .arg(info->name(), from, to);
}

QString QiSqlStatement::dropColumn(QiModelMetaInfo *info, const QString &name){
    return QString("ALTER TABLE %1 DROP COLUMN %2;")
            .arg(info->name(), name);
}

QString QiSqlStatement::createIndexIfNotExists(const QiBaseIndex& index){
    QString createIndex = index.isUnique()
            ? QStringLiteral("CREATE UNIQUE INDEX IF NOT EXISTS %1 on %2 (%3)")
            : QStringLiteral("CREATE INDEX IF NOT EXISTS %1 on %2 (%3)");

    QString sql = createIndex.arg(index.name())
                             .arg(index.metaInfo()->name())
                             .arg(index.columnDefList().join(","));

    if (!index.where().isEmpty())          // partial index
        sql += QString(" WHERE %1").arg(index.where());

    sql += ";";

    return sql;
}

QString QiSqlStatement::dropIndexIfExists(QString name){
    QString createIndex = "DROP INDEX IF EXISTS %1;";

    QString sql = createIndex.arg(name);

    return sql;
}

QStringList QiSqlStatement::createFtsIndex(const QiBaseFtsIndex &index){
    QStringList stmts;

    QString fts   = index.name();
    QString table = index.metaInfo()->name();
    QStringList cols = index.columns();
    QString colList = cols.join(",");

    QStringList newCols, oldCols;
    foreach (QString c, cols) {
        newCols << "new." + c;
        oldCols << "old." + c;
    }

    // 1. The FTS5 virtual table using the model's table as external content.
    stmts << QString("CREATE VIRTUAL TABLE IF NOT EXISTS %1 USING fts5(%2, content='%3', content_rowid='id');")
             .arg(fts).arg(colList).arg(table);

    // 2. Triggers that keep the index in sync with the base table.
    stmts << QString("CREATE TRIGGER IF NOT EXISTS %1_ai AFTER INSERT ON %2 BEGIN "
                     "INSERT INTO %1(rowid,%3) VALUES (new.id,%4); END;")
             .arg(fts).arg(table).arg(colList).arg(newCols.join(","));

    stmts << QString("CREATE TRIGGER IF NOT EXISTS %1_ad AFTER DELETE ON %2 BEGIN "
                     "INSERT INTO %1(%1,rowid,%3) VALUES('delete',old.id,%4); END;")
             .arg(fts).arg(table).arg(colList).arg(oldCols.join(","));

    stmts << QString("CREATE TRIGGER IF NOT EXISTS %1_au AFTER UPDATE ON %2 BEGIN "
                     "INSERT INTO %1(%1,rowid,%3) VALUES('delete',old.id,%4); "
                     "INSERT INTO %1(rowid,%3) VALUES (new.id,%5); END;")
             .arg(fts).arg(table).arg(colList).arg(oldCols.join(",")).arg(newCols.join(","));

    // 3. Backfill the index from existing rows.
    stmts << QString("INSERT INTO %1(%1) VALUES('rebuild');").arg(fts);

    return stmts;
}

QStringList QiSqlStatement::dropFtsIndex(QString name){
    QStringList stmts;
    stmts << QString("DROP TRIGGER IF EXISTS %1_ai;").arg(name);
    stmts << QString("DROP TRIGGER IF EXISTS %1_ad;").arg(name);
    stmts << QString("DROP TRIGGER IF EXISTS %1_au;").arg(name);
    stmts << QString("DROP TABLE IF EXISTS %1;").arg(name);
    return stmts;
}

QString QiSqlStatement::insertInto(QiModelMetaInfo *info,QStringList fields){
    return _insertInto(info,"INSERT",fields);
}

QString QiSqlStatement::replaceInto(QiModelMetaInfo *info,QStringList fields){
    return _insertInto(info,"REPLACE",fields);
}

QString QiSqlStatement::upsertInto(QiModelMetaInfo *info,QStringList fields,QStringList conflictColumns){
    QStringList values;
    foreach (QString f, fields) {
        values << ":" + f;
    }

    // The SET clause updates every inserted column except the conflict key(s)
    // and the primary key, using the values that would have been inserted.
    QStringList setList;
    foreach (QString f, fields) {
        if (f == "id" || conflictColumns.contains(f))
            continue;
        setList << QString("%1=excluded.%1").arg(f);
    }

    QString action;
    if (setList.isEmpty()) {
        action = "DO NOTHING";
    } else {
        action = QString("DO UPDATE SET %1").arg(setList.join(","));
    }

    QString sql = QString("INSERT INTO %1 (%2) values (%3) ON CONFLICT(%4) %5;")
                    .arg(info->name())
                    .arg(fields.join(","))
                    .arg(values.join(","))
                    .arg(conflictColumns.join(","))
                    .arg(action);

    return sql;
}

QString QiSqlStatement::_insertInto(QiModelMetaInfo *info ,QString type, QStringList fields){
    QString sql,format;
    QStringList values;

    format = QString("%4 INTO %1 (%2) values (%3);");

    foreach (QString f, fields) {
        values << ":" + f;
    }

    sql = format.arg(info->name(), fields.join(","),values.join(",") , type);

    return sql;
}


QString QiSqlStatement::select(QiSharedQuery query) {
    QiQueryRules rules;
    rules =  query;
    QStringList sql;

    // Correct SQL clause order: ... WHERE / GROUP BY / HAVING (in selectCore),
    // then ORDER BY, then LIMIT / OFFSET.
    sql << selectCore(rules);

    if (rules.orderBy().size() > 0) {
        sql << orderBy(rules);
    }

    if (rules.limit() > 0 || rules.offset() > 0) {
        sql << limitAndOffset(rules.limit() > 0 ? rules.limit() : -1,
                              rules.offset() > 0 ? rules.offset() : 0);
    }

    sql << ";";

    return sql.join(" ");
}

QString QiSqlStatement::deleteFrom(QiSharedQuery query) {
    QiQueryRules rules;
    rules =  query;
    QStringList sql;

    sql << QString("DELETE FROM %1").arg(rules.metaInfo()->name());

    QiExpression expression = rules.expression();
    if (!expression.isNull()) {

//        sql << QString("WHERE %1").arg(where.toString());
        sql << QString("WHERE %1").arg(expression.string());

    }

    /// @todo Implemente order by

    if (rules.limit() > 0) {
        sql << limitAndOffset(rules.limit());
    }

    sql << ";";

    return sql.join(" ");
}

QString QiSqlStatement::update(QiSharedQuery query, const QStringList &fields) {
    QiQueryRules rules;
    rules = query;
    QStringList sql;

    QStringList assignments;
    foreach (QString field, fields)
        assignments << QString("%1 = :set_%1").arg(field);

    sql << QString("UPDATE %1 SET %2")
            .arg(rules.metaInfo()->name())
            .arg(assignments.join(", "));

    QiExpression expression = rules.expression();
    if (!expression.isNull())
        sql << QString("WHERE %1").arg(expression.string());

    sql << ";";

    return sql.join(" ");
}

QString QiSqlStatement::selectCore(QiQueryRules rules){
    QStringList res;

    QString quantifier = rules.distinct() ? "DISTINCT" : "ALL";
    res << QString("SELECT %1 %2 FROM %3").arg(quantifier).arg(selectResultColumn(rules)).arg(rules.metaInfo()->name());

    QString joins = joinClause(rules);
    if (!joins.isEmpty()) {
        res << joins;
    }

    QiExpression expression = rules.expression();
    if (!expression.isNull()) {
//        res << QString("WHERE %1").arg(where.toString());
        res << QString("WHERE %1").arg(expression.string());
    }

    if (rules.groupBy().size() > 0) {
        res << QString("GROUP BY %1").arg(rules.groupBy().join(","));
    }

    QiExpression having = rules.having();
    if (!having.isNull()) {
        // Namespace the placeholders (":arg0" -> ":harg0") so they do not
        // collide with the WHERE clause's placeholders. exec() renames the
        // bound values the same way.
        QString havingStr = having.string();
        havingStr.replace(QLatin1String(":arg") , QLatin1String(":harg"));
        res << QString("HAVING %1").arg(havingStr);
    }

    return res.join(" ");
}

QString QiSqlStatement::joinClause(QiQueryRules rules){
    QList<QiBaseJoin> joins = rules.joins();
    if (joins.size() == 0)
        return QString();

    QStringList res;

    for (int j = 0 ; j < joins.size() ; j++) {
        QiBaseJoin join = joins.at(j);

        QString clause = QString("%1 %2").arg(join.keyword()).arg(join.table());

        QiWhere on = join.resolvedOn(rules.metaInfo());
        if (!on.isNull()) {
            QiExpression expression(on);
            QString onString = expression.string();
            // Namespace the placeholders so they do not collide with the
            // filter's placeholders (or other joins'). QiSharedQuery::exec()
            // applies the same renaming when binding the values.
            onString.replace(QLatin1String(":arg") , QString(":j%1arg").arg(j));
            clause += QString(" ON %1").arg(onString);
        }

        res << clause;
    }

    return res.join(" ");
}

QString QiSqlStatement::selectResultColumn(QiQueryRules rules){
    QString res;
    QStringList fields = rules.fields();
    QString func = rules.func();

    if (fields.size() > 0) {
        res = fields.join(",");
    } else if (func.isEmpty() && rules.joins().size() > 0) {
        // A join is present but no explicit fields were requested. Selecting
        // "*" would pull in the joined tables' columns and make column names
        // (such as "id") ambiguous when mapping the result back to the primary
        // model. Qualify the primary model's columns explicitly instead.
        QiModelMetaInfo *info = rules.metaInfo();
        QString table = info->name();
        QStringList cols;
        int n = info->size();
        for (int i = 0 ; i < n ; i++) {
            cols << QString("%1.%2").arg(table).arg(info->at(i)->name);
        }
        res = cols.join(",");
    } else {
        res = "*";
    }

    if (!func.isEmpty()) {
        res = QString("%1(%2)").arg(func).arg(res);
    }
    return res;
}

QString QiSqlStatement::limitAndOffset(int limit, int offset) {
    QStringList res;
    res << QString("LIMIT %1").arg(limit);
    if (offset > 0) {
        res << QString("OFFSET %1").arg(offset);
    }
    return res.join(" ");
}

QString QiSqlStatement::orderBy(QiQueryRules rules){
    QStringList orderingTerms;

    orderingTerms << "ORDER BY";
    orderingTerms << rules.orderBy().join(",");

    return orderingTerms.join(" ");
}

QString QiSqlStatement::formatValue(QVariant value,bool trimStrings) {
    QString res;

    switch (value.userType() ){

    case QMetaType::QString:
    case QMetaType::QChar:
        res = value.toString();
        if (trimStrings)
            res = res.trimmed();
        res.replace(QLatin1Char('\''), QLatin1String("''"));
        res = QString("'%1'").arg(res);
        break;

    default:
        // @todo Implement more data type

        res = value.toString();
        break;

    }

    return res;
}
