#include <QtCore>
#include <QSqlError>
#include <QSharedDataPointer>
#include "qimodel.h"
#include "qisql.h"
#include "qisqlitestatement.h"
#include "qilog.h"

class QiSqlPriv : public QSharedData {
public:
    QiSqlPriv()  {
    }

    ~QiSqlPriv(){
    }

    QSharedPointer<QiSqlStatement> m_statement;

    QSqlDatabase m_db;

    /// The query object used in last operation
    QSharedPointer<QSqlQuery> m_lastQuery;

    QMutex m_mutex;
};

/* QiSql */

QiSql::QiSql(QiSqlStatement *statement)
{
    d = new QiSqlPriv();
    if (statement) {
        d->m_statement = QSharedPointer<QiSqlStatement>(statement);
    }
}

QiSql::QiSql(const QiSql& other) : d(other.d) {

}

QiSql::~QiSql(){
}

QiSql& QiSql::operator=(const QiSql &rhs){
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

void QiSql::setStatement(QiSqlStatement *statement) {
    d->m_statement = QSharedPointer<QiSqlStatement>(statement);
}

QiSqlStatement* QiSql::statement() {
    return d->m_statement.data();
}

void QiSql::setDatabase(QSqlDatabase db){
    d->m_db = db;
}

QSqlDatabase QiSql::database(){
    return d->m_db;
}

bool QiSql::createTableIfNotExists(QiModelMetaInfo* info){
    QString sql = d->m_statement->createTableIfNotExists(info);

    QSqlQuery q = query();

//    d->m_lastQuery = new QSqlQuery(query());
    bool ret = q.exec(sql);
    setLastQuery(q);

    return ret;
}

QSqlQuery QiSql::query(){
    return QSqlQuery(d->m_db);
}

QSqlQuery QiSql::lastQuery(){
    return QSqlQuery(*d->m_lastQuery);
}

void QiSql::setLastQuery(QSqlQuery value)
{
    QiLog::logQuery(value);   // trace every write / DDL statement (SELECTs log in QiSharedQuery)

    d->m_mutex.lock();
    if (d->m_lastQuery == 0) {
        d->m_lastQuery = QSharedPointer<QSqlQuery>(new QSqlQuery(query()));
    }

    if (value.isActive())
        value.finish();

    *d->m_lastQuery = value;
    d->m_mutex.unlock();
}

bool QiSql::dropTable(QiModelMetaInfo* info){
    QString sql = d->m_statement->dropTable(info);

    QSqlQuery q = query();

    bool res = q.exec(sql);

    setLastQuery(q);

    return res;

//    QString sql = d->m_statement->dropTable(info);

//    d->m_lastQuery = new QSqlQuery(query());

//    return d->m_lastQuery->exec(sql);
}

bool QiSql::createIndexIfNotExists(const QiBaseIndex &index) {
    QString sql = d->m_statement->createIndexIfNotExists(index);

    QSqlQuery q = query();
    bool res = q.exec(sql);

    setLastQuery(q);

    return res;
}

bool QiSql::transaction(){
    return d->m_db.transaction();
}

bool QiSql::commit(){
    return d->m_db.commit();
}

bool QiSql::rollback(){
    return d->m_db.rollback();
}

QStringList QiSql::columnNames(QiModelMetaInfo* info){
    QStringList cols;
    QSqlQuery q = query();
    if (q.exec(QString("PRAGMA table_info(%1)").arg(info->name()))) {
        while (q.next()) {
            cols << q.value(1).toString(); // column 1 of table_info is the name
        }
    }
    setLastQuery(q);
    return cols;
}

bool QiSql::addColumn(QiModelMetaInfo* info, const QiModelMetaInfoField* field){
    QString sql = d->m_statement->addColumn(info,field);
    if (sql.isEmpty())
        return true; // unsupported field type: nothing to migrate

    QSqlQuery q = query();
    bool res = q.exec(sql);
    setLastQuery(q);
    return res;
}

bool QiSql::renameColumn(QiModelMetaInfo* info, const QString &from, const QString &to){
    QSqlQuery q = query();
    bool res = q.exec(d->m_statement->renameColumn(info, from, to));
    setLastQuery(q);
    return res;
}

bool QiSql::dropColumn(QiModelMetaInfo* info, const QString &name){
    QSqlQuery q = query();
    bool res = q.exec(d->m_statement->dropColumn(info, name));
    setLastQuery(q);
    return res;
}

bool QiSql::dropIndexIfExists(QString name){
    QString sql = d->m_statement->dropIndexIfExists(name);

    QSqlQuery q = query();
    bool res = q.exec(sql);

    setLastQuery(q);

    return res;
}

bool QiSql::createFtsIndex(const QiBaseFtsIndex &index){
    QStringList stmts = d->m_statement->createFtsIndex(index);

    QSqlQuery q = query();
    bool res = true;
    foreach (QString sql , stmts) {
        if (!q.exec(sql)) {
            res = false;
            break;
        }
    }

    setLastQuery(q);

    return res;
}

bool QiSql::dropFtsIndex(QString name){
    QStringList stmts = d->m_statement->dropFtsIndex(name);

    QSqlQuery q = query();
    bool res = true;
    foreach (QString sql , stmts) {
        if (!q.exec(sql)) {
            res = false;
            break;
        }
    }

    setLastQuery(q);

    return res;
}

bool QiSql::exists(QiModelMetaInfo* info){
    if (d->m_db.driverName() != "QSQLITE") {
        qWarning() << "Only QSQLITE dirver is supported.";
        return false;
    }

    QString sql = QiSqliteStatement::exists(info);
//    qDebug() << sql;
    QSqlQuery q = query();

    bool res = false;
    if (q.exec(sql)) {
        if (q.next())
            res = true;
    }

    setLastQuery(q);

    return res;
}

bool QiSql::insertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool updateId) {
    return insertInto(info,model,fields,updateId,false);
}

