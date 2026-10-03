#include <algorithm>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include "qimigrator.h"
#include "qisql.h"
#include "qilog.h"

QiMigrator::QiMigrator(QiConnection connection)
    : m_conn(connection) {
}

// --- Registering ------------------------------------------------------------

void QiMigrator::add(int version, const QString &name, Step up, Step down) {
    Entry e{ version, name, std::move(up), std::move(down), QString(), QString(), QString() };
    for (Entry &have : m_steps) {
        if (have.version == version) { have = std::move(e); return; }   // the last registration wins
    }
    m_steps.append(std::move(e));
}

void QiMigrator::addSql(int version, const QString &name, const QString &upSql, const QString &downSql) {
    add(version, name, Step(), Step());
    for (Entry &e : m_steps) {
        if (e.version != version)
            continue;
        e.sql = upSql;
        e.downSql = downSql;
        e.checksum = checksum(upSql);
    }
}

int QiMigrator::addDirectory(const QString &dir) {
    m_error.clear();
    QDir d(dir);
    if (!d.exists()) {
        failed(QStringLiteral("no migrations folder at %1").arg(dir));
        return -1;
    }

    // 0001_create_tables.sql, 0002_add_body.up.sql + 0002_add_body.down.sql
    static const QRegularExpression fileName(
        QStringLiteral("^(\\d+)(?:[_-](.*?))?(?:\\.(up|down))?\\.sql$"),
        QRegularExpression::CaseInsensitiveOption);

    struct Found { QString name; QString up; QString down; bool hasUp = false; bool hasDown = false; };
    QMap<int, Found> found;

    const QStringList files = d.entryList(QStringList() << QStringLiteral("*.sql") << QStringLiteral("*.SQL"),
                                          QDir::Files, QDir::Name);
    for (const QString &f : files) {
        const QRegularExpressionMatch m = fileName.match(f);
        if (!m.hasMatch()) {
            failed(QStringLiteral("%1: a migration file is named <version>_<name>.sql").arg(f));
            return -1;
        }
        const int version = m.captured(1).toInt();
        if (version <= 0) {
            failed(QStringLiteral("%1: versions start at 1").arg(f));
            return -1;
        }
        QFile file(d.filePath(f));
        if (!file.open(QIODevice::ReadOnly)) {
            failed(QStringLiteral("%1: %2").arg(f, file.errorString()));
            return -1;
        }
        const QString text = QString::fromUtf8(file.readAll());
        const bool down = m.captured(3).compare(QLatin1String("down"), Qt::CaseInsensitive) == 0;

        Found &e = found[version];
        if (down ? e.hasDown : e.hasUp) {
            failed(QStringLiteral("%1: version %2 is used by more than one file").arg(f).arg(version));
            return -1;
        }
        if (down) { e.down = text; e.hasDown = true; }
        else      { e.up = text;   e.hasUp = true; }
        if (e.name.isEmpty() || !down)
            e.name = m.captured(2).replace(QLatin1Char('_'), QLatin1Char(' ')).trimmed();
    }

    for (auto it = found.constBegin(); it != found.constEnd(); ++it) {
        if (!it->hasUp) {
            failed(QStringLiteral("version %1 has a .down.sql file but no migration").arg(it.key()));
            return -1;
        }
    }
    for (auto it = found.constBegin(); it != found.constEnd(); ++it)
        addSql(it.key(), it->name, it->up, it->down);
    return found.size();
}

// --- Status -----------------------------------------------------------------

QString QiMigrator::driver() const {
    return m_conn.sql().database().driverName();
}

bool QiMigrator::isSqlite() const {
    return driver() == QLatin1String("QSQLITE");
}

// The SQLite-only migrator kept one history, in user_version; the default table
// takes over from it. A table of your own (setTable) is a separate history and
// leaves user_version alone.
bool QiMigrator::usesUserVersion() const {
    return isSqlite() && m_table.compare(QLatin1String("qivot_migrations"), Qt::CaseInsensitive) == 0;
}

bool QiMigrator::tableExists() const {
    QSqlDatabase db = m_conn.sql().database();
    if (db.tables().contains(m_table, Qt::CaseInsensitive))
        return true;
    // Some MySQL client libraries list no tables at all; ask the server.
    const QString d = driver();
    if (d == QLatin1String("QMYSQL") || d == QLatin1String("QMARIADB")) {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("SELECT COUNT(*) FROM information_schema.tables "
                                 "WHERE table_schema = DATABASE() AND table_name = ?"));
        q.addBindValue(m_table);
        return q.exec() && q.next() && q.value(0).toInt() > 0;
    }
    return false;
}

