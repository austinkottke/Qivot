#include <QtCore>
#include <QMetaObject>
#include <QMetaProperty>
#include <QSqlError>
#include "qimodel.h"
#include "qimetainfoquery_p.h"
#include "qilist.h"
#include "qilog.h"

#include "qisql.h"

//#define TABLE_NAME "Model without QI_MODEL"
#define TABLE_NAME ""


QiModel::QiModel() : m_connection ( QiConnection::defaultConnection()){

}

QiModel::QiModel(QiConnection connection) : m_connection(connection)
{
}

QiModel::~QiModel(){
}

QString QiModel::tableName() const{
    return TABLE_NAME;
}

QString QiModel::TableName() {
    return TABLE_NAME;
}

void QiModel::setConnection(QiConnection connection){
    m_connection = connection;
}

QiConnection QiModel::connection(){
    return m_connection;
}

QiError QiModel::lastError() const {
    return m_error;
}

void QiModel::setError(const QiError &error) {
    m_error = error;
    if (error.isValid())
        QiLog::write(QiLog::Model, QiLog::Error,
                     tableName() + QLatin1String(": ") + error.text());
}

void QiModel::setError(const QString &message) {
    setError(QiError(QiError::ValidationError, message));
}

bool QiModel::save(bool forceInsert,bool forceAllField) {
    m_error.clear();
    if (!clean() ) {
        if (!m_error.isValid())
            m_error = QiError(QiError::ValidationError, QStringLiteral("clean() rejected the record"));
        return false;
    }
    QiModelMetaInfo *info = metaInfo();
    Q_ASSERT(info);

    QStringList fields = info->fieldNameList();
    QStringList nonNullFields;
    if (forceAllField) {

        nonNullFields = fields;

    } else {

    foreach (QString field , fields) {
        QVariant v = info->value(this,field);
        if (forceInsert && field == "id" ) // skip id field when forceInsert
            continue;

        if (!v.isNull() ) {
//            qDebug() << field;
            nonNullFields << field;
        }
    }

    }

    bool res ;

    QiSql sql = m_connection.sql();

    if (forceInsert || id->isNull() ) {
        res = sql.replaceInto(info,this,nonNullFields,true);
    } else {
        res = sql.replaceInto(info,this,nonNullFields,false);
    }

    if (!res)
        m_error = QiError(QiError::StatementError, sql.lastQuery().lastError().text());

    m_connection.setLastQuery(sql.lastQuery());

    if (res)
        m_connection.notifyChanged(tableName());   // reactive: views watching this table refresh

    return res;
}

bool QiModel::upsert(const QStringList &conflictColumns, bool forceAllField) {
    m_error.clear();
    if (!clean()) {
        if (!m_error.isValid())
            m_error = QiError(QiError::ValidationError, QStringLiteral("clean() rejected the record"));
        return false;
    }
    QiModelMetaInfo *info = metaInfo();
    Q_ASSERT(info);

    QStringList fields = info->fieldNameList();
    QStringList writeFields;
    if (forceAllField) {
        writeFields = fields;
    } else {
        foreach (QString field , fields) {
            QVariant v = info->value(this,field);
            if (field == "id" && v.isNull()) // let the DB assign the id
                continue;
            if (!v.isNull())
                writeFields << field;
        }
    }

    QiSql sql = m_connection.sql();
    bool res = sql.upsertInto(info,this,writeFields,conflictColumns,id->isNull());

    if (!res)
        m_error = QiError(QiError::StatementError, sql.lastQuery().lastError().text());

    m_connection.setLastQuery(sql.lastQuery());

    if (res)
        m_connection.notifyChanged(tableName());   // reactive

    return res;
}

bool QiModel::load(QiWhere where){
    m_error.clear();
    bool res = false;

    _QiMetaInfoQuery query( metaInfo() ,  m_connection);

    query = query.filter(where).limit(1);
    if (query.exec()){
        if (query.next()){
            res = query.recordTo(this);
        } else {
            m_error = QiError(QiError::NotFound, QStringLiteral("no matching record"));
        }
    } else {
        m_error = QiError(QiError::StatementError, query.lastQuery().lastError().text());
    }

    if (!res)
        id->clear();

    m_connection.setLastQuery(query.lastQuery());

    return res;
}

bool QiModel::remove() {
    m_error.clear();
    QiModelMetaInfo *info = metaInfo();

    // Delete by the built-in "id" column when the schema has one; otherwise
    // (a QI_DECLARE_MODEL_NOID model) delete by the declared primary key —
    // which may be composite, so match on every primary-key column.
    QStringList pkFields = info->primaryKeyFields();
    const bool hasIdColumn = pkFields.contains(QStringLiteral("id"));

    QiWhere filter;
    bool haveFilter = false;

    if (hasIdColumn && !id->isNull()) {
        filter = QiWhere(QStringLiteral("id")) == id();
        haveFilter = true;
    } else {
        pkFields.removeAll(QStringLiteral("id"));
        bool ok = !pkFields.isEmpty();
        for (int i = 0; i < pkFields.size(); i++) {
            QVariant val = info->value(this, pkFields.at(i));
            if (val.isNull()) { ok = false; break; }
            QiWhere cond = QiWhere(pkFields.at(i)) == val;
            filter = (i == 0) ? cond : (filter && cond);
        }
        haveFilter = ok;
    }

    if (!haveFilter) {
        m_error = QiError(QiError::ValidationError,
                          QStringLiteral("cannot remove a record with no id / primary-key value"));
        return false;
    }

    _QiMetaInfoQuery query( info ,  m_connection);

    query = query.filter( filter );

    bool res = query.remove();
    if (res){
        id->clear();
    } else {
        m_error = QiError(QiError::StatementError, query.lastQuery().lastError().text());
    }

    m_connection.setLastQuery( query.lastQuery());

    if (res)
        m_connection.notifyChanged(info->name());   // reactive

    return res;
}

bool QiModel::clean(){
    return true;
}

QiSharedList QiModel::initialData() const {
    return QiSharedList();
}
