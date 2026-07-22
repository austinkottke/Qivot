#include <QtCore>
#include <QSqlDatabase>
#include <QCoreApplication>
#include <QSqlError>
#include <QMutex>

#include "qimodel.h"
#include "qiconnection.h"
#include "qisqlitestatement.h"
#include "qisql.h"
#include "qilog.h"

class QiConnectionPriv : public QSharedData
{
  public:
    QiConnectionPriv() {
        lastQuery = nullptr;
    }

    ~QiConnectionPriv() {
        if (lastQuery)
            delete lastQuery;
    }

    QiSql m_sql;

    /// Registered modeles
    QList<QiModelMetaInfo*> m_models;

    /// The last query being used.
    QSqlQuery *lastQuery;

    /// The last error from a connection-level operation
    QiError lastError;

    QMutex mutex;
};

/// The default connection shared for all objects
QiConnection m_defaultConnection;

QiConnection::QiConnection()
{
    d = new QiConnectionPriv();
}

QiConnection::QiConnection(const QiConnection& other) : d(other.d){
}

QiConnection & QiConnection::operator=(const QiConnection &rhs){
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

QiConnection::~QiConnection(){
}

bool QiConnection::operator==(const QiConnection &rhs) {
    return d.constData() == rhs.d.constData();
}

bool QiConnection::operator!=(const QiConnection &rhs) {
    return d.constData() != rhs.d.constData();
}

bool QiConnection::open(QSqlDatabase db){
    return open(db, true);
}

bool QiConnection::open(QSqlDatabase db, bool asDefault){
    Q_ASSERT(db.isOpen());

    if (db.driverName() != "QSQLITE") {
        qWarning() << "Only QSQLITE dirver is supported.";
        setLastError(QiError(QiError::NotSupported, QStringLiteral("Only the QSQLITE driver is supported")));
        return false;
    }

    if (asDefault
        && !m_defaultConnection.isOpen()
        && this != &m_defaultConnection
        ) {

        d.operator = (m_defaultConnection.d); // It become the default connection

    }

    d->m_sql.setStatement(new QiSqliteStatement());
    d->m_sql.setDatabase(db);

    // Enable recursive triggers so REPLACE-based save() fires the DELETE
    // triggers that keep an FTS index (and any user triggers) in sync.
    d->m_sql.query().exec("PRAGMA recursive_triggers = ON");

    // Enforce foreign keys (SQLite ignores them by default).
    d->m_sql.query().exec("PRAGMA foreign_keys = ON");

    QiLog::write(QiLog::Connection, QiLog::Info,
                 QStringLiteral("opened \"%1\"%2")
                     .arg(db.databaseName(),
                          asDefault ? QStringLiteral(" (default)") : QString()));
    return true;
}

bool QiConnection::isOpen(){
    return d->m_sql.database().isOpen();
}

void QiConnection::close(){
    QiLog::write(QiLog::Connection, QiLog::Info, QStringLiteral("closed"));
    if (d->lastQuery) {
        delete d->lastQuery;
        d->lastQuery = 0;
    }
    d->m_sql.setDatabase(QSqlDatabase());
}

bool QiConnection::addModel(QiModelMetaInfo* metaInfo){
    bool res = false;
    if (!metaInfo) {
        return res;
    }

    if (!d->m_models.contains(metaInfo)) {
        d->m_models << metaInfo;
        res = true;
    }
    return res;
}

QiConnection QiConnection::defaultConnection(){
    return m_defaultConnection;
}

void QiConnection::setToDefaultConnection(){
    if (this != &m_defaultConnection) {
        m_defaultConnection.d.operator =(d);
    }
}

bool QiConnection::createTables(){

    bool res = true;
    for (QiModelMetaInfo *info : d->m_models) {

        if (!d->m_sql.exists(info)) {

            if (!d->m_sql.createTableIfNotExists(info)){
                QString err = d->m_sql.lastQuery().lastError().text();
                qWarning() << QString("QiConnection::createTables() - Failed to create table for %1 . Error : %2").arg(info->className())
                        .arg( err );
                qWarning() << d->m_sql.lastQuery().lastQuery();
                res = false;
                setLastQuery( d->m_sql.lastQuery() );
                setLastError(QiError(QiError::StatementError, err));
                break;
            }

            QiSharedList initialData = info->initialData();
            int n = initialData.size();
            for (int i = 0 ; i< n;i++) {
                initialData.at(i)->save();
            }

        } else {

            // The table already exists : migrate it by adding any columns that
            // the model declares but the table is missing (ALTER TABLE ADD COLUMN).
            QStringList existing = d->m_sql.columnNames(info);
            int n = info->size();
            for (int i = 0 ; i < n ; i++) {
                const QiModelMetaInfoField *field = info->at(i);
                if (existing.contains(field->name))
                    continue;
                if (!d->m_sql.addColumn(info,field)) {
                    QString err = d->m_sql.lastQuery().lastError().text();
                    qWarning() << QString("QiConnection::createTables() - Failed to add column %1.%2 : %3")
                                  .arg(info->name()).arg(field->name).arg(err);
                    res = false;
                    setLastQuery( d->m_sql.lastQuery() );
                    setLastError(QiError(QiError::StatementError, err));
                    break;
                }
            }
            if (!res)
                break;
        }
    }

    return res;
}

bool QiConnection::dropTables() {
    bool res = true;

    for (QiModelMetaInfo *info : d->m_models) {
        if (!d->m_sql.exists(info))
            continue;

        if (!d->m_sql.dropTable(info) ) {
            res = false;
            setLastQuery( d->m_sql.lastQuery() );
            setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
            break;
        }

    }

    return res;
}

bool QiConnection::createIndex(const QiBaseIndex &index) {
    bool res = d->m_sql.createIndexIfNotExists(index);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

bool QiConnection::dropIndex(QString name){
    bool res = d->m_sql.dropIndexIfExists(name);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

bool QiConnection::renameColumn(QiModelMetaInfo *metaInfo, const QString &from, const QString &to){
    bool res = d->m_sql.renameColumn(metaInfo, from, to);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

bool QiConnection::dropColumn(QiModelMetaInfo *metaInfo, const QString &name){
    bool res = d->m_sql.dropColumn(metaInfo, name);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

bool QiConnection::transaction() {
    return d->m_sql.transaction();
}

bool QiConnection::commit() {
    return d->m_sql.commit();
}

bool QiConnection::rollback() {
    return d->m_sql.rollback();
}

bool QiConnection::setForeignKeysEnforced(bool enabled) {
    QSqlQuery q = d->m_sql.query();
    return q.exec(QString("PRAGMA foreign_keys = %1").arg(enabled ? "ON" : "OFF"));
}

bool QiConnection::setJournalMode(const QString &mode) {
    QSqlQuery q = d->m_sql.query();
    bool res = q.exec(QString("PRAGMA journal_mode = %1").arg(mode));
    if (!res)
        setLastError(QiError(QiError::StatementError, q.lastError().text()));
    return res;
}

bool QiConnection::createFtsIndex(const QiBaseFtsIndex &index) {
    bool res = d->m_sql.createFtsIndex(index);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

bool QiConnection::dropFtsIndex(QString name) {
    bool res = d->m_sql.dropFtsIndex(name);
    if (!res)
        setLastError(QiError(QiError::StatementError, d->m_sql.lastQuery().lastError().text()));
    return res;
}

QiSql& QiConnection::sql(){
    return d->m_sql;
}

QSqlQuery QiConnection::query(){
    return d->m_sql.query();
}

void QiConnection::setLastQuery(QSqlQuery query){
    d->mutex.lock();
    if (d->lastQuery != nullptr)
        delete d->lastQuery;
    d->lastQuery = new QSqlQuery(query);
    d->mutex.unlock();
}

QiError QiConnection::lastError(){
    QiError error;
    d->mutex.lock();
    error = d->lastError;
    d->mutex.unlock();
    return error;
}

void QiConnection::setLastError(const QiError &error){
    // Statement errors are already logged (with SQL + params) by QiLog::logQuery,
    // so only surface the non-statement ones here to avoid duplicate lines.
    if (error.isValid() && error.type() != QiError::StatementError)
        QiLog::write(QiLog::Connection, QiLog::Error, error.text());

    d->mutex.lock();
    d->lastError = error;
    d->mutex.unlock();
}

QSqlQuery QiConnection::lastQuery(){
    /*
      Although lastQuery() is thread-safe, but as it do not hold the
      lastQuery per thread. The result become meaningless , as it
      may override by another thread.
     @todo Implement last query storage per thread.
     */
    QSqlQuery query;
    d->mutex.lock();
    query = *d->lastQuery;
    d->mutex.unlock();

    return query;
}