bool QiMigrator::begin() {
    // SQLite rebuilds a table to change it (create, copy, drop, rename). With
    // foreign keys enforced, dropping the old parent table deletes or blocks
    // its children, and `PRAGMA foreign_keys` does nothing inside a
    // transaction — so they go off here, before it, and back on in end(), with
    // foreignKeysHold() checking the result first (SQLite's own procedure for
    // schema changes).
    m_restoreForeignKeys = false;
    if (isSqlite()) {
        QSqlQuery q = m_conn.query();
        if (q.exec(QStringLiteral("PRAGMA foreign_keys")) && q.next() && q.value(0).toInt() == 1) {
            q.finish();
            m_restoreForeignKeys = m_conn.query().exec(QStringLiteral("PRAGMA foreign_keys = OFF"));
        }
    }

    // A driver without transactions (some MySQL builds) runs the step bare,
    // which is what MySQL does with DDL anyway.
    m_inTransaction = m_conn.sql().database().driver()->hasFeature(QSqlDriver::Transactions);
    if (!m_inTransaction)
        return true;
    if (m_conn.transaction())
        return true;
    end(false);
    return false;
}

bool QiMigrator::foreignKeysHold() {
    if (!m_restoreForeignKeys)
        return true;
    QSqlQuery q = m_conn.query();
    if (q.exec(QStringLiteral("PRAGMA foreign_key_check")) && q.next()) {
        m_error = QStringLiteral("it leaves rows in %1 whose references to %2 don't resolve")
                      .arg(q.value(0).toString(), q.value(2).toString());
        return false;
    }
    return true;
}

bool QiMigrator::end(bool commit) {
    bool ok = commit;
    if (m_inTransaction) {
        m_inTransaction = false;
        ok = commit && m_conn.commit();
        if (!ok)
            m_conn.rollback();
    }
    if (m_restoreForeignKeys) {
        m_restoreForeignKeys = false;
        m_conn.query().exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    }
    return ok;
}

QVector<QiMigrator::Entry> QiMigrator::ordered() const {
    QVector<Entry> out = m_steps;
    std::sort(out.begin(), out.end(), [](const Entry &a, const Entry &b) { return a.version < b.version; });
    return out;
}

static int sqliteUserVersion(QiConnection &conn) {
    QSqlQuery q = conn.query();
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return 0;
}

QMap<int, QiMigrator::Row> QiMigrator::appliedRows(bool *ok) const {
    QMap<int, Row> rows;
    if (ok) *ok = true;

    if (!tableExists()) {
        // A database from the SQLite-only migrator: everything registered up to
        // its user_version counts as applied (ensureTable() writes that down).
        if (usesUserVersion()) {
            const int uv = sqliteUserVersion(m_conn);
            for (const Entry &e : m_steps)
                if (e.version <= uv)
                    rows.insert(e.version, Row{ e.name, e.checksum, QDateTime(), -1 });
        }
        return rows;
    }

    QSqlQuery q = m_conn.query();
    if (!q.exec(QStringLiteral("SELECT version, name, checksum, applied_at, duration_ms FROM %1").arg(m_table))) {
        if (ok) *ok = false;
        return rows;
    }
    while (q.next()) {
        QDateTime at = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
        if (at.isValid())
            at = at.toUTC();
        rows.insert(q.value(0).toInt(),
                    Row{ q.value(1).toString(), q.value(2).toString(), at,
                         q.value(4).isNull() ? -1 : q.value(4).toInt() });
    }
    return rows;
}

int QiMigrator::currentVersion() const {
    const QMap<int, Row> rows = appliedRows();
    int v = rows.isEmpty() ? 0 : rows.lastKey();
    if (usesUserVersion() && !tableExists())
        v = qMax(v, sqliteUserVersion(m_conn));
    return v;
}

int QiMigrator::targetVersion() const {
    int t = 0;
    for (const Entry &e : m_steps)
        t = qMax(t, e.version);
    return t;
}

