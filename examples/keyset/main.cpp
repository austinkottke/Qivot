/** Keyset (cursor / "seek") pagination with QiKeyset.

    Instead of LIMIT/OFFSET — which re-scans every skipped row and slows down the
    deeper you page — QiKeyset remembers the last row's key and fetches the rows
    *after* it (WHERE id > :cursor ORDER BY id LIMIT n). Every page is one index
    seek, so page 1 and page 10,000 cost the same.

    It also yields a compact cursor you can persist and resume from.
 */
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <qivot.h>

class Event : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
};
QI_DECLARE_MODEL(Event, "event", QI_FIELD(title));

static void printPage(int n, const QiList<Event> &page) {
    if (page.size() == 0) { qInfo().noquote() << "  page" << n << ": (empty)"; return; }
    qInfo().noquote() << "  page" << n << ":" << page.size() << "rows, id"
                      << page.at(0)->id->toInt() << "->"
                      << page.at(page.size() - 1)->id->toInt();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();
    QiConnection conn;
    if (!conn.open(db)) return 1;
    conn.addModel<Event>();
    if (!conn.createTables()) return 1;

    // Seed 250 events.
    QiList<Event> seed;
    QiListWriter writer(&seed);
    for (int i = 1; i <= 250; i++)
        writer << QStringLiteral("Event %1").arg(i, 3, 10, QChar('0')) << writer.next();
    seed.save();

    // --- Page through everything, 100 at a time --------------------------
    qInfo().noquote() << "Paging 250 events, 100 per page:";
    QiKeyset<Event> pager("id", 100);
    int n = 0;
    while (!pager.atEnd())
        printPage(++n, pager.next());

    // --- Save a cursor mid-stream and resume from it ---------------------
    qInfo().noquote() << "\nResume from a saved cursor:";
    QiKeyset<Event> p1("id", 100);
    p1.next();                                   // consume page 1 (ids 1..100)
    const QVariant cursor = p1.cursor();         // persist this anywhere
    qInfo().noquote() << "  saved cursor (last id of page 1):" << cursor.toInt();

    QiKeyset<Event> p2("id", 100);
    p2.seek(cursor);                             // brand-new pager, resumed
    printPage(2, p2.next());                     // continues at id 101

    // --- Page a filtered subset, newest first ----------------------------
    qInfo().noquote() << "\nDescending, filtered (id <= 120), 50 per page:";
    QiKeyset<Event> filtered("id", 50, /*ascending=*/false,
                             Event::col().id <= 120);
    int m = 0;
    while (!filtered.atEnd())
        printPage(++m, filtered.next());

    return 0;
}
