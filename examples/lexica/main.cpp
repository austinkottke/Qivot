/** Lexica — instant full-text search over a big corpus.

    Seeds tens of thousands of short "entries" (a title + a sentence), builds a
    single FTS5 full-text index over them, and searches as you type — ranked by
    relevance, in a millisecond or two. The read-side companion to Fluxo.

    Showcases: QiFtsIndex (FTS5 external-content index + auto-sync triggers),
    QiQuery::search() (MATCH + relevance rank), and how a bulk seed + one index
    build gives you Google-speed search over a local table.

    QIVOT_SELFTEST=1 seeds, searches a couple of terms headless, prints timings, quits.
 */
#include "doc.h"
#include "searchstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QElapsedTimer>
#include <QStringList>
#include <QVector>
#include <QDebug>

static const int kCorpusSize = 80000;

// --- Themed word banks: enough overlap that searches return meaningful, ranked
//     results, enough variety that the corpus feels real. ---
static const QStringList kAdj = {
    "cosmic","silent","ancient","electric","hidden","restless","gilded","frozen",
    "crimson","hollow","luminous","feral","distant","molten","velvet","brittle",
    "sacred","savage","tender","weightless","fractured","endless","gentle","obsidian",
    "radiant","quiet","wandering","brass","glass","drifting","burning","pale" };
static const QStringList kNoun = {
    "ocean","forest","machine","memory","signal","garden","engine","shadow",
    "mountain","river","circuit","dream","storm","harbor","comet","ember",
    "glacier","lantern","orchard","temple","tower","meadow","desert","tide",
    "beacon","cavern","valley","reef","aurora","satellite","furnace","willow" };
static const QStringList kVerb = {
    "drifts","breaks","remembers","ignites","dissolves","awakens","wanders","echoes",
    "collapses","blooms","fractures","hums","gathers","unravels","glimmers","waits",
    "burns","forgets","rises","turns","listens","folds","sings","sleeps" };
static const QStringList kAbstract = {
    "silence","gravity","distance","memory","entropy","hope","rust","dust",
    "light","time","longing","static","winter","fire","salt","thunder" };
static const QStringList kCategory = {
    "Field Notes","Dreams","Machines","Voyages","Fragments","Almanac","Letters","Observations" };

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("lexica.db");
    if (!db.open()) return 1;

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Doc>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    // --- 1) Seed the corpus (fast batch insert, no index yet) ---
    QElapsedTimer t; t.start();
    QiList<Doc> seed;
    QiListWriter w(&seed);
    for (int i = 0; i < kCorpusSize; i++) {
        const QString a1 = kAdj.at((i * 7) % kAdj.size());
        const QString n1 = kNoun.at((i * 3) % kNoun.size());
        const QString v1 = kVerb.at((i * 5) % kVerb.size());
        const QString a2 = kAdj.at((i * 11 + 4) % kAdj.size());
        const QString n2 = kNoun.at((i * 13 + 2) % kNoun.size());
        const QString ab = kAbstract.at((i * 17) % kAbstract.size());
        const QString v2 = kVerb.at((i * 19 + 1) % kVerb.size());

        QString title;
        switch (i % 3) {
            case 0: title = a1 + " " + n1; break;                       // "cosmic ocean"
            case 1: title = "The " + n1 + " of " + ab; break;           // "The ocean of memory"
            default: {                                                  // "Ocean & Willow"
                QString N1 = n1; N1[0] = n1.at(0).toUpper();
                QString N2 = n2; N2[0] = n2.at(0).toUpper();
                title = N1 + " & " + N2;
            }
        }
        const QString body  = "The " + a1 + " " + n1 + " " + v1 + " through the " + a2 +
                              " " + n2 + ", where " + ab + " " + v2 + " and nothing is still.";
        const QString cat   = kCategory.at((i * 23) % kCategory.size());
        w << title << body << cat << w.next();
    }
    seed.save();
    const double seedMs = t.nsecsElapsed() / 1.0e6;

    // --- 2) Build the full-text index over title + body (one bulk rebuild) ---
    t.restart();
    QiFtsIndex<Doc> fts("doc_fts");
    fts << "title" << "body";
    if (!connection.createFtsIndex(fts)) {
        qWarning() << "createFtsIndex failed:" << connection.lastError().text();
        return 1;
    }
    const double indexMs = t.nsecsElapsed() / 1.0e6;
    qInfo().noquote() << QString("Lexica: seeded %1 docs in %2 ms, indexed in %3 ms")
                          .arg(kCorpusSize).arg(seedMs, 0, 'f', 0).arg(indexMs, 0, 'f', 0);

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        for (const QString &term : { QString("cosmic ocean"), QString("memory"), QString("oce") }) {
            QElapsedTimer q; q.start();
            QStringList toks;
            for (const QString &w2 : term.split(' ', Qt::SkipEmptyParts)) toks << w2 + "*";
            const int n = QiQuery<Doc>().search("doc_fts", toks.join(' ')).count();
            qInfo().noquote() << QString("  \"%1\" -> %2 matches in %3 ms")
                                  .arg(term).arg(n).arg(q.nsecsElapsed() / 1.0e6, 0, 'f', 2);
        }
        return 0;
    }

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
