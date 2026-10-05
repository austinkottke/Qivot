#include <QSharedData>
#include <QSqlRecord>
#include <QElapsedTimer>

#include "qisql.h"
#include "qilog.h"
#include "qiconnection.h"
#include "qisharedquery.h"
#include "qisharedquery_p.h"
#include "qisqlstatement.h"
#include "qiexpression.h"

QiSharedQuery::QiSharedQuery() : data(new QiSharedQueryPriv) {
    data->connection = QiConnection::defaultConnection();
}

QiSharedQuery::QiSharedQuery(QiConnection connection) : data(new QiSharedQueryPriv)
{
    data->connection = connection;
}

QiSharedQuery::QiSharedQuery(const QiSharedQuery &rhs) : data(rhs.data)
{
}

QiSharedQuery &QiSharedQuery::operator=(const QiSharedQuery &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QiSharedQuery::~QiSharedQuery()
{
}

void QiSharedQuery::setConnection(QiConnection connection) {
    data->connection = connection;
}

void QiSharedQuery::setMetaInfo(QiModelMetaInfo *info){
    data->metaInfo = info;
}

QiSharedQuery QiSharedQuery::select(QStringList fields) {
    QiSharedQuery query(*this);
    query.data->fields = fields;
    return query;
}

QiSharedQuery QiSharedQuery::select(QString field) {
    QiSharedQuery query(*this);
    QStringList fields;
    fields << field;
    query.data->fields = fields;
    return query;
}

QiSharedQuery QiSharedQuery::filter(QiWhere where) {
    QiSharedQuery query(*this);
    // CHAINED FILTERS AND TOGETHER; they do not replace one another.
    /*
      This assigned the expression outright, so of

          objects().filter(instrument == sym).filter(periodSec == n)

      only the LAST clause survived and the first was silently dropped. Every
      row of every other instrument came back, from a query that reads as
      though it scopes -- the failure is invisible until a second value exists
      in the column, which is why it can sit in a schema for a long time and
      then surface all at once as another instrument's data.

      AND is what a reader of a chain expects and what the ORMs this API
      follows do. A single filter() on a fresh query is unchanged, which is
      every call that was already correct.
     */
    if (query.data->filterWhere.isNull())
        query.data->filterWhere = where;
    else
        query.data->filterWhere = query.data->filterWhere && where;
    query.data->expression = QiExpression(query.data->filterWhere);
    return query;
}

QiSharedQuery QiSharedQuery::join(const QiBaseJoin &join) {
    QiSharedQuery query(*this);
    query.data->joins.append(join);
    return query;
}

QiSharedQuery QiSharedQuery::distinct(bool enabled) {
    QiSharedQuery query(*this);
    query.data->distinct = enabled;
    return query;
}

QiSharedQuery QiSharedQuery::limit(int val){
    QiSharedQuery query(*this);
    query.data->limit = val;
    return query;
}

QiSharedQuery QiSharedQuery::offset(int val){
    QiSharedQuery query(*this);
    query.data->offset = val;
    return query;
}

QiSharedQuery QiSharedQuery::groupBy(QStringList terms){
    QiSharedQuery query(*this);
    query.data->groupBy = terms;
    return query;
}

QiSharedQuery QiSharedQuery::groupBy(QString term){
    QiSharedQuery query(*this);
    query.data->groupBy = QStringList() << term;
    return query;
}

QiSharedQuery QiSharedQuery::having(QiWhere where){
    QiSharedQuery query(*this);
    query.data->having = QiExpression(where);
    return query;
}

QiSharedQuery QiSharedQuery::orderBy(QStringList terms){
    QiSharedQuery query(*this);
    query.data->orderBy = terms;
    return query;
}

QiSharedQuery QiSharedQuery::orderBy(QString term){
    QiSharedQuery query(*this);
    QStringList fields;
    fields << term;
    query.data->orderBy = fields;
    return query;
}

void QiSharedQuery::bindAll(QSqlQuery &query) {
    QiExpression& expression = data->expression;
    QMap<QString,QVariant> values = expression.bindValues();
    QMapIterator<QString, QVariant> iter(values);

    while (iter.hasNext()) {
        iter.next();
        query.bindValue(iter.key() , iter.value());
    }

    // Bind the values found in the ON condition of each JOIN clause. Their
    // placeholders are renamed (":arg0" -> ":j0arg0") by the statement
    // generator so that they do not collide with the filter's placeholders or
    // with the placeholders of other joins.
    for (int j = 0 ; j < data->joins.size() ; j++) {
        QiWhere on = data->joins.at(j).on();
        if (on.isNull())
            continue;

        QiExpression onExpression(on);
        QMap<QString,QVariant> onValues = onExpression.bindValues();
        QMapIterator<QString, QVariant> onIter(onValues);

        while (onIter.hasNext()) {
            onIter.next();
            QString key = onIter.key();
            key.replace(QLatin1String(":arg") , QString(":j%1arg").arg(j));
            query.bindValue(key , onIter.value());
        }
    }

    // Bind the HAVING clause values. Their placeholders are renamed
    // (":arg0" -> ":harg0") by the statement generator so they do not collide
    // with the filter's placeholders.
    if (!data->having.isNull()) {
        QMap<QString,QVariant> havingValues = data->having.bindValues();
        QMapIterator<QString, QVariant> hIter(havingValues);
        while (hIter.hasNext()) {
            hIter.next();
            QString key = hIter.key();
            key.replace(QLatin1String(":arg") , QLatin1String(":harg"));
            query.bindValue(key , hIter.value());
        }
    }
}

bool QiSharedQuery::exec() {
    data->query = data->connection.query();

    Q_ASSERT(data->connection.isOpen());

    QString sql;
    sql = data->connection.sql().statement()->select(*this);

    data->query.prepare(sql);
    bindAll(data->query);

    QElapsedTimer timer;
    timer.start();
    bool res = data->query.exec();
    QiLog::logQuery(data->query, timer.nsecsElapsed());

    data->connection.setLastQuery(data->query);

    if (!res) {
        qWarning() << QString("Failed : %1").arg(data->query.executedQuery());
    }

    return res;
}

bool QiSharedQuery::remove(){
    data->query = data->connection.query();

    QString sql;
    sql = data->connection.sql().statement()->deleteFrom(*this);

    data->query.prepare(sql);

    QiExpression &expression = data->expression;
    QMap<QString,QVariant> values = expression.bindValues();
    QMapIterator<QString, QVariant> iter(values);

    while (iter.hasNext()) {
        iter.next();
        data->query.bindValue(iter.key() , iter.value());
    }

    QElapsedTimer timer;
    timer.start();
    bool res = data->query.exec();
    QiLog::logQuery(data->query, timer.nsecsElapsed());

    data->connection.setLastQuery(data->query);

    if (res && data->notify) {
        QiQueryRules rules; rules = *this;
        if (rules.metaInfo())
            data->connection.notifyChanged(rules.metaInfo()->name());   // reactive
    }

    return res;
}

int QiSharedQuery::update(const QVariantMap &values) {
    if (values.isEmpty())
        return 0;

    data->query = data->connection.query();

    const QStringList fields = values.keys();
    QString sql = data->connection.sql().statement()->update(*this, fields);
    data->query.prepare(sql);

    // Bind the SET values (":set_<field>") ...
    for (auto it = values.constBegin(); it != values.constEnd(); ++it)
        data->query.bindValue(QLatin1String(":set_") + it.key(), it.value());

    // ... and the WHERE filter's values (":argN").
    QiExpression &expression = data->expression;
    QMap<QString, QVariant> whereValues = expression.bindValues();
    QMapIterator<QString, QVariant> iter(whereValues);
    while (iter.hasNext()) {
        iter.next();
        data->query.bindValue(iter.key(), iter.value());
    }

    QElapsedTimer timer;
    timer.start();
    bool ok = data->query.exec();
    QiLog::logQuery(data->query, timer.nsecsElapsed());

    data->connection.setLastQuery(data->query);

    if (ok && data->notify) {
        QiQueryRules rules; rules = *this;
        if (rules.metaInfo())
            data->connection.notifyChanged(rules.metaInfo()->name());   // reactive
    }

    return ok ? data->query.numRowsAffected() : -1;
}

QiSharedList QiSharedQuery::all(){
    QiSharedList res;
    if (exec()) {
        while (next() ) {
            QiAbstractModel* model = data->metaInfo->create();
            QiSharedQuery::recordTo(model);
            res.append(model);
        }
    }

    return res;
}

QVariantList QiSharedQuery::ids(bool *ok) {
    QVariantList res;
    QiModelMetaInfo *info = data->metaInfo;
    if (!info) {
        if (ok) *ok = false;
        return res;
    }
    const QString idColumn = info->name() + QLatin1String(".id");
    QiSharedQuery q(*this);
    q.data->func.clear();
    q.data->fields = QStringList{ idColumn };
    q.data->orderBy << idColumn;   // the same total order rowOf() uses
    const bool good = q.exec();
    if (good) {
        while (q.next())
            res << q.value(0);
    }
    if (ok) *ok = good;
    return res;
}

int QiSharedQuery::rowOf(const QVariant &id, bool *ok) {
    if (ok) *ok = false;
    if (!data->metaInfo || data->distinct || data->limit > 0 || data->offset > 0
            || !data->groupBy.isEmpty() || !data->func.isEmpty())
        return -1;

    QSqlQuery q = data->connection.query();
    if (!q.prepare(data->connection.sql().statement()->rowPosition(*this)))
        return -1;
    bindAll(q);
    q.bindValue(QStringLiteral(":qi_id"), id);

    QElapsedTimer timer;
    timer.start();
    const bool good = q.exec();
    QiLog::logQuery(q, timer.nsecsElapsed());
    if (!good)
        return -1;
    if (ok) *ok = true;
    return q.next() ? q.value(0).toInt() - 1 : -1;
}

void QiSharedQuery::setNotifyChanges(bool notify) {
    data->notify = notify;
}

QiConnection QiSharedQuery::connection() const {
    return data->connection;
}

QSqlQuery QiSharedQuery::lastQuery(){
    return data->query;
}

void QiSharedQuery::reset(){
    QiConnection conn = data->connection;
    QiModelMetaInfo* metaInfo = data->metaInfo ;
    data.operator =(new QiSharedQueryPriv);
    data->connection = conn;
    data->metaInfo = metaInfo;
}

bool QiSharedQuery::next() {
    return data->query.next();
}

QVariant QiSharedQuery::value() {
    QSqlRecord record = data->query.record();

    QVariant res = record.value(0);

    return res;
}

QVariant QiSharedQuery::value(int index) {
    return data->query.record().value(index);
}

int QiSharedQuery::count(){
    // On a copy: the query itself stays as it was, for all() afterwards. And
    // without its ORDER BY, which SQL Server and PostgreSQL refuse next to an
    // aggregate.
    QiSharedQuery q(*this);
    q.data->func = "count";
    q.data->orderBy.clear();

    int res = 0;
    if (q.exec()) {
        if (q.next()){
            res = q.value().toInt();
        }
    }
    data->query = q.data->query;
    return res;
}

QVariant QiSharedQuery::call(QString func , QStringList fields){
    QiSharedQuery q(*this);
    q.data->func = func;
    q.data->fields = fields;
    q.data->orderBy.clear();

    QVariant res;
    if (q.exec()) {
        if (q.next()){
            res = q.value();
        }
    }
    data->query = q.data->query;

    return res;
}

QVariant QiSharedQuery::call(QString func , QString field){
    QStringList fields;
    fields << field;
    return call(func,fields);
}

bool QiSharedQuery::recordTo(QiAbstractModel *model) {
    Q_ASSERT (data->metaInfo);
    Q_ASSERT (data->metaInfo == model->metaInfo() );

    QSqlRecord record = data->query.record();

    // A column the model doesn't declare is skipped, not fatal. Tables gain
    // columns (a migration, another app, a DBA), and an older model must keep
    // loading the columns it does know — as qiRawQuery already does. This used
    // to stop at the first unknown column and report failure, so one added
    // column made every load of the table fail.
    int count = record.count();
    for (int i = 0 ; i < count;i++)
        data->metaInfo->setValue(model, record.fieldName(i), record.value(i));

    return true;
}

bool QiSharedQuery::get(QiAbstractModel* model){
    Q_ASSERT (data->metaInfo);
    Q_ASSERT (data->metaInfo == model->metaInfo() );

    data->limit = 1;
    bool res = false;

    if ( exec() ) {
        if (next()){
            res = recordTo(model);
        }
    }

    return res;
}
