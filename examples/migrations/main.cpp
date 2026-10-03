/** Versioned schema migrations with QiMigrator.

    QiMigrator records what ran in a qivot_migrations table. You register
    migrations by version (as code, SQL text or .sql files); migrate() runs the
    pending ones in order, each in its own transaction. Re-running is a no-op,
    so it's safe to call on every startup.

    Part 1 migrates a fresh database up through several code migrations, proves
    a second run is idempotent, adds one more, and shows a failure rolling back.
    Part 2 runs a folder of .sql files, catches an edited one by its checksum,
    and rolls back with the down steps.
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

    // --- Part 2: migrations as .sql files -----------------------------------
    // Ship them as a folder (here a Qt resource, see migrations.qrc):
    // 0001_create_tags.sql, 0002_seed_tags.up.sql + .down.sql, …
    QiMigrator files(conn);
    files.setTable("tag_migrations");          // a second history beside the first
    qInfo().noquote() << "\nFrom :/migrations:" << files.addDirectory(":/migrations") << "files";
    for (const QiMigrator::Migration &m : files.pending())
        qInfo().noquote() << QString("  pending v%1 %2%3").arg(m.version).arg(m.name,
                                     m.reversible ? QString(" (has a down step)") : QString());

    qInfo().noquote() << "migrate() applied" << files.migrate() << "-> version" << files.currentVersion();
    q.exec("INSERT INTO note_tag (note_id, tag_id) VALUES (1, 1)");
    q.exec("SELECT name, uses FROM tag ORDER BY id");
    QStringList tags;
    while (q.next()) tags << QString("%1=%2").arg(q.value(0).toString()).arg(q.value(1).toInt());
    q.finish();
    qInfo().noquote() << "  tag uses:" << tags.join(", ");

    for (const QiMigrator::Migration &m : files.status())
        qInfo().noquote() << QString("  v%1 %2  applied %3 in %4 ms")
                                 .arg(m.version).arg(m.name)
                                 .arg(m.appliedAt.toString(Qt::ISODate)).arg(m.durationMs);

    // Editing a file that already ran is caught by its checksum.
    QiMigrator edited(conn);
    edited.setTable("tag_migrations");
    edited.addDirectory(":/migrations");
    edited.addSql(1, "create tags", "CREATE TABLE tag (id INTEGER PRIMARY KEY, name TEXT)");
    qInfo().noquote() << "\nEdited v1: migrate() returns" << edited.migrate();
    qInfo().noquote() << "  error:" << edited.lastError();

    // Down steps undo, newest first.
    qInfo().noquote() << "\nrollback(1) undid" << files.rollback(1) << "-> version" << files.currentVersion();
    qInfo().noquote() << "  tag columns:" << columns(conn, "tag").join(", ");

    return 0;
}