QVector<QiMigrator::Migration> QiMigrator::status() const {
    const QMap<int, Row> rows = appliedRows();
    QMap<int, Migration> all;

    for (const Entry &e : m_steps) {
        Migration &m = all[e.version];
        m.version = e.version;
        m.name = e.name;
        m.checksum = e.checksum;
        m.sql = e.sql;
        m.downSql = e.downSql;
        m.known = true;
        m.reversible = e.down || !e.downSql.trimmed().isEmpty();
    }
    for (auto it = rows.constBegin(); it != rows.constEnd(); ++it) {
        Migration &m = all[it.key()];
        m.version = it.key();
        if (m.name.isEmpty())
            m.name = it->name;
        m.applied = true;
        m.appliedAt = it->appliedAt;
        m.durationMs = it->durationMs;
        m.appliedChecksum = it->checksum;
        m.changed = m.known && !m.checksum.isEmpty() && !it->checksum.isEmpty() && m.checksum != it->checksum;
    }
    QVector<Migration> out;
    for (const Migration &m : all)
        out << m;
    return out;
}

QVector<QiMigrator::Migration> QiMigrator::pending() const {
    QVector<Migration> out;
    for (const Migration &m : status())
        if (m.known && !m.applied)
            out << m;
    return out;
}

QVector<QiMigrator::Migration> QiMigrator::changed() const {
    QVector<Migration> out;
    for (const Migration &m : status())
        if (m.changed)
            out << m;
    return out;
}

// --- Running ----------------------------------------------------------------

int QiMigrator::failed(const QString &message) {
    m_error = message;
    QiLog::write(QiLog::Sql, QiLog::Error, message);
    return -1;
}

bool QiMigrator::ensureTable() {
    if (tableExists())
        return true;

    const int adopt = usesUserVersion() ? sqliteUserVersion(m_conn) : 0;

    // Plain types every dialect takes; applied_at is ISO-8601 text so it reads
    // back the same everywhere.
    QSqlQuery q = m_conn.query();
    if (!q.exec(QStringLiteral("CREATE TABLE %1 (version INTEGER NOT NULL PRIMARY KEY, "
                               "name VARCHAR(255) NOT NULL, checksum VARCHAR(64), "
                               "applied_at VARCHAR(32), duration_ms INTEGER)").arg(m_table))) {
        failed(QStringLiteral("could not create %1: %2").arg(m_table, q.lastError().text().trimmed()));
        return false;
    }

    // Upgrading from user_version: what it says ran, ran.
    for (const Entry &e : ordered()) {
        if (e.version > adopt)
            break;
        QSqlQuery ins = m_conn.query();
        ins.prepare(QStringLiteral("INSERT INTO %1 (version, name, checksum, applied_at, duration_ms) "
                                   "VALUES (?, ?, ?, ?, ?)").arg(m_table));
        ins.addBindValue(e.version);
        ins.addBindValue(e.name);
        ins.addBindValue(e.checksum);
        ins.addBindValue(QString());
        ins.addBindValue(-1);
        if (!ins.exec()) {
            failed(QStringLiteral("could not record migration %1: %2").arg(e.version).arg(ins.lastError().text().trimmed()));
            return false;
        }
    }
    if (adopt > 0)
        QiLog::write(QiLog::Sql, QiLog::Info,
                     QStringLiteral("%1 created; migrations up to user_version %2 recorded as applied").arg(m_table).arg(adopt));
    return true;
}

// A statement without its comments (for recognising it, and for messages).
static QString codeOf(const QString &statement) {
    static const QRegularExpression comments(QStringLiteral("--[^\\n]*|/\\*.*?\\*/"),
                                             QRegularExpression::DotMatchesEverythingOption);
    return QString(statement).remove(comments).simplified();
}

bool QiMigrator::runSql(const QString &sql) {
    // The migrator runs each migration in a transaction of its own, so a file's
    // BEGIN / COMMIT (from a dump, or written out of habit) is left out.
    static const QRegularExpression control(
        QStringLiteral("^(BEGIN( (TRANSACTION|TRAN|WORK|DEFERRED|IMMEDIATE|EXCLUSIVE)( TRANSACTION)?)?|"
                       "START TRANSACTION|COMMIT( (TRANSACTION|TRAN|WORK))?|END( TRANSACTION)?)$"),
        QRegularExpression::CaseInsensitiveOption);
    for (const QString &statement : splitStatements(sql, driver())) {
        if (control.match(codeOf(statement)).hasMatch())
            continue;
        QSqlQuery q = m_conn.query();
        if (!q.exec(statement)) {
            QString shown = codeOf(statement);
            if (shown.size() > 120)
                shown = shown.left(117) + QStringLiteral("...");
            m_error = QStringLiteral("%1 (in: %2)").arg(q.lastError().text().trimmed(), shown);
            return false;
        }
    }
    return true;
}

