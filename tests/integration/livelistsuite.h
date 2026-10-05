#ifndef LIVELISTSUITE_H
#define LIVELISTSUITE_H

/** QiLiveListModel against a live database.

    Each save is placed with one ROW_NUMBER() query in the database's own
    dialect, rather than by re-reading the list: rename, insert, leave the
    filter, remove, and a bulk update, each checked against a fresh read.

    Tables are prefixed `ll_`.
 */
#include <functional>
#include <QSqlQuery>
#include <qivot.h>
#include <qilistmodel.h>

class LlContact : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<int>     club;
};
QI_DECLARE_MODEL(LlContact, "ll_contact", QI_FIELD(name), QI_FIELD(club));

namespace LiveListSuite {

using Check = std::function<void(bool, const QString &)>;

static void run(QiConnection &conn, const QString &backend, const Check &check)
{
    Q_UNUSED(backend);
    qInfo().noquote() << "--- live list ---";
    conn.query().exec("DROP TABLE IF EXISTS ll_contact");
    conn.addModel<LlContact>();
    check(conn.createTables(), "livelist: the table is created");

    const int rows = 300;
    QSqlQuery insert = conn.query();
    insert.prepare("INSERT INTO ll_contact (name, club) VALUES (?, 1)");
    conn.transaction();
    for (int i = 0; i < rows; i++) {
        insert.addBindValue(QString("n%1").arg((i * 7) % rows, 4, 10, QChar('0')));
        insert.exec();
    }
    conn.commit();
    insert.finish();

    auto query = [] {
        return LlContact::objects().filter(LlContact::col().club == 1).orderBy("name desc");
    };
    auto inStep = [&](QiLiveListModel &m) {
        const QVariantList ids = query().ids();
        if (ids.size() != m.count())
            return false;
        for (int i = 0; i < ids.size(); i++)
            if (ids.at(i).toString() != m.idAt(i).toString())
                return false;
        return true;
    };

    QiLiveListModel model;
    model.setQuery(query(), 50);
    check(model.count() == rows, QString("livelist: %1 ids read (%2)").arg(model.count()).arg(rows));
    check(model.valueAt(0, "name").toString() == "n0299", "livelist: in the query's order (name desc)");

    bool ok = false;
    LlContact probe;
    probe.load(LlContact::col().name == "n0100");
    const int row = query().rowOf(probe.id(), &ok);
    check(ok && row == 199, QString("livelist: ROW_NUMBER() finds a record's row (%1, ok %2)").arg(row).arg(ok));

    int resets = 0, moves = 0;
    QObject::connect(&model, &QAbstractItemModel::modelReset, [&] { resets++; });
    QObject::connect(&model, &QAbstractItemModel::rowsMoved, [&] { moves++; });
    int before = model.queryCount();
    probe.name = "n9999";                                   // to the top
    const bool edited = probe.save();
    check(edited, QString("livelist: a loaded record saves an edit %1").arg(edited ? QString() : probe.lastError().text()));
    model.flush();
    check(moves == 1 && resets == 0 && model.indexOf(probe.id()) == 0,
          QString("livelist: a rename is one move (moves %1, resets %2)").arg(moves).arg(resets));
    check(model.queryCount() - before == 1, QString("livelist: placed with one query, not a re-read (%1)").arg(model.queryCount() - before));
    check(model.valueAt(0, "name").toString() == "n9999", "livelist: and the record is read again");

    LlContact fresh; fresh.name = "n0150x"; fresh.club = 1;
    fresh.save();
    model.flush();
    check(model.count() == rows + 1 && inStep(model), "livelist: a new record is inserted in its place");

    fresh.club = 2;
    fresh.save();
    model.flush();
    check(model.count() == rows && inStep(model), "livelist: leaving the filter removes the row");

    before = model.queryCount();
    probe.remove();
    model.flush();
    check(model.count() == rows - 1 && model.queryCount() == before, "livelist: a removal costs no query");

    LlContact::objects().filter(LlContact::col().name < "n0010").update({ { "club", 3 } });
    model.flush();
    check(model.count() == rows - 11 && inStep(model) && resets == 0,
          QString("livelist: a bulk update re-reads the ids and keeps the view (%1 rows)").arg(model.count()));

    conn.query().exec("DROP TABLE IF EXISTS ll_contact");
}

} // namespace LiveListSuite

#endif // LIVELISTSUITE_H
