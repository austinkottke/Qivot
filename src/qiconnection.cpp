#include <QtCore>
#include <QSqlDatabase>
#include <QCoreApplication>
#include <QSqlError>
#include <QMutex>
#include <QPointer>
#include <QThread>
#include <QFileInfo>
#include <QAtomicInt>

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

    ~QiConnectionPriv();

    QiSql m_sql;

    /// Registered modeles
    QList<QiModelMetaInfo*> m_models;

    /// The last query being used.
    QSqlQuery *lastQuery;

    /// The last error from a connection-level operation
    QiError lastError;

    /// Reactive change listeners
    struct Hook {
        int id;
        std::function<void(const QiChange&)> fn;
        QPointer<QObject> context;   // where it runs; null and !hasContext: the writer's thread
        bool hasContext;
    };
    QVector<Hook> changeHooks;

    /// Which database this connection writes to, for sharing changes with every
    /// other connection to it (see QiChangeBus). Empty until open().
    QString busKey;

    /// Changes made inside a transaction, reported when it commits
    QVector<QiChange> pendingChanges;
    QVector<int> pendingMarks;       // pendingChanges.size() at each SAVEPOINT
    bool plainTransaction = false;   // transaction() .. commit()/rollback()

    /// Nesting depth for QiTransaction (0 = none, 1 = outermost BEGIN, >1 = SAVEPOINT)
    int txnDepth = 0;

    QMutex mutex;
};

// --- QiChangeBus: changes shared by every connection to one database ----------
//
// Each open connection is filed under its database (the SQLite file, or the
// server, port and database name). A write through any of them is reported to
// the hooks of all of them, so a list on the UI thread hears about a save made
// by a QiAsync worker on its own pooled connection.
namespace {
struct QiChangeBus {
    QMutex mutex;
    QHash<QString, QVector<QiConnectionPriv *>> byDatabase;

    static QiChangeBus &instance() {
        // Never destroyed: connections (the global default one among them) are
        // destroyed during static teardown and still unfile themselves.
        static QiChangeBus *bus = new QiChangeBus;
        return *bus;
    }

    void file(QiConnectionPriv *conn, const QString &key) {
        QMutexLocker lock(&mutex);
        unfileLocked(conn);
        conn->busKey = key;
        if (!key.isEmpty())
            byDatabase[key].append(conn);
    }

    void unfileLocked(QiConnectionPriv *conn) {
        if (conn->busKey.isEmpty())
            return;
        auto it = byDatabase.find(conn->busKey);
        if (it != byDatabase.end()) {
            it->removeAll(conn);
            if (it->isEmpty())
                byDatabase.erase(it);
        }
        conn->busKey.clear();
    }

    void unfile(QiConnectionPriv *conn) {
        QMutexLocker lock(&mutex);
        unfileLocked(conn);
    }

    // Every hook of every connection to the source's database (the source's own
    // hooks too, even when it isn't filed: not open, or opened under no name).
    QVector<QiConnectionPriv::Hook> hooksFor(QiConnectionPriv *source) {
        QVector<QiConnectionPriv::Hook> hooks;
        QMutexLocker lock(&mutex);
        QVector<QiConnectionPriv *> conns;
        if (!source->busKey.isEmpty())
            conns = byDatabase.value(source->busKey);
        if (!conns.contains(source))
            conns.prepend(source);
        for (QiConnectionPriv *conn : conns) {
            QMutexLocker connLock(&conn->mutex);
            hooks += conn->changeHooks;
        }
        return hooks;
    }
};

// The database a connection writes to, as a key shared by every connection to it.
QString qiDatabaseKey(const QSqlDatabase &db) {
    const QString driver = db.driverName();
    const QString name = db.databaseName();
    if (driver == QLatin1String("QSQLITE")) {
        if (name.isEmpty() || name == QLatin1String(":memory:") || name.contains(QLatin1String("mode=memory")))
            return QStringLiteral("sqlite-memory:") + db.connectionName();   // private to the connection
        const QFileInfo file(name);
        const QString path = file.exists() ? file.canonicalFilePath() : file.absoluteFilePath();
        return QStringLiteral("sqlite:") + path;
    }
    return QStringLiteral("%1://%2:%3/%4").arg(driver, db.hostName().toLower())
                                          .arg(db.port()).arg(name);
}

QAtomicInt qiNextHookId(1);

void qiDeliver(const QVector<QiConnectionPriv::Hook> &hooks, const QiChange &change) {
    for (const auto &h : hooks) {
        if (!h.hasContext) {
            h.fn(change);                              // on the writer's thread
        } else if (h.context) {
            if (h.context->thread() == QThread::currentThread()) {
                h.fn(change);
            } else {
                auto fn = h.fn;                        // on the context's thread
                QMetaObject::invokeMethod(h.context.data(), [fn, change]() { fn(change); },
                                          Qt::QueuedConnection);
            }
        }
    }
}
} // namespace

