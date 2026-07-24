/** Fluxo — a flow-field particle recorder that showcases Qivot's write path.

    A GPU-smooth particle system drifts across the canvas leaving glowing trails.
    Every frame, the position of every particle is batch-written to SQLite in one
    transaction (WAL) — thousands of row-writes per second, sustained. Drag the
    timeline and any past frame is read straight back out of the database and
    replayed: the picture on screen is reconstructed from rows.

    Showcases: QiList batch save (prepared multi-row INSERT), WAL journal mode,
    an index for instant history reads, retention pruning, and reading typed
    models back for replay.

    QIVOT_SELFTEST=1 runs headless for ~1.5s then prints the write stats and quits
    (use with QT_QPA_PLATFORM=offscreen).
 */
#include "sample.h"
#include "fluxview.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSqlDatabase>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("fluxo.db");
    if (!db.open()) return 1;

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.setJournalMode("WAL");                       // readers never block the writer
    connection.query().exec("PRAGMA synchronous = NORMAL"); // fast, still safe under WAL
    connection.addModel<Sample>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    // One index on the frame column turns every "replay this frame" read into an
    // instant index lookup instead of a full-table scan.
    QiIndex<Sample> byFrame("sample_by_frame");
    byFrame << "frame";
    (void) connection.createIndex(byFrame);

    // QIVOT_ART=<path>: render the pure particle canvas headlessly to a PNG and
    // exit (no window). Handy for generating the example art.
    if (qEnvironmentVariableIsSet("QIVOT_ART")) {
        FluxView v;
        v.headlessRender(1200, 760, 520, qEnvironmentVariable("QIVOT_ART"));
        return 0;
    }

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QTimer::singleShot(1500, &app, [&] {
            FluxView *v = engine.rootObjects().first()->findChild<FluxView *>();
            if (v)
                qInfo().noquote() << "SELFTEST  rows_written=" << v->rowsWritten()
                                  << " live_rows=" << v->liveRows()
                                  << " frames=" << v->headFrame()
                                  << " db_MB=" << QString::number(v->dbSizeMB(), 'f', 2);
            app.quit();
        });
    }
    return app.exec();
}
