/** Nested transactions — a savepoint sandbox.

    Open nested "scopes" (the first is a BEGIN, deeper ones are SAVEPOINTs), add
    and remove items inside them, then roll a scope back to undo just its edits
    while outer scopes keep theirs — or commit to keep them.

    QIVOT_SELFTEST=1 drives a nested begin/rollback/commit sequence then quits.
 */
#include "models.h"
#include "sandboxstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();
    QiConnection conn;
    if (!conn.open(db)) return 1;
    conn.addModel<Item>();
    if (!conn.createTables()) return 1;

    { Item a; a.label = "Committed row"; a.save(); }   // one row to start

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        SandboxStore *store = root->findChild<SandboxStore *>();
        QTimer::singleShot(200, &app, [store] {
            if (!store) return;
            const int base = Item::objects().count();          // 1
            store->beginScope();                                // BEGIN
            store->addItem("outer A");
            store->beginScope();                                // SAVEPOINT (depth 2)
            store->addItem("inner B");
            qInfo() << "in nested scopes, count:" << Item::objects().count() << "depth:" << store->depth();
            store->rollbackScope();                             // undo inner -> B gone
            qInfo() << "after inner rollback, count:" << Item::objects().count() << "depth:" << store->depth();
            store->commitScope();                               // keep outer -> A kept
            qInfo() << "after outer commit, count:" << Item::objects().count()
                    << "(base was" << base << ")";
        });
        QTimer::singleShot(600, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