static void qiFinishTransaction(QiConnection &conn, QExplicitlySharedDataPointer<QiConnectionPriv> &d, bool committed);

QiConnectionPriv::~QiConnectionPriv() {
    QiChangeBus::instance().unfile(this);
    if (lastQuery)
        delete lastQuery;
}

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

    const QString driver = db.driverName();
    const bool sqlite = (driver == QLatin1String("QSQLITE"));

    // Supported drivers: SQLite, MySQL/MariaDB, PostgreSQL, SQL Server (QODBC),
    // Oracle (QOCI), and DuckDB (a QDUCKDB driver an embedder registers around
    // DuckDB's C API — Qt ships none). This allow-list is deliberately separate from
    // QiSqlStatement::forDriver()'s own driver-name dispatch — both must be kept
    // in sync, or a new dialect either can't open a connection at all (rejected
    // here first) or silently falls back to the SQLite statement generator
    // (forDriver()'s fallback) despite this gate having let the driver through.
    if (!sqlite
        && driver != QLatin1String("QMYSQL")
        && driver != QLatin1String("QMARIADB")
        && driver != QLatin1String("QPSQL")
        && driver != QLatin1String("QODBC")
        && driver != QLatin1String("QOCI")
        && driver != QLatin1String("QDUCKDB")
        && driver != QLatin1String("DUCKDB")) {
        qWarning() << "Unsupported SQL driver:" << driver;
        setLastError(QiError(QiError::NotSupported,
                             QStringLiteral("Unsupported SQL driver: %1").arg(driver)));
        return false;
    }

    if (asDefault
        && !m_defaultConnection.isOpen()
        && this != &m_defaultConnection
        ) {

        d.operator = (m_defaultConnection.d); // It become the default connection

    }

    d->m_sql.setStatement(QiSqlStatement::forDriver(driver));
    d->m_sql.setDatabase(db);
    QiChangeBus::instance().file(d.data(), qiDatabaseKey(db));   // share changes with its other connections

    if (sqlite) {
        // SQLite-only session settings. Enable recursive triggers so REPLACE-based
        // save() fires the DELETE triggers that keep an FTS index in sync, and turn
        // on foreign-key enforcement (SQLite ignores FKs by default). MySQL and
        // Postgres enforce foreign keys natively.
        d->m_sql.query().exec("PRAGMA recursive_triggers = ON");
        d->m_sql.query().exec("PRAGMA foreign_keys = ON");
    }

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
    QiChangeBus::instance().unfile(d.data());
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
    const bool ok = d->m_sql.transaction();
    if (ok && d->txnDepth == 0)
        d->plainTransaction = true;
    return ok;
}

bool QiConnection::commit() {
    const bool ok = d->m_sql.commit();
    if (ok && d->plainTransaction) {
        d->plainTransaction = false;
        qiFinishTransaction(*this, d, true);
    }
    return ok;
}

bool QiConnection::rollback() {
    const bool ok = d->m_sql.rollback();
    if (d->plainTransaction) {
        d->plainTransaction = false;
        qiFinishTransaction(*this, d, false);
    }
    return ok;
}

int QiConnection::beginScope() {
    const int depth = d->txnDepth + 1;
    bool ok;
    if (depth == 1) {
        ok = d->m_sql.transaction();                 // outermost: real BEGIN

        // The counter said no transaction was open and SQLite disagreed, so an
        // earlier COMMIT or ROLLBACK failed and left one behind. Clear the stray
        // one and take the BEGIN again: without this the connection refuses
        // every transaction for the rest of the process's life, and a caller
        // whose only symptom is "insert failed" has no way to see why.
        //
        // Gated on the message rather than run for any failure, because a BEGIN
        // also fails on a plain lock — and rolling back on THAT would throw away
        // a transaction somebody else is still filling.
        if (!ok && d->m_sql.database().lastError().text()
                       .contains(QStringLiteral("within a transaction"))) {
            d->m_sql.rollback();
            ok = d->m_sql.transaction();
        }
    } else {
        QSqlQuery q = d->m_sql.query();              // nested: SAVEPOINT
        ok = q.exec(QStringLiteral("SAVEPOINT qi_sp_%1").arg(depth));
    }
    if (!ok)
        return 0;
    d->txnDepth = depth;
    if (depth > 1) {
        QMutexLocker lock(&d->mutex);
        d->pendingMarks.append(d->pendingChanges.size());   // a savepoint's changes start here
    }
    return depth;
}

