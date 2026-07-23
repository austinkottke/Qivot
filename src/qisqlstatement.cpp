#include <QStringList>

#include "qisqlstatement.h"
#include "qiexpression.h"
#include "qijoin.h"

QiSqlStatement::QiSqlStatement()
{
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
    // Overridden by the concrete (SQLite) generator.
    Q_UNUSED(info);
    Q_UNUSED(field);
    return QString();
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
