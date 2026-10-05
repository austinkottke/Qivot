#include <QtTest>
#include <QSignalSpy>
#include <QSqlQuery>
#include <qivot.h>
#include <qilistmodel.h>
#include "livelisttests.h"

class LContact : public QiModel {
    QI_MODEL
public:
    QiField<QString> last;
    QiField<QString> first;
    QiField<int>     club;
    QiField<QString> note;
};
QI_DECLARE_MODEL(LContact, "lcontact",
    QI_FIELD(last), QI_FIELD(first), QI_FIELD(club), QI_FIELD(note));

static QiConnection g_conn;
static const int kRows = 10000;

static QiQuery<LContact> clubOne() {
    return LContact::objects().filter(LContact::col().club == 1).orderBy(QStringList{ "last", "first" });
}

// Counts what a model told its views.
struct Signals {
    explicit Signals(QAbstractItemModel *m)
        : reset(m, SIGNAL(modelReset())), inserted(m, SIGNAL(rowsInserted(QModelIndex,int,int))),
          removed(m, SIGNAL(rowsRemoved(QModelIndex,int,int))), moved(m, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int))),
          changed(m, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>))) {}
    void clear() { reset.clear(); inserted.clear(); removed.clear(); moved.clear(); changed.clear(); }
    QSignalSpy reset, inserted, removed, moved, changed;
};

// The model's rows match what the database says now.
static bool inStep(QiLiveListModel &model) {
    const QVariantList ids = clubOne().ids();
    if (ids.size() != model.count())
        return false;
    for (int i = 0; i < ids.size(); i++) {
        if (ids.at(i).toString() != model.idAt(i).toString())
            return false;
    }
    return true;
}

static QVariant idOfLast(const QString &last) {
    LContact c;
    return c.load(LContact::col().last == last) ? c.id() : QVariant();
}

void LiveListTests::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "livelist");
    db.setDatabaseName(":memory:");
    QVERIFY(db.open());
    g_conn = QiConnection::defaultConnection();     // the models use the default connection:
    if (g_conn.isOpen())                            // point it at this database
        g_conn.close();
    QVERIFY(g_conn.open(db));
    g_conn.addModel<LContact>();
    QVERIFY(g_conn.createTables());

    // 10,000 contacts, their names in a scrambled order, all in club 1
    QVERIFY(g_conn.transaction());
    QSqlQuery q = g_conn.query();
    QVERIFY(q.prepare("INSERT INTO lcontact (last, first, club, note) VALUES (?, ?, 1, '')"));
    for (int i = 0; i < kRows; i++) {
        q.addBindValue(QString("L%1").arg((i * 7919) % kRows, 5, 10, QChar('0')));
        q.addBindValue(QString("F%1").arg(i));
        QVERIFY(q.exec());
    }
    QVERIFY(g_conn.commit());
}

void LiveListTests::listModelDiffs()
{
    // QiListModel's live mode re-runs its (small) query, but only the rows
    // that differ reach the view.
    QiListModel model;
    model.setLive<LContact>(g_conn, [] {
        return LContact::objects().filter(LContact::col().club == 7).orderBy("last").all();
    });
    QCOMPARE(model.count(), 0);

    LContact a; a.last = "Ada"; a.first = "A"; a.club = 7; QVERIFY(a.save());
    LContact c; c.last = "Cyd"; c.first = "C"; c.club = 7; QVERIFY(c.save());
    QTest::qWait(10);
    QCOMPARE(model.count(), 2);

    Signals s(&model);
    LContact b; b.last = "Bob"; b.first = "B"; b.club = 7; QVERIFY(b.save());
    QTest::qWait(10);
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.inserted.count(), 1);
    QCOMPARE(s.inserted.at(0).at(1).toInt(), 1);           // between Ada and Cyd
    QCOMPARE(s.changed.count(), 0);                         // Ada and Cyd are as they were

    s.clear();
    a.note = "edited"; QVERIFY(a.save());
    QTest::qWait(10);
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.changed.count(), 1);
    QCOMPARE(s.changed.at(0).at(0).value<QModelIndex>().row(), 0);

    s.clear();
    a.last = "Zed"; QVERIFY(a.save());                      // Ada moves to the end
    QTest::qWait(10);
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.moved.count(), 1);
    QCOMPARE(model.data(model.index(2), model.roleNames().key("last")).toString(), QString("Zed"));

    s.clear();
    QVERIFY(b.remove());
    QTest::qWait(10);
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.removed.count(), 1);
    QCOMPARE(model.count(), 2);

    (void) LContact::objects().filter(LContact::col().club == 7).remove();
}

