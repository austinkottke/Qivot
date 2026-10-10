#ifndef SAVESUITE_H
#define SAVESUITE_H

/** save() never deletes a row.

    Qivot used to save with REPLACE INTO on SQLite and MySQL, which is a delete
    and an insert. Updating a parent record then fired ON DELETE CASCADE and
    emptied its children, cleared ON DELETE SET NULL links, and a new record
    whose unique value clashed replaced the other row. These checks pin the
    safe behaviour on every backend: INSERT a new record, UPDATE an existing
    one, upsert a model keyed on its own primary key.

    Tables are prefixed `sv_`. The unit tests run this suite on SQLite too.
 */
#include <functional>
#include <QSqlQuery>
#include <qivot.h>

class SvAuthor : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> email;
};
QI_DECLARE_MODEL(SvAuthor, "sv_author", QI_FIELD(name), QI_FIELD(email, QiUnique));

class SvBook : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
    QiForeignKey<SvAuthor, QiFkCascade> author;     // ON DELETE CASCADE
};
QI_DECLARE_MODEL(SvBook, "sv_book", QI_FIELD(title), QI_FIELD(author));

class SvReview : public QiModel {
    QI_MODEL
public:
    QiField<QString> body;
    QiForeignKey<SvAuthor, QiFkSetNull> author;     // ON DELETE SET NULL
};
QI_DECLARE_MODEL(SvReview, "sv_review", QI_FIELD(body), QI_FIELD(author));

class SvSetting : public QiModel {                   // keyed on its own column, no id
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> value;
};
QI_DECLARE_MODEL_NOID(SvSetting, "sv_setting",
    QI_FIELD(name, QiPrimary | QiNotNull), QI_FIELD(value));

namespace SaveSuite {

using Check = std::function<void(bool, const QString &)>;

static int countOf(QiConnection &conn, const QString &sql)
{
    QSqlQuery q = conn.query();
    const int n = q.exec(sql) && q.next() ? q.value(0).toInt() : -1;
    q.finish();
    return n;
}

static void dropAll(QiConnection &conn)
{
    for (const char *t : { "sv_review", "sv_book", "sv_setting", "sv_author" })
        conn.query().exec(QString("DROP TABLE IF EXISTS %1").arg(t));
}

static void run(QiConnection &conn, const QString &backend, const Check &check)
{
    Q_UNUSED(backend);
    qInfo().noquote() << "--- save ---";
    dropAll(conn);
    conn.addModel<SvAuthor>();
    conn.addModel<SvBook>();
    conn.addModel<SvReview>();
    conn.addModel<SvSetting>();
    check(conn.createTables(), QString("save: the tables are created %1").arg(conn.lastError().text()));

    SvAuthor ada;
    ada.name = "Ada";
    ada.email = "ada@example.com";
    check(ada.save() && !ada.id->isNull(), QString("save: a new record is inserted and gets an id (%1)").arg(ada.lastError().text()));
    const QVariant adaId = ada.id();

    for (const char *title : { "Notes", "Engine" }) {
        SvBook b; b.title = title; b.author = ada.id();
        check(b.save(), QString("save: a book (%1)").arg(b.lastError().text()));
    }
    SvReview r; r.body = "Visionary"; r.author = ada.id();
    check(r.save(), QString("save: a review (%1)").arg(r.lastError().text()));

    // The bug: updating the parent deleted it and inserted it again.
    ada.name = "Ada Lovelace";
    check(ada.save(), QString("save: the author is updated (%1)").arg(ada.lastError().text()));
    check(ada.id() == adaId, "save: and keeps its id");
    check(countOf(conn, "SELECT COUNT(*) FROM sv_book") == 2,
          QString("save: ON DELETE CASCADE children survive the update (%1 of 2 books)")
              .arg(countOf(conn, "SELECT COUNT(*) FROM sv_book")));
    check(countOf(conn, QString("SELECT COUNT(*) FROM sv_review WHERE author = %1").arg(adaId.toInt())) == 1,
          "save: ON DELETE SET NULL children keep their link");
    SvAuthor reloaded;
    check(reloaded.load(SvAuthor::col().id == adaId) && reloaded.name.get().toString() == "Ada Lovelace",
          "save: the change is in the database");

    // Saving again with nothing changed is still a success (MySQL reports 0 rows changed).
    check(ada.save(), QString("save: saving an unchanged record succeeds (%1)").arg(ada.lastError().text()));

    // A new record whose unique value clashes fails; it doesn't replace the other row.
    SvAuthor twin;
    twin.name = "Impostor";
    twin.email = "ada@example.com";
    check(!twin.save(), "save: a new record that clashes with a unique value is refused");
    check(twin.validation().error("email") == "is already taken",
          QString("save: and the refusal comes back on the field (%1)").arg(twin.validation().error("email")));
    check(countOf(conn, "SELECT COUNT(*) FROM sv_author") == 1
          && countOf(conn, "SELECT COUNT(*) FROM sv_book") == 2,
          "save: the existing author and their books are untouched");

    // An update that clashes fails too.
    SvAuthor grace; grace.name = "Grace"; grace.email = "grace@example.com";
    check(grace.save(), "save: a second author");
    grace.email = "ada@example.com";
    check(!grace.save(), "save: an update that clashes with a unique value is refused");
    check(countOf(conn, "SELECT COUNT(*) FROM sv_author") == 2, "save: and both authors are still there");
    grace.email = "grace@example.com";

    // A record whose row has gone is inserted again.
    QSqlQuery del = conn.query();
    del.exec(QString("DELETE FROM sv_author WHERE id = %1").arg(grace.id().toInt()));
    del.finish();
    grace.name = "Grace Hopper";
    check(grace.save(), QString("save: a record whose row was deleted is saved again (%1)").arg(grace.lastError().text()));
    SvAuthor back;
    check(back.load(SvAuthor::col().name == "Grace Hopper"), "save: and is in the table");

    // Batch save: loaded records are updated in place, new ones inserted.
    QiList<SvAuthor> all = SvAuthor::objects().all();
    for (int i = 0; i < all.size(); i++)
        all.at(i)->name = all.at(i)->name.get().toString() + " (edited)";
    auto *newcomer = new SvAuthor();
    newcomer->name = "Hedy";
    newcomer->email = "hedy@example.com";
    all.append(newcomer);
    check(all.save(), "save: a batch of edited and new records");
    check(countOf(conn, "SELECT COUNT(*) FROM sv_book") == 2,
          "save: a batch update keeps the children too");
    check(countOf(conn, "SELECT COUNT(*) FROM sv_author") == 3 && !newcomer->id->isNull(),
          "save: and inserts the new one with an id");
    check(SvAuthor::objects().filter(SvAuthor::col().name == "Ada Lovelace (edited)").count() == 1,
          "save: the edits are in the database");

    // A model keyed on its own primary key: the second save updates the row.
    SvSetting theme; theme.name = "theme"; theme.value = "light";
    check(theme.save(), QString("save: a keyed record (%1)").arg(theme.lastError().text()));
    theme.value = "dark";
    check(theme.save(), QString("save: saved again (%1)").arg(theme.lastError().text()));
    check(countOf(conn, "SELECT COUNT(*) FROM sv_setting") == 1
          && SvSetting::objects().filter(SvSetting::col().value == "dark").count() == 1,
          "save: is one row, updated");

    dropAll(conn);
}

} // namespace SaveSuite

#endif // SAVESUITE_H
