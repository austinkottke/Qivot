#ifndef VALIDATIONSUITE_H
#define VALIDATIONSUITE_H

/** QiValidation's database rules against a live database.

    The rules that run SQL: uniqueness (QiUnique, QiUniqueIn with a scope),
    QiExists, QiNoOverlap with date-times, enforced() rules written into
    CREATE TABLE as CHECK constraints in each dialect, and a duplicate the
    database refuses mapped back to its field.

    Tables are prefixed `vs_`.
 */
#include <functional>
#include <QDateTime>
#include <QSqlQuery>
#include <qivot.h>

class VsMember : public QiModel {
    QI_MODEL
public:
    QiField<QString> email;
    QiField<QString> name;
    QiField<int>     club;
    QiField<QString> nickname;
};
QI_DECLARE_MODEL(VsMember, "vs_member",
    QI_FIELD(email,    QiNotNull | QiUnique | QiTrim() | QiLower() | QiEmail()),
    QI_FIELD(name,     QiLength(2, 40).enforced()),
    QI_FIELD(club),
    QI_FIELD(nickname, QiUniqueIn({ "club" })));

class VsRoom : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(VsRoom, "vs_room", QI_FIELD(name));

class VsSlot : public QiModel {
    QI_MODEL
public:
    QiField<int>       room_id;
    QiField<QDateTime> starts_at;
    QiField<QDateTime> ends_at;
};
QI_DECLARE_MODEL(VsSlot, "vs_slot",
    QI_FIELD(room_id,   QiNotNull | QiExists("vs_room")),
    QI_FIELD(starts_at, QiNotNull),
    QI_FIELD(ends_at,   QiNotNull | QiAfter("starts_at") | QiNoOverlap("starts_at", { "room_id" })));

namespace ValidationSuite {

using Check = std::function<void(bool, const QString &)>;

static void run(QiConnection &conn, const QString &backend, const Check &check)
{
    qInfo().noquote() << "--- validation ---";
    for (const char *t : { "vs_slot", "vs_room", "vs_member" })
        conn.query().exec(QString("DROP TABLE IF EXISTS %1").arg(t));
    conn.addModel<VsMember>();
    conn.addModel<VsRoom>();
    conn.addModel<VsSlot>();
    const bool created = conn.createTables();
    check(created, QString("validation: the tables, with an enforced CHECK, are created %1").arg(created ? QString() : conn.lastError().text()));

    // enforced(): the database refuses what the rule refuses.
    QSqlQuery q = conn.query();
    check(!q.exec("INSERT INTO vs_member (email, name) VALUES ('raw@example.com', 'A')"),
          "validation: the CHECK from QiLength(2, 40).enforced() refuses a 1-character name");
    q.finish();

    VsMember ada;
    ada.email = "  Ada@Example.com ";
    ada.name = "Ada";
    ada.club = 1;
    ada.nickname = "countess";
    check(ada.save(), QString("validation: a valid member saves (%1)").arg(ada.lastError().text()));
    check(ada.email.get().toString() == "ada@example.com", "validation: normalised before saving");

    VsMember twin;
    twin.email = "ADA@example.com";
    twin.name = "Ada Two";
    twin.club = 1;
    check(twin.validate().error("email") == "is already taken", "validation: QiUnique is checked against the table");
    // save() leaves QiUnique to the database. On SQLite, DuckDB and MySQL save() is a
    // REPLACE, which would remove Ada instead, so there it's validate() that matters.
    if (backend == "postgres" || backend == "sqlserver") {
        check(!twin.save(), "validation: the database refuses the duplicate");
        if (backend == "postgres")
            check(twin.validation().error("email") == "is already taken",
                  QString("validation: the database's refusal comes back on the field (%1)").arg(twin.lastError().text().left(80)));
    }

    VsMember sameClub;
    sameClub.email = "grace@example.com"; sameClub.name = "Grace"; sameClub.club = 1; sameClub.nickname = "countess";
    check(sameClub.validate().error("nickname") == "is already taken", "validation: QiUniqueIn({\"club\"}) within the club");
    sameClub.club = 2;
    check(sameClub.validate().isValid(), "validation: and free in another club");

    ada.name = "Ada Lovelace";
    check(ada.validate().isValid(), "validation: editing a record doesn't clash with itself");

    VsSlot orphan;
    orphan.room_id = 4242;
    orphan.starts_at = QDateTime(QDate(2030, 1, 7), QTime(9, 0));
    orphan.ends_at = QDateTime(QDate(2030, 1, 7), QTime(10, 0));
    check(orphan.validate().error("room_id") == "doesn't exist", "validation: QiExists");

    VsRoom room; room.name = "Atlas";
    check(room.save(), "validation: a room");
    auto slot = [&](QTime from, QTime to) {
        VsSlot s;
        s.room_id = room.id().toInt();
        s.starts_at = QDateTime(QDate(2030, 1, 7), from);
        s.ends_at = QDateTime(QDate(2030, 1, 7), to);
        return s;
    };
    VsSlot ten = slot(QTime(10, 0), QTime(11, 0));
    check(ten.save(), QString("validation: a booking 10:00-11:00 (%1)").arg(ten.lastError().text()));
    VsSlot clash = slot(QTime(10, 30), QTime(11, 30));
    check(clash.validate().error("ends_at") == "overlaps another entry", "validation: QiNoOverlap catches 10:30-11:30");
    check(!clash.save(), "validation: and save() refuses it");
    VsSlot next = slot(QTime(11, 0), QTime(12, 0));
    check(next.validate().isValid(), QString("validation: 11:00-12:00 touches but doesn't overlap (%1)").arg(next.validation().summary()));
    ten.ends_at = QDateTime(QDate(2030, 1, 7), QTime(10, 45));
    check(ten.validate().isValid(), "validation: moving a booking within its own time is fine");

    for (const char *t : { "vs_slot", "vs_room", "vs_member" })
        conn.query().exec(QString("DROP TABLE IF EXISTS %1").arg(t));
}

} // namespace ValidationSuite

#endif // VALIDATIONSUITE_H
