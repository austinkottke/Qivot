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
    query.data->expression = QiExpression(where);
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

bool QiSharedQuery::exec() {
    data->query = data->connection.query();

    Q_ASSERT(data->connection.isOpen());

    QString sql;
    sql = data->connection.sql().statement()->select(*this);

    data->query.prepare(sql);

    QiExpression& expression = data->expression;
    QMap<QString,QVariant> values = expression.bindValues();
    QMapIterator<QString, QVariant> iter(values);

    while (iter.hasNext()) {
        iter.next();
        data->query.bindValue(iter.key() , iter.value());
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
            data->query.bindValue(key , onIter.value());
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
            data->query.bindValue(key , hIter.value());
        }
    }

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

    if (res) {
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

    if (ok) {
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
    int res = 0;
    data->func = "count";

    if (exec()) {
        if (next()){
            res = value().toInt();
        }
    }
    return res;
}

QVariant QiSharedQuery::call(QString func , QStringList fields){
    data->func = func;
    data->fields = fields;

    QVariant res;
    if (exec()) {
        if (next()){
            res = value();
        }
    }

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
    bool res = true;

    QSqlRecord record = data->query.record();

    int count = record.count();
    for (int i = 0 ; i < count;i++){
        QString field = record.fieldName(i);
        res = data->metaInfo->setValue(model,field,record.value(i));
        if (!res)
            break;
    }

    return res;
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
