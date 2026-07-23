/** Analytics dashboard — showcases several later Qivot features in QML:

    - qiRawQuery: a CTE + window-function leaderboard (rank + running total),
      mapped into typed rows.
    - QiAsync + QiConnectionPool + QiCancelToken: a heavy "Recompute" job runs on
      a worker thread with a live progress bar and a working Cancel button.

    Uses a file database (async needs a per-thread connection to a real file).
    QIVOT_SELFTEST=1 drives refresh + a recompute then quits (headless check).
 */
#include "models.h"
#include "dashboardstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QFile>
#include <QTimer>
#include <QDebug>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QFile::remove("dashboard.db");                    // fresh each launch
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("dashboard.db");
    db.open();

    QiConnection conn;
    if (!conn.open(db)) return 1;
    conn.setJournalMode("WAL");                       // reader (worker) + writer
    conn.addModel<Customer>();
    conn.addModel<Sale>();
    if (!conn.createTables()) return 1;

    // --- Seed customers and sales ---
    const QStringList names = {
        "Ada Lovelace", "Alan Turing", "Grace Hopper", "Linus Torvalds",
        "Margaret Hamilton", "Dennis Ritchie", "Barbara Liskov", "Ken Thompson" };

    QVector<int> customerIds;
    for (const QString &n : names) {
        Customer c; c.name = n; c.save();
        customerIds << c.id->toInt();
    }
    // Deterministic but varied sales.
    QiList<Sale> sales;
    QiListWriter writer(&sales);
    for (int i = 0; i < 600; i++) {
        const int cust = customerIds.at((i * 7) % customerIds.size());
        const int amount = 50 + (i * 37) % 950;       // $50 .. $999
        writer << cust << amount << writer.next();
    }
    sales.save();

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        DashboardStore *store = root->findChild<DashboardStore *>();
        QTimer::singleShot(200, &app, [store] {
            if (!store) return;
            qInfo() << "leaderboard rows:" << store->leaderboard().size()
                    << "grand total:" << store->grandTotal();
            if (!store->leaderboard().isEmpty())
                qInfo() << "top:" << store->leaderboard().first().toMap();
            store->recompute();
        });
        QTimer::singleShot(1200, &app, [store] { if (store) store->cancel(); });
        QTimer::singleShot(2000, &app, [store] { if (store) qInfo() << "status:" << store->status(); });
        QTimer::singleShot(2400, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
