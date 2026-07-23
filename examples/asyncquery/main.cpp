/** Asynchronous queries with QiAsync.

    A large query runs on a background thread and comes back as a QFuture, so the
    main (event-loop / UI) thread stays responsive. This program seeds a big
    table, fires an expensive query off-thread, and prints "tick" from the main
    thread the whole time it's running — then reports the result when it lands.

    Needs a file-based database (worker threads open their own connection to it)
    and QT += concurrent.
 */
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QFile>
#include <qivot.h>
#include <qiasync.h>

class Event : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
};
QI_DECLARE_MODEL(Event, "event", QI_FIELD(title));

// When a query object is *built* on a worker thread, Qivot's bookkeeping briefly
// resolves the default connection's QSqlDatabase handle, and Qt prints a benign
// "requested database does not belong to the calling thread" notice. The query
// itself runs on this thread's own connection (see exec() → connection.query()),
// so the result is correct; we just filter that one noisy line for a clean demo.
static void filterHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    if (msg.contains(QLatin1String("does not belong to the calling thread")))
        return;
    fprintf(stderr, "%s\n", qUtf8Printable(qFormatLogMessage(type, ctx, msg)));
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(filterHandler);
    QCoreApplication app(argc, argv);

    QFile::remove("asyncquery.db");                 // fresh file each run
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("asyncquery.db");
    db.open();
    QiConnection conn;
    if (!conn.open(db)) return 1;
    conn.setJournalMode("WAL");                     // readers don't block the writer
    conn.addModel<Event>();
    if (!conn.dropTables() || !conn.createTables()) return 1;

    // Seed a big table so the query takes real time.
    const int N = 200000;
    qInfo().noquote() << "Seeding" << N << "rows…";
    QiList<Event> seed;
    QiListWriter writer(&seed);
    for (int i = 1; i <= N; i++)
        writer << QStringLiteral("Event %1").arg(i) << writer.next();
    seed.save();

    // Tell QiAsync how worker threads should open the same database.
    QiAsync::configure("QSQLITE", "asyncquery.db");
    qInfo().noquote() << "main thread:" << QThread::currentThreadId();

    QElapsedTimer clock; clock.start();

    // Run the heavy fetch off-thread; get an int (row count) back as a QFuture.
    auto *watcher = new QFutureWatcher<int>(&app);
    QObject::connect(watcher, &QFutureWatcherBase::finished, &app, [&]{
        qInfo().noquote() << "  <- result:" << watcher->result()
                          << "rows, wall time" << clock.elapsed() << "ms";
        app.quit();
    });

    watcher->setFuture( QiAsync::run([](QiConnection &c) -> int {
        QElapsedTimer t; t.start();
        const int rows = Event::objects(c).all().size();     // materialize everything
        qInfo().noquote() << "  [worker" << QThread::currentThreadId()
                          << "] fetched" << rows << "rows in" << t.elapsed() << "ms";
        return rows;
    }) );

    // Meanwhile the main event loop keeps running — prove it.
    int ticks = 0;
    QTimer ticker;
    QObject::connect(&ticker, &QTimer::timeout, &app, [&]{
        qInfo().noquote() << "  . main-thread tick" << ++ticks << "(not blocked)";
    });
    ticker.start(15);

    return app.exec();
}
