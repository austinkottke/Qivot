#ifndef MIGRATIONSUITE_H
#define MIGRATIONSUITE_H

/** QiMigrator against a live database.

    The same SQL migrations on every backend: they're applied and recorded,
    re-running is a no-op, a failing one is rolled back (except on MySQL,
    which commits DDL as it goes — the check says so), down steps undo them,
    and an edited migration that already ran is refused. migrate() and
    rollback() take the backend's lock (advisory lock, GET_LOCK,
    sp_getapplock), so this also proves those statements run.

    Every object is prefixed `qm_` and migrations are recorded in
    `qm_migrations`, so it can't collide with the other suites.
 */
#include <functional>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <qimigrator.h>
#include <qisql.h>

namespace MigrationSuite {

using Check = std::function<void(bool, const QString &)>;

static void run(QiConnection &conn, const QString &backend, const Check &check)
{
    qInfo().noquote() << "--- migrations ---";
    QSqlDatabase db = conn.sql().database();
    const bool mysql = backend == "mysql";
    const bool sqlite = backend == "sqlite";

    for (const char *t : { "qm_extra", "qm_note", "qm_migrations" })
        conn.query().exec(QString("DROP TABLE IF EXISTS %1").arg(t));

    const QString dropIndex = (mysql || backend == "sqlserver")
                                  ? QString("DROP INDEX qm_note_title ON qm_note")
                                  : QString("DROP INDEX qm_note_title");
    auto build = [&](QiMigrator &m, const QString &first) {
        m.setTable("qm_migrations");
        m.addSql(1, "create note", first, "DROP TABLE qm_note");
        m.addSql(2, "add body", "ALTER TABLE qm_note ADD body VARCHAR(200)",
                                "ALTER TABLE qm_note DROP COLUMN body");
        m.addSql(3, "title index", "CREATE INDEX qm_note_title ON qm_note (title)", dropIndex);
    };
    const QString create = "CREATE TABLE qm_note (id INTEGER NOT NULL PRIMARY KEY, title VARCHAR(80));\n"
                           "INSERT INTO qm_note (id, title) VALUES (1, 'it''s; fine');";

    QiMigrator m(conn);
    build(m, create);
    check(m.pending().size() == 3, "migrations: three pending on a fresh database");
    const int applied = m.migrate();
    check(applied == 3, QString("migrations: migrate() applied %1 (%2)").arg(applied).arg(m.lastError()));
    check(m.currentVersion() == 3, "migrations: current version is 3");
    check(m.pending().isEmpty(), "migrations: nothing pending");
    check(db.record("qm_note").contains("body"), "migrations: the body column exists");
    check(m.migrate() == 0, "migrations: a second migrate() is a no-op");

    QSqlQuery q = conn.query();
    check(q.exec("SELECT title FROM qm_note WHERE id = 1") && q.next() && q.value(0).toString() == "it's; fine",
          "migrations: a multi-statement migration ran every statement");
    check(q.exec("SELECT COUNT(*) FROM qm_migrations") && q.next() && q.value(0).toInt() == 3,
          "migrations: three rows in qm_migrations");
    q.finish();         // an open SQLite cursor would lock the tables the down steps drop
    const QVector<QiMigrator::Migration> st = m.status();
    check(st.size() == 3 && st[2].applied && st[2].appliedAt.isValid() && st[2].checksum == st[2].appliedChecksum,
          "migrations: status() reads back the recorded rows");

    // A migration that fails on its second statement.
    m.addSql(4, "half", "CREATE TABLE qm_extra (id INTEGER);\nALTER TABLE qm_missing ADD x INTEGER;");
    check(m.migrate() == -1 && m.lastError().contains("migration 4"),
          QString("migrations: a failing migration is reported (%1)").arg(m.lastError()));
    check(m.currentVersion() == 3, "migrations: and not recorded");
    const bool extra = !db.record("qm_extra").isEmpty();
    if (mysql)
        check(extra, "migrations: MySQL kept the DDL before the failure (it commits DDL at once)");
    else
        check(!extra, "migrations: its first statement was rolled back too");
    conn.query().exec("DROP TABLE IF EXISTS qm_extra");

    // Down steps.
    QiMigrator back(conn);
    build(back, create);
    const int undone = back.rollback(1);
    check(undone == 2, QString("migrations: rollback(1) undid %1 (%2)").arg(undone).arg(back.lastError()));
    check(back.currentVersion() == 1, "migrations: back at version 1");
    check(!db.record("qm_note").contains("body"), "migrations: the body column is gone");
    check(back.migrate() == 2, "migrations: and forward again");

    // An applied migration edited afterwards.
    QiMigrator edited(conn);
    build(edited, "CREATE TABLE qm_note (id INTEGER NOT NULL PRIMARY KEY, title VARCHAR(120));");
    check(edited.changed().size() == 1 && edited.changed().first().version == 1,
          "migrations: the edited migration is reported as changed");
    edited.addSql(4, "more", "CREATE TABLE qm_extra (id INTEGER)");
    check(edited.migrate() == -1 && db.record("qm_extra").isEmpty(),
          "migrations: migrate() refuses to run past it");
    check(edited.acceptChecksums() && edited.changed().isEmpty(), "migrations: acceptChecksums() records the edit");
    check(edited.migrate() == 1, "migrations: then the rest runs");

    if (sqlite) {
        check(q.exec("PRAGMA user_version") && q.next() && q.value(0).toInt() == 0,
              "migrations: a table of its own leaves SQLite's user_version alone");
        q.finish();
    }

    for (const char *t : { "qm_extra", "qm_note", "qm_migrations" })
        conn.query().exec(QString("DROP TABLE IF EXISTS %1").arg(t));
}

} // namespace MigrationSuite

#endif // MIGRATIONSUITE_H