bool QiMigrator::runStep(const Entry &e, bool up) {
    m_error.clear();
    if (up)
        return e.up ? e.up(m_conn) : runSql(e.sql);
    if (e.down)
        return e.down(m_conn);
    return runSql(e.downSql);
}

bool QiMigrator::record(const Entry &e, int durationMs) {
    QSqlQuery q = m_conn.query();
    q.prepare(QStringLiteral("INSERT INTO %1 (version, name, checksum, applied_at, duration_ms) "
                             "VALUES (?, ?, ?, ?, ?)").arg(m_table));
    q.addBindValue(e.version);
    q.addBindValue(e.name);
    q.addBindValue(e.checksum);
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    q.addBindValue(durationMs);
    if (!q.exec()) {
        m_error = q.lastError().text().trimmed();
        return false;
    }
    return true;
}

bool QiMigrator::unrecord(int version) {
    QSqlQuery q = m_conn.query();
    q.prepare(QStringLiteral("DELETE FROM %1 WHERE version = ?").arg(m_table));
    q.addBindValue(version);
    if (!q.exec()) {
        m_error = q.lastError().text().trimmed();
        return false;
    }
    return true;
}

void QiMigrator::syncUserVersion() {
    if (!usesUserVersion())
        return;
    QSqlQuery q = m_conn.query();
    int top = 0;
    if (q.exec(QStringLiteral("SELECT MAX(version) FROM %1").arg(m_table)) && q.next())
        top = q.value(0).toInt();
    // PRAGMA doesn't take a bound parameter; the value is our own int.
    m_conn.query().exec(QStringLiteral("PRAGMA user_version = %1").arg(top));
}

// One lock name per migrations table, so two apps sharing a server don't wait on each other.
static qint64 advisoryKey(const QString &table) {
    const QByteArray h = QCryptographicHash::hash(("qivot:" + table.toLower()).toUtf8(), QCryptographicHash::Sha256);
    return h.toHex().left(14).toLongLong(nullptr, 16);      // 56 bits: always a positive bigint
}

bool QiMigrator::lock() {
    // The connection keeps its last query, often a SELECT still on a row; on
    // SQLite that open cursor makes every DROP fail with "table is locked".
    QSqlQuery last = m_conn.lastQuery();
    if (last.isActive())
        last.finish();

    const QString d = driver();
    QSqlQuery q = m_conn.query();
    bool ok = true;
    if (d == QLatin1String("QPSQL")) {
        ok = q.exec(QStringLiteral("SELECT pg_advisory_lock(%1)").arg(advisoryKey(m_table)));
    } else if (d == QLatin1String("QMYSQL") || d == QLatin1String("QMARIADB")) {
        ok = q.exec(QStringLiteral("SELECT GET_LOCK('qivot:%1', 60)").arg(m_table)) && q.next() && q.value(0).toInt() == 1;
    } else if (m_conn.sql().database().driver()->dbmsType() == QSqlDriver::MSSqlServer) {
        ok = q.exec(QStringLiteral("SET NOCOUNT ON; DECLARE @r INT; "
                                   "EXEC @r = sp_getapplock @Resource = N'qivot:%1', @LockMode = 'Exclusive', "
                                   "@LockOwner = 'Session', @LockTimeout = 60000; SELECT @r").arg(m_table))
             && q.next() && q.value(0).toInt() >= 0;
    } else {
        return true;     // SQLite / DuckDB: the database serialises writers
    }
    if (!ok) {
        failed(QStringLiteral("could not take the migration lock: %1")
                   .arg(q.lastError().text().trimmed().isEmpty() ? QStringLiteral("timed out")
                                                                 : q.lastError().text().trimmed()));
        return false;
    }
    m_locked = true;
    return true;
}