bool QiSql::replaceInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool updateId){
    return insertInto(info,model,fields,updateId,true);
}

bool QiSql::upsertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,QStringList conflictColumns,bool updateId){
    QString sql = d->m_statement->upsertInto(info,fields,conflictColumns);

    QSqlQuery q = query();
    q.prepare(sql);

    foreach (QString field , fields) {
        q.bindValue(":" + field , info->value(model,field,true));
    }

    bool res = false;

    if (q.exec()) {
        res = true;
        if (updateId) {
            int id = q.lastInsertId().toInt();
            if (id != 0 && model->id.get().toInt() != id)
                model->id.set(id);
        }
    }

    setLastQuery(q);

    return res;
}

bool QiSql::insertIntoBatch(QiModelMetaInfo* info,const QList<QiModel*>& models,QStringList fields,bool replace){
    QString sql;
    if (replace) {
        sql = d->m_statement->replaceInto(info,fields);
    } else {
        sql = d->m_statement->insertInto(info,fields);
    }

    QSqlQuery q = query();
    if (!q.prepare(sql)) {
        setLastQuery(q);
        return false;
    }

    bool res = true;
    foreach (QiModel *model , models) {
        foreach (QString field , fields) {
            q.bindValue(":" + field , info->value(model,field,true));
        }
        if (!q.exec()) {
            res = false;
            break;
        }
        int id = q.lastInsertId().toInt();
        if (id != 0 && model->id.get().toInt() != id)
            model->id.set(id);
    }

    setLastQuery(q);

    return res;
}

bool QiSql::insertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool updateId,bool replace){
    QString sql;

    QSqlQuery q = query();

    if (replace){
        sql = d->m_statement->replaceInto(info,fields);
    } else {
        sql = d->m_statement->insertInto(info,fields);
    }

//    qDebug() << sql;
    q.prepare(sql);

    foreach (QString field , fields) {
        QVariant value;
        value = info->value(model,field,true);
//        qDebug() << "bind " << field;
        q.bindValue(":" + field , value);
    }

    bool res = false;

    if (q.exec()) {
        res = true;
        if (updateId) {
            int id = q.lastInsertId().toInt();
            if (model->id.get().toInt() != id)
                model->id.set(id);
        }
    }

    setLastQuery(q);

    return res;
}
