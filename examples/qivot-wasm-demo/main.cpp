/** Qivot Notes — a tiny live notes board, built to run in the browser via
    Qt for WebAssembly (and natively too).

    Single-threaded, no external SQLite, no filesystem — the database is `:memory:`,
    seeded at startup. Notes are exposed to QML through a live QiListModel, so
    adding/removing updates the view on its own.
 */
#include "note.h"
#include "notestore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QDateTime>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    // In-memory DB: fresh each launch, and the right choice for WebAssembly —
    // the browser sandbox has no persistent filesystem for a native-style .db file.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) return 1;

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Note>();
    if (!connection.createTables()) return 1;

    // A few notes to start with.
    const QStringList seeds = {
        "Welcome 👋 — this whole page is a Qt app compiled to WebAssembly.",
        "Every note here is a row in an in-browser SQLite database, via Qivot.",
        "Type below and hit Add — the list updates itself (a live QiListModel)." };
    const QStringList colors = { "#FDE68A", "#BBF7D0", "#BFDBFE" };
    for (int i = 0; i < seeds.size(); i++) {
        Note n; n.text = seeds.at(i); n.color = colors.at(i);
        n.createdAt = QDateTime::currentDateTime().toString("MMM d · h:mm ap");
        n.save();
    }

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        NoteStore s;
        qInfo() << "initial rows:" << s.notes()->rowCount();
        s.add("headless test note");
        QCoreApplication::processEvents();   // let the coalesced live-refresh run
        qInfo() << "after add rows:" << s.notes()->rowCount();
        return 0;
    }

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
