/** Live integration test for the SQLite, MySQL, PostgreSQL, and SQL Server backends.

    Runs a full round-trip against a real server and verifies the cross-dialect
    paths that the SQL-generation unit tests can't:
      - auto-increment id returned after insert (Postgres lastval, SQL Server
        SCOPE_IDENTITY, vs SQLite/MySQL lastInsertId)
      - long text stored without truncation (MySQL TEXT vs VARCHAR(255))
      - double / bool / JSON / QDate / QDateTime round-trip
      - batch insert (QiList::save) with a per-row id assigned to every model
      - transactions: rollback discards, commit persists
      - a STRING primary key model (no auto id) save + load + upsert-translation
      - the migration path (createTables twice is a safe no-op via portable columnNames)
      - upsert on a unique key updates in place instead of duplicating

    Usage:  ./integration <sqlite|mysql|postgres|sqlserver>
    The `sqlite` backend uses an in-memory database and needs no server, so the whole
    suite can be validated locally; the others exercise the real dialects in CI.
    (Oracle isn't wired in here yet — QOCI isn't reliably available in CI; see
    src/qioraclestatement.h, which is covered by the unit tests instead.)

    If the driver plugin is missing or no server answers, it SKIPS (exit 0) rather than
    failing — unless QIVOT_REQUIRE_DB is set (as CI does), when it hard-fails instead.
    See tests/integration/README.md.
 */
#include "intmodel.h"

#include <qimysqlstatement.h>
#include <qipgstatement.h>
#include <qimssqlstatement.h>

// DuckDB has no Qt-bundled driver; build with `qmake CONFIG+=duckdb DUCKDB_DIR=<path>`
// to compile in the QDUCKDB driver (drivers/duckdb) and enable the `duckdb` backend.
#ifdef QIVOT_DUCKDB
#include <QSqlDriver>
#include "duckdbdriver.h"
#endif

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QJsonObject>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QDebug>
#include <cstdio>
#include <cmath>

static int failures = 0;
static void check(bool ok, const QString &msg) {
    qInfo().noquote() << (ok ? "  PASS  " : "  FAIL  ") + msg;
    if (!ok) failures++;
}

