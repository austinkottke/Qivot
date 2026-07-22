/** Modern QML integration for Qivot.

    - `Post` is a Q_GADGET model (see post.h) — its fields are Q_PROPERTYs.
    - `PostStore` is a QML-registered (QML_ELEMENT) controller exposing a
      QiListModel + invokable slots + a NOTIFY-backed `status` property.
    - main.qml creates the PostStore declaratively (no setContextProperty),
      binds a ListView to its model and a label to `status`, and calls its
      slots from the UI.

    Set QIVOT_SELFTEST=1 to auto-quit shortly after loading (used to verify the
    example headlessly with the offscreen platform).
 */

#include "post.h"
#include "poststore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    // --- Set up the database before QML loads ---
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("qmlmodel.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db))
        return 1;
    connection.addModel<Post>();
    if (!connection.dropTables() || !connection.createTables())
        return 1;

    QiFtsIndex<Post> fts("post_fts");
    fts << "title" << "author";
    connection.createFtsIndex(fts);

    // Seed a few rows
    Post p1; p1.remoteId = 1; p1.title = "hello world";  p1.author = "ada";   p1.save();
    Post p2; p2.remoteId = 2; p2.title = "sqlite rocks"; p2.author = "grace"; p2.save();
    Post p3; p3.remoteId = 3; p3.title = "qml + orm";    p3.author = "ada";   p3.save();

    // --- Load the QML UI (PostStore is created inside main.qml) ---
    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [](QObject *obj, const QUrl &) {
                         if (!obj) {
                             qCritical() << "Failed to load QML";
                             QCoreApplication::exit(1);
                         }
                     }, Qt::QueuedConnection);
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty())
        return 1;

    // Headless self-test: drive add + remove through the store, then quit.
    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        PostStore *store = root->findChild<PostStore *>();
        QTimer::singleShot(600, &app, [store]() {
            if (store) {
                store->add("added from the UI", "you");   // exercise insert
                store->remove(2);                          // exercise delete
            }
        });
        QTimer::singleShot(1200, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