void QiMigrator::unlock() {
    if (!m_locked)
        return;
    m_locked = false;
    const QString d = driver();
    QSqlQuery q = m_conn.query();
    if (d == QLatin1String("QPSQL"))
        q.exec(QStringLiteral("SELECT pg_advisory_unlock(%1)").arg(advisoryKey(m_table)));
    else if (d == QLatin1String("QMYSQL") || d == QLatin1String("QMARIADB"))
        q.exec(QStringLiteral("SELECT RELEASE_LOCK('qivot:%1')").arg(m_table));
    else
        q.exec(QStringLiteral("EXEC sp_releaseapplock @Resource = N'qivot:%1', @LockOwner = 'Session'").arg(m_table));
}

namespace {
struct LockGuard {
    std::function<void()> release;
    ~LockGuard() { release(); }
};
}

int QiMigrator::run(int upTo) {
    m_error.clear();
    if (!lock())
        return -1;
    LockGuard guard{ [this] { unlock(); } };

    if (!ensureTable())
        return -1;

    // Read after taking the lock: another instance may have just migrated.
    bool ok = true;
    const QMap<int, Row> rows = appliedRows(&ok);
    if (!ok)
        return failed(QStringLiteral("could not read %1").arg(m_table));

    const QVector<Migration> edited = changed();
    if (!edited.isEmpty())
        return failed(QStringLiteral("migration %1 (%2) was changed after it was applied; "
                                     "undo the edit, or accept it with acceptChecksums()")
                          .arg(edited.first().version).arg(edited.first().name));

    int applied = 0;
    for (const Entry &e : ordered()) {
        if (rows.contains(e.version))
            continue;                       // already applied
        if (upTo >= 0 && e.version > upTo)
            break;

        if (!begin())
            return failed(QStringLiteral("could not begin a transaction: %1")
                              .arg(m_conn.sql().database().lastError().text().trimmed()));

        QElapsedTimer timer;
        timer.start();
        if (!runStep(e, true)) {
            QString reason = m_error.isEmpty() ? m_conn.lastError().text().trimmed() : m_error;
            end(false);
            if (reason.isEmpty())
                reason = QStringLiteral("the migration step returned false");
            return failed(QStringLiteral("migration %1 (%2) failed: %3").arg(e.version).arg(e.name, reason));
        }
        if (!foreignKeysHold()) {
            const QString reason = m_error;
            end(false);
            return failed(QStringLiteral("migration %1 (%2) failed: %3").arg(e.version).arg(e.name, reason));
        }
        if (!record(e, int(timer.elapsed()))) {
            const QString reason = m_error;
            end(false);
            return failed(QStringLiteral("migration %1 (%2): could not record it: %3").arg(e.version).arg(e.name, reason));
        }
        syncUserVersion();                  // transactional in SQLite
        if (!end(true)) {
            return failed(QStringLiteral("migration %1 (%2): commit failed").arg(e.version).arg(e.name));
        }

        QiLog::write(QiLog::Sql, QiLog::Info,
                     QStringLiteral("migrated to v%1 (%2)").arg(e.version).arg(e.name));
        applied++;
    }
    return applied;
}

int QiMigrator::migrate() {
    return run(-1);
}

int QiMigrator::migrateTo(int version) {
    return run(version);
}