void LiveListTests::tenThousand()
{
    QiLiveListModel model;
    QElapsedTimer timer;
    timer.start();
    model.setQuery(clubOne(), 100);
    const qint64 ms = timer.elapsed();
    QCOMPARE(model.count(), kRows);
    QCOMPARE(model.queryCount(), 1);                        // the ids, nothing else
    QCOMPARE(model.cachedRecords(), 0);
    qInfo("10,000 ids read in %lld ms", ms);

    QCOMPARE(model.valueAt(0, "last").toString(), QString("L00000"));
    QCOMPARE(model.valueAt(5000, "last").toString(), QString("L05000"));
    QCOMPARE(model.queryCount(), 3);                        // one batch for each
    QCOMPARE(model.cachedRecords(), 200);
    QCOMPARE(model.valueAt(5050, "last").toString(), QString("L05050"));
    QCOMPARE(model.queryCount(), 3);                        // same batch: in memory

    const int lastRole = model.roleNames().key("last");
    QCOMPARE(model.data(model.index(9999), lastRole).toString(), QString("L09999"));
}

void LiveListTests::editInPlace()
{
    QiLiveListModel model;
    model.setQuery(clubOne());
    QCOMPARE(model.valueAt(4000, "note").toString(), QString());
    Signals s(&model);
    const int before = model.queryCount();

    LContact c;
    QVERIFY(c.load(LContact::col().last == "L04000"));
    c.note = "called back";
    QVERIFY(c.save());
    model.flush();

    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.moved.count(), 0);
    QCOMPARE(s.changed.count(), 1);
    QCOMPARE(s.changed.at(0).at(0).value<QModelIndex>().row(), 4000);
    QCOMPARE(model.queryCount() - before, 1);               // where is it now: still 4000
    QCOMPARE(model.valueAt(4000, "note").toString(), QString("called back"));
    QCOMPARE(model.queryCount() - before, 2);               // and the one record, read again
}

void LiveListTests::moveInsertRemove()
{
    QiLiveListModel model;
    model.setQuery(clubOne());
    Signals s(&model);

    // Rename: the row moves to its new place, and only that.
    int before = model.queryCount();
    LContact c;
    QVERIFY(c.load(LContact::col().last == "L07000"));
    const QVariant id = c.id();
    c.last = "L00000a";                                     // just after L00000
    QVERIFY(c.save());
    model.flush();
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.moved.count(), 1);
    QCOMPARE(s.inserted.count() + s.removed.count(), 0);
    QCOMPARE(model.indexOf(id), 1);
    QCOMPARE(model.valueAt(1, "last").toString(), QString("L00000a"));
    QCOMPARE(model.count(), kRows);
    QVERIFY(model.queryCount() - before <= 2);              // its position, then its record
    QVERIFY(inStep(model));

    // A new contact: inserted at its place in the order.
    s.clear();
    before = model.queryCount();
    LContact n; n.last = "L05000b"; n.first = "New"; n.club = 1;
    QVERIFY(n.save());
    model.flush();
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.inserted.count(), 1);
    QCOMPARE(s.inserted.at(0).at(1).toInt(), 5002);        // after L05000, one row lower since L00000a
    QCOMPARE(model.count(), kRows + 1);
    QCOMPARE(model.queryCount() - before, 2);               // its position, and the count check
    QVERIFY(inStep(model));

    // Removed: no query at all.
    s.clear();
    before = model.queryCount();
    QVERIFY(n.remove());
    model.flush();
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.removed.count(), 1);
    QCOMPARE(s.removed.at(0).at(1).toInt(), 5002);
    QCOMPARE(model.queryCount() - before, 0);
    QCOMPARE(model.count(), kRows);

    // Put it back
    c.last = "L07000";
    QVERIFY(c.save());
    model.flush();
    QCOMPARE(model.indexOf(id), 7000);
    QVERIFY(inStep(model));
}

