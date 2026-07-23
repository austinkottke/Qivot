/** Prefetch vs N+1 — load a one-to-many relation efficiently.

    Two buttons load the same authors-and-their-books data two ways, and a live
    counter shows how many SQL statements each ran: one-per-author (N+1) versus a
    fixed two queries with qiPrefetchHasMany.

    QIVOT_SELFTEST=1 loads both ways then quits (headless check).
 */
#include "models.h"
#include "prefetchstore.h"

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
    conn.addModel<Author>();
    conn.addModel<Book>();
    if (!conn.createTables()) return 1;

    // --- Seed 30 authors, each with a few books ---
    const QStringList firsts = { "Ada","Alan","Grace","Linus","Margaret","Dennis","Barbara",
        "Ken","Donald","Edsger","Tim","Guido","Bjarne","James","Rob","Brian","John","Anita",
        "Radia","Katherine","Shafi","Frances","Leslie","Vint","Whitfield","Adele","Fei-Fei",
        "Yann","Geoffrey","Andrew" };
    for (int i = 0; i < firsts.size(); i++) {
        Author a; a.name = firsts.at(i); a.save();
        const int books = 1 + (i % 5);                       // 1..5 books each
        for (int b = 0; b < books; b++) {
            Book bk; bk.author = a.id(); bk.title = QString("%1 vol.%2").arg(firsts.at(i)).arg(b + 1); bk.save();
        }
    }

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        PrefetchStore *store = root->findChild<PrefetchStore *>();
        QTimer::singleShot(200, &app, [store] {
            if (!store) return;
            store->loadNaive();
            qInfo() << "naive:" << store->mode() << "queries:" << store->queryCount()
                    << "authors:" << store->authors().size();
            store->loadPrefetch();
            qInfo() << "prefetch:" << store->mode() << "queries:" << store->queryCount()
                    << "authors:" << store->authors().size();
        });
        QTimer::singleShot(600, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
