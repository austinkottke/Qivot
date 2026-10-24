#include <QtTest>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <qivot.h>
#include <qiasync.h>
#include <qilistmodel.h>
#include <qitransaction.h>
#include <qiconnectionpool.h>
#include "sharedchangetests.h"

class SContact : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<int>     club;
};
QI_DECLARE_MODEL(SContact, "scontact", QI_FIELD(name), QI_FIELD(club));

static QiConnection g_conn;

static QiQuery<SContact> everyone() {
    return SContact::objects().orderBy(QStringList{ "name" });
}

// A second connection, on this thread, to the same file
static QiConnection openSecond(const QString &path, const QString &name) {
    QSqlDatabase db = QSqlDatabase::contains(name) ? QSqlDatabase::database(name)
                                                    : QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(path);
    db.open();
    QiConnection conn;
    conn.open(db, false);
    return conn;
}

static bool saveOn(QiConnection conn, const QString &name) {
    SContact c;
    c.setConnection(conn);
    c.name = name;
    c.club = 1;
    return c.save();
}

void SharedChangeTests::initTestCase()
{
    QVERIFY(m_dir.isValid());
    const QString path = m_dir.filePath("shared.db");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "shared-main");
    db.setDatabaseName(path);
    QVERIFY(db.open());
    g_conn = QiConnection::defaultConnection();     // the models use the default connection
    if (g_conn.isOpen())
        g_conn.close();
    QVERIFY(g_conn.open(db));
    g_conn.addModel<SContact>();
    QVERIFY(g_conn.createTables());
    for (const char *n : { "Bea", "Dan", "Fay" })
        QVERIFY(saveOn(g_conn, n));

    QiAsync::configure("QSQLITE", path);            // workers open the same file
}

void SharedChangeTests::workerSaveReachesLiveList()
{
    QiLiveListModel model;
    model.setQuery(everyone());
    QCOMPARE(model.count(), 3);
    QSignalSpy inserted(&model, SIGNAL(rowsInserted(QModelIndex,int,int)));
    QSignalSpy reset(&model, SIGNAL(modelReset()));

    // A background thread saves through its own pooled connection.
    QFuture<bool> f = QiAsync::run([](QiConnection &c) { return saveOn(c, "Cal"); });
    f.waitForFinished();
    QVERIFY(f.result());

    QTRY_COMPARE(model.count(), 4);                 // delivered to the main thread
    QCOMPARE(inserted.count(), 1);
    QCOMPARE(inserted.at(0).at(1).toInt(), 1);      // Bea, Cal, Dan, Fay
    QCOMPARE(reset.count(), 0);
    QCOMPARE(model.valueAt(1, "name").toString(), QString("Cal"));

    // QiListModel's live mode hears it too.
    QiListModel small;
    small.setLive<SContact>(g_conn, [] { return everyone().all(); });
    QCOMPARE(small.count(), 4);
    f = QiAsync::run([](QiConnection &c) { return saveOn(c, "Eve"); });
    f.waitForFinished();
    QTRY_COMPARE(small.count(), 5);
    QTRY_COMPARE(model.count(), 5);
}

void SharedChangeTests::workerEditAndRemove()
{
    QiLiveListModel model;
    model.setQuery(everyone());
    const QVariant beaId = model.idAt(0);
    QSignalSpy moved(&model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)));
    QSignalSpy removed(&model, SIGNAL(rowsRemoved(QModelIndex,int,int)));

    QFuture<bool> f = QiAsync::run([beaId](QiConnection &c) {
        SContact bea;
        bea.setConnection(c);
        if (!bea.load(SContact::col().id == beaId))
            return false;
        bea.name = "Zoe";                           // to the end
        return bea.save();
    });
    f.waitForFinished();
    QVERIFY(f.result());
    QTRY_COMPARE(moved.count(), 1);
    QCOMPARE(model.indexOf(beaId), model.count() - 1);

    f = QiAsync::run([beaId](QiConnection &c) {
        SContact zoe;
        zoe.setConnection(c);
        return zoe.load(SContact::col().id == beaId) && zoe.remove();
    });
    f.waitForFinished();
    QVERIFY(f.result());
    QTRY_COMPARE(removed.count(), 1);
    QCOMPARE(model.indexOf(beaId), -1);
}