int QiMigrator::rollback(int version) {
    m_error.clear();
    if (!lock())
        return -1;
    LockGuard guard{ [this] { unlock(); } };

    if (!ensureTable())
        return -1;

    bool ok = true;
    const QMap<int, Row> rows = appliedRows(&ok);
    if (!ok)
        return failed(QStringLiteral("could not read %1").arg(m_table));

    QMap<int, Entry> known;
    for (const Entry &e : m_steps)
        known.insert(e.version, e);

    // Check them all first, so a missing down step doesn't leave a half-undone run.
    QVector<Entry> undo;
    for (auto it = rows.constEnd(); it != rows.constBegin();) {
        --it;
        if (it.key() <= version)
            break;
        if (!known.contains(it.key()))
            return failed(QStringLiteral("migration %1 (%2) is recorded but not registered, so it can't be undone")
                              .arg(it.key()).arg(it->name));
        const Entry &e = known[it.key()];
        if (!e.down && e.downSql.trimmed().isEmpty())
            return failed(QStringLiteral("migration %1 (%2) has no down step").arg(e.version).arg(e.name));
        undo << e;
    }

    int undone = 0;
    for (const Entry &e : undo) {
        if (!begin())
            return failed(QStringLiteral("could not begin a transaction: %1")
                              .arg(m_conn.sql().database().lastError().text().trimmed()));
        if (!runStep(e, false)) {
            QString reason = m_error.isEmpty() ? m_conn.lastError().text().trimmed() : m_error;
            end(false);
            if (reason.isEmpty())
                reason = QStringLiteral("the down step returned false");
            return failed(QStringLiteral("undoing migration %1 (%2) failed: %3").arg(e.version).arg(e.name, reason));
        }
        if (!foreignKeysHold()) {
            const QString reason = m_error;
            end(false);
            return failed(QStringLiteral("undoing migration %1 (%2) failed: %3").arg(e.version).arg(e.name, reason));
        }
        if (!unrecord(e.version)) {
            const QString reason = m_error;
            end(false);
            return failed(QStringLiteral("undoing migration %1 (%2): %3").arg(e.version).arg(e.name, reason));
        }
        syncUserVersion();
        if (!end(true)) {
            return failed(QStringLiteral("undoing migration %1 (%2): commit failed").arg(e.version).arg(e.name));
        }
        QiLog::write(QiLog::Sql, QiLog::Info,
                     QStringLiteral("rolled back v%1 (%2)").arg(e.version).arg(e.name));
        undone++;
    }
    return undone;
}

bool QiMigrator::acceptChecksums() {
    m_error.clear();
    if (!ensureTable())
        return false;
    for (const Migration &m : changed()) {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("UPDATE %1 SET checksum = ? WHERE version = ?").arg(m_table));
        q.addBindValue(m.checksum);
        q.addBindValue(m.version);
        if (!q.exec()) {
            failed(QStringLiteral("could not update migration %1: %2").arg(m.version).arg(q.lastError().text().trimmed()));
            return false;
        }
    }
    return true;
}

QString QiMigrator::lastError() const {
    return m_error;
}

void QiMigrator::setTable(const QString &table) {
    m_table = table;
}

QString QiMigrator::table() const {
    return m_table;
}

// --- SQL text ---------------------------------------------------------------

QString QiMigrator::checksum(const QString &sql) {
    if (sql.trimmed().isEmpty())
        return QString();
    QString text = sql;
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));      // the same file on Windows and elsewhere
    return QString::fromLatin1(QCryptographicHash::hash(text.trimmed().toUtf8(), QCryptographicHash::Sha256).toHex());
}

