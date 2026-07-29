/** Live integration test for the MySQL and PostgreSQL backends.

    Runs a full round-trip (create table, insert, load, upsert) against a real
    server and verifies the cross-dialect paths that the SQL-generation unit tests
    can't: the returned auto-increment id, JSON/JSONB storage, long-text (no VARCHAR
    truncation), and upsert on a unique key.

    Usage:  ./integration <mysql|postgres>
    If the driver plugin is missing or no server answers, it SKIPS (exit 0) rather
    than failing — so it's safe to run anywhere. See tests/integration/README.md.
 */
#include "intmodel.h"

#include <qimysqlstatement.h>
#include <qipgstatement.h>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QJsonObject>
#include <QDebug>
#include <cstdio>
#include <cmath>

static int failures = 0;
static void check(bool ok, const QString &msg) {
    qInfo().noquote() << (ok ? "  PASS  " : "  FAIL  ") + msg;
    if (!ok) failures++;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QString backend = argc > 1 ? QString::fromLocal8Bit(argv[1]).toLower() : QString();

    QString driver, user, pass;
    int port = 0;
    if (backend == "mysql")                              { driver = "QMYSQL"; port = 3307; user = "root";     pass = "qivot"; }
    else if (backend == "postgres" || backend == "pg")   { driver = "QPSQL";  port = 5432; user = "postgres"; pass = "qivot"; }
    else if (backend == "print-mysql" || backend == "print-postgres") {
        // Print the generated SQL (no DB / driver needed) so it can be validated
        // directly against a real server via psql / mariadb.
        QiSqlStatement *s = (backend == "print-postgres")
                              ? static_cast<QiSqlStatement*>(new QiPgStatement())
                              : static_cast<QiSqlStatement*>(new QiMysqlStatement());
        QiModelMetaInfo *info = qiMetaInfo<IntThing>();
        const QStringList cols = QStringList() << "name" << "code" << "score" << "active" << "meta";
        std::printf("%s\n", qPrintable(s->createTableIfNotExists(info)));
        return 0;
    }
    else { qWarning().noquote() << "usage: integration <mysql|postgres|print-mysql|print-postgres>"; return 2; }

    if (!QSqlDatabase::drivers().contains(driver)) {
        qWarning().noquote() << "SKIP:" << driver << "driver plugin not available";
        return 0;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(driver);
    db.setHostName("127.0.0.1");
    db.setPort(port);
    db.setDatabaseName("qivot_test");
    db.setUserName(user);
    db.setPassword(pass);
    if (!db.open()) {
        qWarning().noquote() << "SKIP: cannot connect to" << backend << "-" << db.lastError().text();
        return 0;
    }

    qInfo().noquote() << QString("\n=== Integration: %1 (%2) ===").arg(backend, driver);

    QiConnection conn;
    if (!conn.open(db)) { qWarning() << "QiConnection::open failed"; return 1; }
    conn.addModel<IntThing>();
    conn.dropTables();
    check(conn.createTables(), "createTables");

    // Insert — a >255 char name proves MySQL uses TEXT (no truncation), and the
    // returned id proves Postgres RETURNING (and MySQL/SQLite lastInsertId).
    const QString longName = QString("x").repeated(600);
    IntThing t;
    t.name  = longName;
    t.code  = "CODE-1";
    t.score = 3.14159;
    t.active = true;
    QJsonObject meta; meta.insert("k", "v"); meta.insert("n", 42);
    t.meta = meta;
    check(t.save(), "insert row");
    const int id = t.id.get().toInt();
    check(id > 0, QString("auto-increment id assigned = %1").arg(id));

    // Load back, verify every column type round-trips.
    IntThing loaded;
    check(loaded.load(QiWhere("code = ", "CODE-1")), "load by unique code");
    check(QString(loaded.name).length() == 600, QString("long text preserved (len = %1)").arg(QString(loaded.name).length()));
    check(std::fabs(double(loaded.score) - 3.14159) < 1e-6, "double round-trip");
    check(bool(loaded.active) == true, "bool round-trip");
    const QJsonObject lm = loaded.meta;
    check(lm.value("n").toInt() == 42 && lm.value("k").toString() == "v", "JSON round-trip");

    check(QiQuery<IntThing>().count() == 1, "count == 1");

    // Upsert on the unique key -> UPDATE in place (MySQL ON DUPLICATE KEY / Postgres ON CONFLICT).
    IntThing u;
    u.code = "CODE-1";
    u.name = "updated";
    u.score = 9.99;
    u.active = false;
    check(u.upsert(QStringList() << "code"), "upsert on code");
    check(QiQuery<IntThing>().count() == 1, "upsert did not duplicate (still 1 row)");
    IntThing after;
    after.load(QiWhere("code = ", "CODE-1"));
    check(QString(after.name) == "updated", "upsert updated the row in place");

    conn.close();
    qInfo().noquote() << (failures == 0 ? "ALL PASSED\n" : QString("%1 CHECK(S) FAILED\n").arg(failures));
    return failures == 0 ? 0 : 1;
}