void SharedChangeTests::hookRunsOnItsContextThread()
{
    QObject receiver;                               // lives on the main thread
    QThread *seenOn = nullptr;
    QiChange seen;
    const int hook = g_conn.addRowChangeHook([&](const QiChange &change) {
        seenOn = QThread::currentThread();
        seen = change;
    }, &receiver);

    QFuture<bool> f = QiAsync::run([](QiConnection &c) { return saveOn(c, "Gus"); });
    f.waitForFinished();
    QTRY_VERIFY(seenOn != nullptr);
    QCOMPARE(seenOn, QThread::currentThread());     // not the worker's
    QCOMPARE(seen.table, QString("scontact"));
    QCOMPARE(int(seen.kind), int(QiChange::Inserted));
    QVERIFY(!seen.id.isNull());
    g_conn.removeChangeHook(hook);
}

void SharedChangeTests::transactionWaitsForCommit()
{
    QiConnection other = openSecond(m_dir.filePath("shared.db"), "shared-other");
    QStringList heard;
    QObject receiver;
    const int hook = g_conn.addRowChangeHook([&](const QiChange &c) { heard << c.table; }, &receiver);

    QVERIFY(other.transaction());
    QVERIFY(saveOn(other, "Hal"));
    QVERIFY(saveOn(other, "Ida"));
    QCoreApplication::processEvents();
    QCOMPARE(heard.size(), 0);                      // nothing yet: not committed
    QVERIFY(other.commit());
    QCoreApplication::processEvents();
    QCOMPARE(heard.size(), 2);                      // both, after the commit
    g_conn.removeChangeHook(hook);
}

void SharedChangeTests::rollbackSaysNothing()
{
    QiConnection other = openSecond(m_dir.filePath("shared.db"), "shared-other");
    int heard = 0;
    QObject receiver;
    const int hook = g_conn.addRowChangeHook([&](const QiChange &) { heard++; }, &receiver);
    const int before = SContact::objects().count();

    QVERIFY(other.transaction());
    QVERIFY(saveOn(other, "Jon"));
    QVERIFY(other.rollback());
    QCoreApplication::processEvents();
    QCOMPARE(heard, 0);
    QCOMPARE(SContact::objects().count(), before);
    g_conn.removeChangeHook(hook);
}

void SharedChangeTests::savepointRollbackForgetsItsChanges()
{
    QiConnection other = openSecond(m_dir.filePath("shared.db"), "shared-other");
    QStringList heard;
    QObject receiver;
    const int hook = g_conn.addRowChangeHook([&](const QiChange &c) {
        SContact s;
        if (s.load(SContact::col().id == c.id))
            heard << s.name.get().toString();
    }, &receiver);

    {
        QiTransaction outer(other);
        QVERIFY(saveOn(other, "Kim"));
        {
            QiTransaction inner(other);             // a SAVEPOINT
            QVERIFY(saveOn(other, "Lou"));
            QVERIFY(inner.rollback());
        }
        const bool committed = outer.commit();
        QVERIFY2(committed, qPrintable(QSqlDatabase::database("shared-other").lastError().text()));
    }
    QCoreApplication::processEvents();
    QCOMPARE(heard, QStringList{ "Kim" });          // Lou was rolled back
    g_conn.removeChangeHook(hook);
}

void SharedChangeTests::otherDatabasesStayQuiet()
{
    // Another file with a table of the same name: its writes aren't ours.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "shared-elsewhere");
    db.setDatabaseName(m_dir.filePath("elsewhere.db"));
    QVERIFY(db.open());
    QiConnection elsewhere;
    QVERIFY(elsewhere.open(db, false));
    elsewhere.addModel<SContact>();
    QVERIFY(elsewhere.createTables());

    int heard = 0;
    QObject receiver;
    const int hook = g_conn.addRowChangeHook([&](const QiChange &) { heard++; }, &receiver);
    QVERIFY(saveOn(elsewhere, "Max"));
    QCoreApplication::processEvents();
    QCOMPARE(heard, 0);
    g_conn.removeChangeHook(hook);
}

void SharedChangeTests::poolThreadsComeAndGo()
{
    // Threads that start, use the pool and end, one after another, as a
    // QThreadPool retires idle threads. A new thread often gets a finished
    // one's native id, and must not be handed that thread's connection.
    QiConnectionPool pool;
    pool.configure("QSQLITE", m_dir.filePath("shared.db"));
    int worked = 0;
    for (int i = 0; i < 20; i++) {
        bool ok = false;
        QThread *t = QThread::create([&pool, &ok] {
            QiConnection c = pool.connection();
            ok = c.isOpen() && SContact::objects(c).count() > 0;
        });
        t->start();
        t->wait();
        delete t;
        if (ok)
            worked++;
    }
    QCOMPARE(worked, 20);
}

void SharedChangeTests::cleanupTestCase()
{
    QThreadPool::globalInstance()->waitForDone();
}