/// The full round-trip suite, shared by every backend.
static void runSuite(QiConnection &conn) {
    conn.dropTables();
    check(conn.createTables(), "createTables");

    // Migration path: a second createTables must be a clean no-op. That only holds if
    // columnNames() correctly reports the existing columns (the portable QSqlDatabase::
    // record path) — a broken reader would try to re-add every column and error out.
    check(conn.createTables(), "createTables again is a no-op (migration reads columns)");

    // --- single insert: long text, double, bool, JSON, and date/time round-trip ---
    const QString longName = QString("x").repeated(600);
    const QDate     day = QDate(2026, 7, 29);
    const QDateTime ts  = QDateTime(QDate(2026, 7, 29), QTime(10, 30, 15));
    IntThing t;
    t.name  = longName;
    t.code  = "CODE-1";
    t.score = 3.14159;
    t.active = true;
    QJsonObject meta; meta.insert("k", "v"); meta.insert("n", 42);
    t.meta = meta;
    t.day = day;
    t.ts  = ts;
    const bool inserted = t.save();
    check(inserted, "insert row");
    if (!inserted)
        qWarning().noquote() << "      -> db error:" << t.lastError().text();
    const int id = t.id.get().toInt();
    check(id > 0, QString("auto-increment id assigned = %1").arg(id));

    IntThing loaded;
    check(loaded.load(QiWhere("code = ", "CODE-1")), "load by unique code");
    check(QString(loaded.name).length() == 600, QString("long text preserved (len = %1)").arg(QString(loaded.name).length()));
    check(std::fabs(double(loaded.score) - 3.14159) < 1e-6, "double round-trip");
    check(bool(loaded.active) == true, "bool round-trip");
    const QJsonObject lm = loaded.meta;
    check(lm.value("n").toInt() == 42 && lm.value("k").toString() == "v", "JSON round-trip");
    check(QDate(loaded.day) == day, "QDate round-trip");
    check(QDateTime(loaded.ts).toString("yyyy-MM-dd HH:mm:ss") == ts.toString("yyyy-MM-dd HH:mm:ss"), "QDateTime round-trip");
    check(QiQuery<IntThing>().count() == 1, "count == 1");

    // --- batch insert: one prepared statement, an id assigned back to every row ---
    QiList<IntThing> batch;
    for (int i = 1; i <= 3; i++) {
        IntThing *b = new IntThing();
        b->name  = QString("bulk-%1").arg(i);
        b->code  = QString("BULK-%1").arg(i);
        b->score = i + 0.5;
        b->active = (i % 2 == 0);
        b->meta  = QJsonObject();
        b->day   = day;
        b->ts    = ts;
        batch.append(b);
    }
    check(batch.save(), "batch insert (QiList::save)");
    bool allIds = true;
    for (int i = 0; i < batch.size(); i++)
        if (batch.at(i)->id.get().toInt() <= 0) allIds = false;
    check(allIds, "batch: an id was assigned to every row");
    check(batch.at(0)->id.get().toInt() != batch.at(1)->id.get().toInt(), "batch: ids are distinct");
    check(QiQuery<IntThing>().count() == 4, "count == 4 after batch");

    // --- transaction: rollback discards, commit persists ---
    {
        QiTransaction tx;
        IntThing r; r.name = "temp"; r.code = "ROLLBACK-1"; r.score = 0; r.active = false;
        r.meta = QJsonObject(); r.day = day; r.ts = ts;
        r.save();
        // tx goes out of scope without commit -> rolled back
    }
    check(QiQuery<IntThing>().filter(QiWhere("code = ", "ROLLBACK-1")).count() == 0, "transaction rollback discarded the row");
    {
        QiTransaction tx;
        IntThing r; r.name = "kept"; r.code = "COMMIT-1"; r.score = 0; r.active = true;
        r.meta = QJsonObject(); r.day = day; r.ts = ts;
        r.save();
        check(tx.commit(), "transaction commit");
    }
    check(QiQuery<IntThing>().filter(QiWhere("code = ", "COMMIT-1")).count() == 1, "committed row persisted");

    // --- string primary key (no auto id) : save, load, upsert-in-place ---
    IntTag tag; tag.slug = "hello"; tag.label = "Hello World";
    check(tag.save(), "string-PK save");
    IntTag tagLoaded;
    check(tagLoaded.load(QiWhere("slug = ", "hello")), "string-PK load");
    check(QString(tagLoaded.label) == "Hello World", "string-PK value round-trip");
    IntTag tagUpd; tagUpd.slug = "hello"; tagUpd.label = "Changed";
    check(tagUpd.save(), "string-PK save again (replace on key)");
    check(QiQuery<IntTag>().count() == 1, "string-PK: no duplicate row on re-save");
    IntTag tagAfter; tagAfter.load(QiWhere("slug = ", "hello"));
    check(QString(tagAfter.label) == "Changed", "string-PK: row updated in place");

    // --- upsert on a unique (non-primary) key updates in place ---
    IntThing u;
    u.code = "CODE-1"; u.name = "updated"; u.score = 9.99; u.active = false;
    u.meta = QJsonObject(); u.day = day; u.ts = ts;
    check(u.upsert(QStringList() << "code"), "upsert on unique code");
    check(QiQuery<IntThing>().filter(QiWhere("code = ", "CODE-1")).count() == 1, "upsert did not duplicate");
    IntThing after;
    after.load(QiWhere("code = ", "CODE-1"));
    check(QString(after.name) == "updated", "upsert updated the row in place");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QString backend = argc > 1 ? QString::fromLocal8Bit(argv[1]).toLower() : QString();

#ifdef QIVOT_DUCKDB
    QSqlDatabase::registerSqlDriver("QDUCKDB", new QSqlDriverCreator<DuckDbDriver>());
#endif

    // With QIVOT_REQUIRE_DB set (as CI does), a missing driver or failed connection
    // is a hard failure; otherwise it's a graceful skip so the test is safe anywhere.
    const bool requireDb = qEnvironmentVariableIsSet("QIVOT_REQUIRE_DB");
    auto skipOrFail = [&](const QString &why) -> int {
        if (requireDb) { qCritical().noquote() << "FAIL:" << why; return 1; }
        qWarning().noquote() << "SKIP:" << why;
        return 0;
    };

    if (backend == "print-mysql" || backend == "print-postgres" || backend == "print-sqlserver") {
        // Print the generated SQL (no DB / driver needed) so it can be validated
        // directly against a real server via psql / mariadb / sqlcmd.
        QiSqlStatement *s = (backend == "print-postgres")   ? static_cast<QiSqlStatement*>(new QiPgStatement())
                          : (backend == "print-sqlserver")  ? static_cast<QiSqlStatement*>(new QiMsSqlStatement())
                          :                                   static_cast<QiSqlStatement*>(new QiMysqlStatement());
        QiModelMetaInfo *info = qiMetaInfo<IntThing>();
        const QStringList cols = QStringList() << "name" << "code" << "score" << "active" << "meta" << "day" << "ts";
        std::printf("CREATE: %s\n", qPrintable(s->createTableIfNotExists(info)));
        std::printf("INSERT: %s\n", qPrintable(s->insertInto(info, cols)));
        std::printf("UPSERT: %s\n", qPrintable(s->upsertInto(info, cols, QStringList() << "code")));
        return 0;
    }

    QString driver, defUser, defPass;
    int defPort = 0;
    bool isSqlite = false;
    bool isOdbc = false;
    if (backend == "sqlite")                               { driver = "QSQLITE"; isSqlite = true; }
    else if (backend == "mysql" || backend == "mariadb")   { driver = "QMYSQL"; defPort = 3306; defUser = "root";     defPass = "qivot"; }
    else if (backend == "postgres" || backend == "pg")     { driver = "QPSQL";  defPort = 5432; defUser = "postgres"; defPass = "qivot"; }
    else if (backend == "sqlserver" || backend == "mssql") { driver = "QODBC";  defPort = 1433; defUser = "sa"; defPass = "Qivot_Test1"; isOdbc = true; }
    else if (backend == "duckdb") {
#ifdef QIVOT_DUCKDB
        driver = "QDUCKDB"; isSqlite = true;   // embedded + in-memory, like SQLite
#else
        return skipOrFail("built without DuckDB — rebuild with: qmake CONFIG+=duckdb DUCKDB_DIR=<path>");
#endif
    }
    else { qWarning().noquote() << "usage: integration <sqlite|mysql|postgres|sqlserver|duckdb|print-mysql|print-postgres|print-sqlserver>"; return 2; }

    if (!QSqlDatabase::drivers().contains(driver))
        return skipOrFail(driver + " driver plugin not available");

    QSqlDatabase db = QSqlDatabase::addDatabase(driver);
    if (isSqlite) {
        db.setDatabaseName(":memory:");
    } else if (isOdbc) {
        // QODBC wants a full ODBC connection string via setDatabaseName(), not the
        // discrete setHostName()/setPort()/setUserName()/setPassword() sequence the
        // other network drivers use. QIVOT_ODBC_DRIVER lets a different installed
        // driver name be substituted (e.g. "ODBC Driver 17 for SQL Server").
        const QString host = qEnvironmentVariable("QIVOT_HOST", "127.0.0.1");
        const QString port = qEnvironmentVariable("QIVOT_PORT", QString::number(defPort));
        const QString name = qEnvironmentVariable("QIVOT_NAME", "qivot_test");
        const QString user = qEnvironmentVariable("QIVOT_USER", defUser);
        const QString pass = qEnvironmentVariable("QIVOT_PASS", defPass);
        const QString odbcDriver = qEnvironmentVariable("QIVOT_ODBC_DRIVER", "ODBC Driver 18 for SQL Server");
        db.setDatabaseName(QString("Driver={%1};Server=%2,%3;Database=%4;Uid=%5;Pwd=%6;"
                                    "TrustServerCertificate=yes;")
                               .arg(odbcDriver, host, port, name, user, pass));
    } else {
        db.setHostName(qEnvironmentVariable("QIVOT_HOST", "127.0.0.1"));
        db.setPort(qEnvironmentVariable("QIVOT_PORT", QString::number(defPort)).toInt());
        db.setDatabaseName(qEnvironmentVariable("QIVOT_NAME", "qivot_test"));
        db.setUserName(qEnvironmentVariable("QIVOT_USER", defUser));
        db.setPassword(qEnvironmentVariable("QIVOT_PASS", defPass));
    }
    if (!db.open())
        return skipOrFail(QString("cannot connect to %1 - %2").arg(backend, db.lastError().text()));

    qInfo().noquote() << QString("\n=== Integration: %1 (%2) ===").arg(backend, driver);

    QiConnection conn;
    if (!conn.open(db)) { qWarning() << "QiConnection::open failed"; return 1; }
    conn.addModel<IntThing>();
    conn.addModel<IntTag>();

    runSuite(conn);

    conn.close();
    qInfo().noquote() << (failures == 0 ? "ALL PASSED\n" : QString("%1 CHECK(S) FAILED\n").arg(failures));
    return failures == 0 ? 0 : 1;
}
