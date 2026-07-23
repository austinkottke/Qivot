/** DB-backed infinite scroll — a gradient discovery feed.

    Seeds 5,000 richly-varied rows, then binds a ListView to a QiLazyListModel
    that streams them 24 at a time via canFetchMore()/fetchMore() — only as far
    as you scroll. Nothing but the pages you've reached is ever held in memory.

    QIVOT_SELFTEST=1 loads a few pages then quits (for headless checks).
 */
#include "item.h"
#include "itemstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QVector>
#include <QPair>
#include <QStringList>
#include <QTimer>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("infinitescroll.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Item>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    // --- Generate 5,000 vibrant, deterministic cards ---
    const QVector<QPair<QString, QString>> grads = {
        {"#FF6CAB","#7366FF"}, {"#42E695","#3BB2B8"}, {"#FDBB2D","#22C1C3"},
        {"#F45C43","#EB3349"}, {"#8E2DE2","#4A00E0"}, {"#F7971E","#FFD200"},
        {"#00C9FF","#92FE9D"}, {"#FC466B","#3F5EFB"}, {"#11998E","#38EF7D"},
        {"#FF512F","#DD2476"}, {"#1FA2FF","#12D8FA"}, {"#D4145A","#FBB03B"},
        {"#5F2C82","#49A09D"}, {"#F00000","#DC281E"}, {"#00B4DB","#0083B0"},
        {"#EECDA3","#EF629F"} };
    const QStringList adjs = {
        "Cosmic","Neon","Velvet","Lunar","Solar","Crystal","Electric","Golden",
        "Midnight","Aurora","Coral","Emerald","Sapphire","Crimson","Frost","Ember",
        "Mystic","Radiant","Silent","Wild" };
    const QStringList nouns = {
        "Drift","Bloom","Pulse","Wave","Horizon","Cascade","Mirage","Echo",
        "Nova","Vortex","Prism","Dune","Reef","Halo","Comet","Glacier",
        "Storm","Meadow","Zephyr","Bliss" };
    const QStringList cats = {
        "Abstract","Gradient","Texture","Pattern","Landscape","Aurora","Study","Nebula" };

    QiList<Item> seed;
    QiListWriter writer(&seed);
    const int N = 5000;
    for (int i = 0; i < N; i++) {
        const QPair<QString, QString> g = grads.at((i * 7) % grads.size());
        const QString title = adjs.at(i % adjs.size()) + " " +
                              nouns.at((i / adjs.size()) % nouns.size());
        const QString cat = cats.at((i * 3) % cats.size());
        const int metric = int((quint32(i) * 2654435761u) % 9900u) + 100;
        writer << title << cat << g.first << g.second << metric << writer.next();
    }
    seed.save();

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        QAbstractItemModel *m = root->findChild<ItemStore *>()
                                    ? root->findChild<ItemStore *>()->items() : nullptr;
        QTimer::singleShot(300, &app, [m]{ if (m) { m->fetchMore(QModelIndex()); m->fetchMore(QModelIndex()); } });
        QTimer::singleShot(900, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