bool QiConnection::commitScope(int depth) {
    if (depth <= 0)
        return false;
    bool ok;
    if (depth == 1) {
        ok = d->m_sql.commit();

        // A COMMIT THAT FAILS LEAVES THE TRANSACTION OPEN.
        /*
          The depth is about to drop to 0 either way — the scope is over and its
          owner is gone. If SQLite is still holding a transaction at that point,
          the next scope believes it is the outermost, issues a real BEGIN, and
          gets "cannot start a transaction within a transaction" — and so does
          every scope after it, forever, because nothing ever closes the stray.
          One failed commit under lock contention was enough to stop a running
          process writing for good.

          So force the connection into the state the counter is about to claim.
          The work is lost regardless: the commit is what failed.
         */
        if (!ok)
            d->m_sql.rollback();
    } else {
        QSqlQuery q = d->m_sql.query();
        ok = q.exec(QStringLiteral("RELEASE qi_sp_%1").arg(depth));

        // Same reasoning one level in: a RELEASE that fails leaves the savepoint
        // on the stack, and the name is reused by the next scope at this depth.
        if (!ok) {
            q.exec(QStringLiteral("ROLLBACK TO qi_sp_%1").arg(depth));
            q.exec(QStringLiteral("RELEASE qi_sp_%1").arg(depth));
        }
    }
    if (d->txnDepth == depth)
        d->txnDepth = depth - 1;
    if (depth == 1) {
        qiFinishTransaction(*this, d, ok);      // a failed commit rolled back
    } else {
        QMutexLocker lock(&d->mutex);
        if (!d->pendingMarks.isEmpty())
            d->pendingMarks.removeLast();      // released into the enclosing transaction
    }
    return ok;
}

bool QiConnection::rollbackScope(int depth) {
    if (depth <= 0)
        return false;
    bool ok;
    if (depth == 1) {
        ok = d->m_sql.rollback();
    } else {
        // RELEASE even if the ROLLBACK TO failed. The two used to be chained
        // with &&, which skipped the RELEASE on exactly the occasions the
        // savepoint most needed taking off the stack.
        QSqlQuery q = d->m_sql.query();
        ok = q.exec(QStringLiteral("ROLLBACK TO qi_sp_%1").arg(depth));
        ok = q.exec(QStringLiteral("RELEASE qi_sp_%1").arg(depth)) && ok;
    }
    if (d->txnDepth == depth)
        d->txnDepth = depth - 1;
    if (depth == 1) {
        qiFinishTransaction(*this, d, false);
    } else {
        QMutexLocker lock(&d->mutex);           // forget what the savepoint changed
        if (!d->pendingMarks.isEmpty())
            d->pendingChanges.resize(d->pendingMarks.takeLast());
    }
    return ok;
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

int QiConnection::addChangeHook(std::function<void(const QString&)> hook, QObject *context){
    return addRowChangeHook([hook](const QiChange &change) { hook(change.table); }, context);
}

int QiConnection::addRowChangeHook(std::function<void(const QiChange&)> hook, QObject *context){
    QMutexLocker lock(&d->mutex);
    const int id = qiNextHookId.fetchAndAddRelaxed(1);
    d->changeHooks.append(QiConnectionPriv::Hook{ id, std::move(hook), QPointer<QObject>(context), context != nullptr });
    return id;
}

void QiConnection::removeChangeHook(int id){
    QMutexLocker lock(&d->mutex);
    for (int i = 0 ; i < d->changeHooks.size() ; i++) {
        if (d->changeHooks.at(i).id == id) {
            d->changeHooks.removeAt(i);
            break;
        }
    }
}

void QiConnection::notifyChanged(const QString &table){
    QiChange change;
    change.table = table;
    change.kind = QiChange::Many;
    notifyChanged(change);
}

void QiConnection::notifyChanged(const QiChange &change){
    {
        // Inside a transaction: hold it until the commit. Another connection
        // couldn't see the row yet, and a rollback means it never happened.
        QMutexLocker lock(&d->mutex);
        if (d->txnDepth > 0 || d->plainTransaction) {
            d->pendingChanges.append(change);
            return;
        }
    }
    qiDeliver(QiChangeBus::instance().hooksFor(d.data()), change);
}

// The outermost transaction ended: report what it changed, or forget it.
static void qiFinishTransaction(QiConnection &conn, QExplicitlySharedDataPointer<QiConnectionPriv> &d, bool committed) {
    QVector<QiChange> changes;
    {
        QMutexLocker lock(&d->mutex);
        changes.swap(d->pendingChanges);
        d->pendingMarks.clear();
    }
    if (committed) {
        for (const QiChange &change : changes)
            conn.notifyChanged(change);
    }
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
    if (d->lastQuery != nullptr)        // nothing has run yet
        query = *d->lastQuery;
    d->mutex.unlock();

    return query;
}