static bool isWordChar(QChar c) {
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

QStringList QiMigrator::splitStatements(const QString &sql, const QString &driver) {
    static const QRegularExpression noSplit(QStringLiteral("^\\s*--\\s*qivot:no-split\\b"),
                                            QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);
    if (noSplit.match(sql).hasMatch())
        return sql.trimmed().isEmpty() ? QStringList() : QStringList(sql.trimmed());

    const bool pg = driver == QLatin1String("QPSQL");
    const bool mysql = driver == QLatin1String("QMYSQL") || driver == QLatin1String("QMARIADB");
    const bool mssql = driver == QLatin1String("QODBC");

    QStringList out;
    QString cur;
    bool hasCode = false;      // more than comments and whitespace
    bool lineStart = true;     // only whitespace so far on this line
    // CREATE TRIGGER / PROCEDURE / FUNCTION bodies hold `;` of their own:
    // BEGIN … END (and CASE … END) nest, and only the outermost `;` ends them.
    QStringList lead;
    bool leadDone = false, block = false;
    int depth = 0;

    auto flush = [&]() {
        const QString s = cur.trimmed();
        if (hasCode && !s.isEmpty())
            out << s;
        cur.clear();
        hasCode = false;
        lead.clear();
        leadDone = block = false;
        depth = 0;
    };

    const int n = sql.size();
    int i = 0;
    while (i < n) {
        const QChar c = sql.at(i);

        // SQL Server batch separator: a line saying only GO.
        if (mssql && lineStart && (c == QLatin1Char('G') || c == QLatin1Char('g'))) {
            int end = sql.indexOf(QLatin1Char('\n'), i);
            if (end < 0) end = n;
            if (sql.mid(i, end - i).trimmed().compare(QLatin1String("GO"), Qt::CaseInsensitive) == 0) {
                flush();
                i = end;
                continue;
            }
        }

        if (c == QLatin1Char('\n')) { cur += c; lineStart = true; ++i; continue; }
        if (c.isSpace())            { cur += c; ++i; continue; }
        lineStart = false;

        // -- line comment
        if (c == QLatin1Char('-') && i + 1 < n && sql.at(i + 1) == QLatin1Char('-')) {
            int end = sql.indexOf(QLatin1Char('\n'), i);
            if (end < 0) end = n;
            cur += sql.mid(i, end - i);
            i = end;
            continue;
        }
        // /* block comment */
        if (c == QLatin1Char('/') && i + 1 < n && sql.at(i + 1) == QLatin1Char('*')) {
            int end = sql.indexOf(QLatin1String("*/"), i + 2);
            end = end < 0 ? n : end + 2;
            cur += sql.mid(i, end - i);
            i = end;
            continue;
        }

        hasCode = true;

        // 'strings', "identifiers", `identifiers`, [identifiers]
        QChar close;
        if (c == QLatin1Char('\'') || c == QLatin1Char('"') || c == QLatin1Char('`'))
            close = c;
        else if (mssql && c == QLatin1Char('['))
            close = QLatin1Char(']');
        if (!close.isNull()) {
            int j = i + 1;
            while (j < n) {
                if (mysql && close == QLatin1Char('\'') && sql.at(j) == QLatin1Char('\\')) { j += 2; continue; }
                if (sql.at(j) == close) {
                    if (j + 1 < n && sql.at(j + 1) == close) { j += 2; continue; }   // '' doubled
                    break;
                }
                ++j;
            }
            j = qMin(j + 1, n);
            cur += sql.mid(i, j - i);
            i = j;
            leadDone = true;
            continue;
        }

        // PostgreSQL $$ … $$ / $tag$ … $tag$
        if (pg && c == QLatin1Char('$') && (i == 0 || !isWordChar(sql.at(i - 1)))) {
            int j = i + 1;
            while (j < n && (isWordChar(sql.at(j)) && !(j == i + 1 && sql.at(j).isDigit())))
                ++j;
            if (j < n && sql.at(j) == QLatin1Char('$')) {
                const QString tag = sql.mid(i, j - i + 1);
                int end = sql.indexOf(tag, j + 1);
                end = end < 0 ? n : end + tag.size();
                cur += sql.mid(i, end - i);
                i = end;
                continue;
            }
        }

        if (isWordChar(c)) {
            int j = i;
            while (j < n && isWordChar(sql.at(j)))
                ++j;
            const QString word = sql.mid(i, j - i).toUpper();
            cur += sql.mid(i, j - i);
            i = j;

            if (!leadDone) {
                lead << word;
                if (lead.size() == 1 && word != QLatin1String("CREATE") && word != QLatin1String("ALTER"))
                    leadDone = true;
                else if (word == QLatin1String("TRIGGER") || word == QLatin1String("PROCEDURE")
                         || word == QLatin1String("PROC") || word == QLatin1String("FUNCTION")
                         || word == QLatin1String("EVENT"))
                    { block = true; leadDone = true; }
                else if (lead.size() >= 6 || word == QLatin1String("TABLE") || word == QLatin1String("INDEX")
                         || word == QLatin1String("VIEW"))
                    leadDone = true;
            } else if (block) {
                if (word == QLatin1String("BEGIN") || word == QLatin1String("CASE")) {
                    ++depth;
                } else if (word == QLatin1String("END")) {
                    // END IF / END LOOP / END WHILE / END REPEAT close what never opened a level.
                    int k = i;
                    while (k < n && sql.at(k).isSpace()) ++k;
                    int l = k;
                    while (l < n && isWordChar(sql.at(l))) ++l;
                    const QString next = sql.mid(k, l - k).toUpper();
                    if (next != QLatin1String("IF") && next != QLatin1String("LOOP")
                        && next != QLatin1String("WHILE") && next != QLatin1String("REPEAT"))
                        depth = qMax(0, depth - 1);
                }
            }
            continue;
        }

        if (c == QLatin1Char('('))
            leadDone = true;

        if (c == QLatin1Char(';')) {
            // On SQL Server a procedure runs to the end of its batch (GO).
            if (block && (mssql || depth > 0)) {
                cur += c;
            } else {
                flush();
            }
            ++i;
            continue;
        }

        cur += c;
        ++i;
    }
    flush();
    return out;
}
