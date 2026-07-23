/** Versioned schema migrations with QiMigrator.

    QiMigrator tracks the schema version in SQLite's PRAGMA user_version. You
    register migrations by version; migrate() runs the pending ones in order,
    each in its own transaction, and bumps user_version. Re-running is a no-op,
    so it's safe to call on every startup.

    This program migrates a fresh database up through several versions, proves
    that a second run is idempotent, then adds one more migration and applies it.
 */
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QSqlQuery>
#include <qivot.h>

static QStringList columns(QiConnection &conn, const QString &table) {
    QSqlQuery q = conn.query();
    q.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table));
    QStringList cols;
    while (q.next()) cols << q.value(1).toString();
    return cols;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();
    QiConnection conn;
    if (!conn.open(db)) return 1;

    // Register the schema history. Order of add() doesn't matter.
    QiMigrator migrator(conn);

    migrator.add(1, "create note table", [](QiConnection &c) {
        return c.query().exec(
            "CREATE TABLE note (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL)");
    });
    migrator.add(2, "add body column", [](QiConnection &c) {
        return c.query().exec("ALTER TABLE note ADD COLUMN body TEXT");
    });
    migrator.add(3, "add pinned flag + index", [](QiConnection &c) {
        return c.query().exec("ALTER TABLE note ADD COLUMN pinned INTEGER NOT NULL DEFAULT 0")
            && c.query().exec("CREATE INDEX idx_note_pinned ON note(pinned)");
    });

    qInfo().noquote() << "Fresh database.";
    qInfo().noquote() << "  version:" << migrator.currentVersion()
                      << " target:" << migrator.targetVersion();

    int applied = migrator.migrate();
    qInfo().noquote() << "\nmigrate() applied" << applied << "migrations"
                      << "-> version" << migrator.currentVersion();
    qInfo().noquote() << "  note columns:" << columns(conn, "note").join(", ");

    // Running again does nothing — already up to date.
    qInfo().noquote() << "\nmigrate() again applied" << migrator.migrate()
                      << "(idempotent)";

    // Ship a new version later: register it and migrate() again.
    migrator.add(4, "backfill a welcome note", [](QiConnection &c) {
        QSqlQuery q = c.query();
        q.prepare("INSERT INTO note (title, body, pinned) VALUES (?, ?, 1)");
        q.addBindValue("Welcome");
        q.addBindValue("Your first note.");
        return q.exec();
    });

    qInfo().noquote() << "\nAdded v4; migrate() applied" << migrator.migrate()
                      << "-> version" << migrator.currentVersion();

    QSqlQuery q = conn.query();
    q.exec("SELECT count(*) FROM note");
    q.next();
    qInfo().noquote() << "  note rows:" << q.value(0).toInt();

    // A failing migration rolls back and leaves the version untouched.
    migrator.add(5, "intentionally broken", [](QiConnection &c) {
        return c.query().exec("ALTER TABLE nonexistent ADD COLUMN x TEXT");
    });
    qInfo().noquote() << "\nBroken v5: migrate() returns" << migrator.migrate()
                      << "(-1 = failed)";
    qInfo().noquote() << "  version still:" << migrator.currentVersion();
    qInfo().noquote() << "  error:" << migrator.lastError();

    return 0;
}
