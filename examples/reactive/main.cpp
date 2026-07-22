/** Reactive Qivot — a to-do list whose view updates itself.

    Nothing in the UI calls "reload". Every mutation just save()/remove()s a Task;
    the ListView is bound to a *live* QiListModel (see TaskStore) that refreshes
    automatically. Tick "Auto-add" and watch rows appear on their own — proof the
    view reacts to database changes it didn't initiate.

    QIVOT_SELFTEST=1 drives a few mutations then quits (for headless checks).
 */
#include "task.h"
#include "taskstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("reactive.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Task>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    Task a; a.title = "Try Reactive Qivot";      a.done = 0; a.save();
    Task b; b.title = "Watch it update itself";  b.done = 1; b.save();

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        TaskStore *store = root->findChild<TaskStore *>();
        QTimer::singleShot(400,  &app, [store]{ if (store) store->add("from selftest"); });
        QTimer::singleShot(800,  &app, [store]{ if (store) { store->toggle(1); store->remove(2); } });
        QTimer::singleShot(1400, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