void LiveListTests::leavesTheFilter()
{
    QiLiveListModel model;
    model.setQuery(clubOne());
    Signals s(&model);

    LContact c;
    QVERIFY(c.load(LContact::col().last == "L03000"));
    c.club = 2;                                             // not in club 1 any more
    QVERIFY(c.save());
    model.flush();
    QCOMPARE(s.removed.count(), 1);
    QCOMPARE(s.removed.at(0).at(1).toInt(), 3000);
    QCOMPARE(model.count(), kRows - 1);

    s.clear();
    c.club = 1;                                             // and back again
    QVERIFY(c.save());
    model.flush();
    QCOMPARE(s.inserted.count(), 1);
    QCOMPARE(s.inserted.at(0).at(1).toInt(), 3000);
    QCOMPARE(s.reset.count(), 0);
    QVERIFY(inStep(model));
}

void LiveListTests::severalAtOnce()
{
    // Writes before the event loop runs are worked out together.
    QiLiveListModel model;
    model.setQuery(clubOne());
    Signals s(&model);

    LContact a; a.last = "L06000x"; a.first = "A"; a.club = 1; QVERIFY(a.save());
    LContact b; b.last = "L00100x"; b.first = "B"; b.club = 1; QVERIFY(b.save());
    LContact m;
    QVERIFY(m.load(LContact::col().last == "L08000"));
    m.last = "L00050x"; QVERIFY(m.save());
    LContact r;
    QVERIFY(r.load(LContact::col().last == "L02000"));
    QVERIFY(r.remove());

    QTest::qWait(10);                                       // applied on the event loop
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.inserted.count(), 2);
    QCOMPARE(s.removed.count(), 1);
    QCOMPARE(s.moved.count(), 1);
    QCOMPARE(model.count(), kRows + 1);
    QVERIFY(inStep(model));

    // tidy up
    QVERIFY(a.remove());
    QVERIFY(b.remove());
    m.last = "L08000"; QVERIFY(m.save());
    LContact back; back.last = "L02000"; back.first = "F"; back.club = 1; QVERIFY(back.save());
    model.flush();
    QVERIFY(inStep(model));
}

void LiveListTests::bulkWrite()
{
    // A bulk write reports "many rows": the ids are read again, and only the
    // differences are signalled.
    QiLiveListModel model;
    model.setQuery(clubOne());
    QCOMPARE(model.valueAt(9995, "last").toString(), QString("L09995"));
    Signals s(&model);
    const int before = model.queryCount();

    const int n = LContact::objects().filter(LContact::col().last >= "L09990").update({ { "club", 3 } });
    QCOMPARE(n, 10);
    model.flush();
    QCOMPARE(s.reset.count(), 0);
    QCOMPARE(s.removed.count(), 1);                         // one run of ten rows
    QCOMPARE(s.removed.at(0).at(2).toInt() - s.removed.at(0).at(1).toInt() + 1, 10);
    QCOMPARE(model.queryCount() - before, 1);               // the ids
    QCOMPARE(model.cachedRecords(), 0);                     // any record may have changed
    QCOMPARE(model.count(), kRows - 10);

    (void) LContact::objects().filter(LContact::col().club == 3).update({ { "club", 1 } });
    model.flush();
    QCOMPARE(s.inserted.count(), 1);
    QCOMPARE(model.count(), kRows);
    QVERIFY(inStep(model));
}

void LiveListTests::cacheIsBounded()
{
    QiLiveListModel model;
    model.setQuery(clubOne(), 50);
    model.setMaxCachedRecords(300);
    for (int row = 0; row < 2000; row++)
        QVERIFY(model.valueAt(row, "last").isValid());
    QVERIFY(model.cachedRecords() <= 300);
    QCOMPARE(model.queryCount(), 1 + 2000 / 50);
}
