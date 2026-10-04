#include <QtTest>
#include <QJsonArray>
#include <QSqlQuery>
#include <QTimeZone>
#include <qivot.h>
#include <qisqlitestatement.h>
#include "validationtests.h"

// --- Models ------------------------------------------------------------------

class VUser : public QiModel {
    QI_MODEL
public:
    QiField<QString> email;
    QiField<QString> name;
    QiField<QString> username;
    QiField<QString> password;
    QiField<QString> password_confirm;
    QiField<QString> country;
    QiField<QString> state;
    QiField<QString> plan;
    QiField<int>     age;
    QiField<double>  price;
    QiField<QDate>   born;
};
QI_DECLARE_MODEL(VUser, "v_user",
    QI_FIELD(email,    QiNotNull | QiUnique | QiTrim() | QiLower() | QiEmail()),
    QI_FIELD(name,     QiLabel("Full name") | QiCollapseSpaces() | QiLength(2, 40)),
    QI_FIELD(username, QiPattern("^[a-z0-9_]{3,20}$", "may only use a-z, 0-9 and _ (3 to 20 of them)")),
    QI_FIELD(password, QiStrongPassword(8)),
    QI_FIELD(password_confirm, QiLabel("Confirmation") | QiSameAs("password")),
    QI_FIELD(country),
    QI_FIELD(state,    QiRequiredIf("country", "US")),
    QI_FIELD(plan,     QiOneOf({ "free", "pro" }).enforced() | QiRequired().on("upgrade")),
    QI_FIELD(age,      QiRange(13, 130).enforced()),
    QI_FIELD(price,    QiDecimals(2) | QiMax(1000).warning().message("is unusually high")),
    QI_FIELD(born,     QiPast() | QiMinAge(13)));

class VRoom : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(VRoom, "v_room", QI_FIELD(name));

class VBooking : public QiModel {
    QI_MODEL
public:
    QiField<int>       room_id;
    QiField<QDateTime> starts_at;
    QiField<QDateTime> ends_at;
};
QI_DECLARE_MODEL(VBooking, "v_booking",
    QI_FIELD(room_id,   QiNotNull | QiExists("v_room")),
    QI_FIELD(starts_at, QiNotNull | QiTimeStep(15) | QiTimeBetween(QTime(8, 0), QTime(20, 0))),
    QI_FIELD(ends_at,   QiNotNull | QiAfter("starts_at") | QiMinutesAfter("starts_at", 15, 240)
                        | QiNoOverlap("starts_at", { "room_id" }).message("is booked by someone else")));

class VProduct : public QiModel {
    QI_MODEL
public:
    QiField<int>     store_id;
    QiField<QString> sku;
    QiField<double>  price;
    QiField<int>     pack;
};
QI_DECLARE_MODEL(VProduct, "v_product",
    QI_FIELD(store_id, QiNotNull),
    QI_FIELD(sku,      QiTrim() | QiUpper() | QiUniqueIn({ "store_id" })),
    QI_FIELD(price,    QiPositive() | QiDecimals(2)),
    QI_FIELD(pack,     QiMultipleOf(6)));

class VStay : public QiModel {
    QI_MODEL
public:
    QiField<QDate>     check_in;
    QiField<QDate>     check_out;
    QiField<QString>   card;
    QiField<QString>   card_expiry;
    QiField<QDateTime> opens;
    QiField<QDate>     meeting;
    QiField<QDate>     weekend_trip;
};
static bool isTestHoliday(const QDate &d) { return d.month() == 12 && d.day() == 25; }
QI_DECLARE_MODEL(VStay, "v_stay",
    QI_FIELD(check_in,    QiTodayOrLater() | QiDateMax("today+1y")),
    QI_FIELD(check_out,   QiDaysAfter("check_in", 1, 30).message("must be 1 to 30 nights after check-in")),
    QI_FIELD(card,        QiLuhn()),
    QI_FIELD(card_expiry, QiNotExpired()),
    QI_FIELD(opens,       QiExistingLocalTime("Europe/Berlin")),
    QI_FIELD(meeting,     QiBusinessDay(isTestHoliday)),
    QI_FIELD(weekend_trip, QiWeekend()));

static QiConnection g_conn;

static VUser goodUser(const QString &email = QStringLiteral("ada@example.com"))
{
    VUser u;
    u.email = email;
    u.name = QStringLiteral("Ada Lovelace");
    u.username = QStringLiteral("ada_l");
    u.password = QStringLiteral("Analytic1");
    u.password_confirm = QStringLiteral("Analytic1");
    u.plan = QStringLiteral("free");
    u.age = 36;
    u.born = QDate(1815, 12, 10);
    return u;
}

// --- Tests -------------------------------------------------------------------

void ValidationTests::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "validation");
    db.setDatabaseName(":memory:");
    QVERIFY(db.open());
    QVERIFY(g_conn.open(db));                     // the default connection (SqliteTests closed theirs)
    g_conn.addModel<VUser>();
    g_conn.addModel<VRoom>();
    g_conn.addModel<VBooking>();
    g_conn.addModel<VProduct>();
    g_conn.addModel<VStay>();
    QVERIFY(g_conn.createTables());
}

void ValidationTests::textRules()
{
    VUser u = goodUser();
    QVERIFY2(u.validate().isValid(), qPrintable(u.validation().summary()));

    VUser bad;
    bad.email = QStringLiteral("not-an-email");
    bad.name = QStringLiteral("A");
    bad.username = QStringLiteral("Ada Lovelace!");
    bad.password = QStringLiteral("short");
    bad.password_confirm = QStringLiteral("different");
    const QiValidation v = bad.validate();
    QVERIFY(!v.isValid());
    QCOMPARE(v.error("email"), QString("is not a valid email address"));
    QCOMPARE(v.error("name"), QString("must be 2 to 40 characters"));
    QCOMPARE(v.error("username"), QString("may only use a-z, 0-9 and _ (3 to 20 of them)"));
    QCOMPARE(v.error("password"), QString("must be at least 8 characters, with a capital letter, a small letter and a digit"));
    QCOMPARE(v.error("password_confirm"), QString("doesn't match Password"));
    QVERIFY(v.summary().contains("Full name: must be 2 to 40 characters"));      // QiLabel
    QVERIFY(v.summary().contains("Confirmation: doesn't match Password"));
    QCOMPARE(v.errorMap().value("email").toString(), QString("is not a valid email address"));
    QCOMPARE(v.toJson().value("valid").toBool(), false);
    QCOMPARE(v.toJson().value("errors").toObject().value("email").toArray().first().toString(), QString("is not a valid email address"));

    // Missing a NOT NULL field: required, before the database says so.
    VUser empty;
    QCOMPARE(empty.validate().error("email"), QString("is required"));
    QVERIFY(!empty.save());
    QCOMPARE(empty.lastError().type(), QiError::ValidationError);
    QVERIFY(empty.lastError().text().contains("Email: is required"));

    // Optional fields: an empty value passes the format rules.
    VUser optional = goodUser("opt@example.com");
    optional.username = QString();
    optional.password = QString();
    optional.password_confirm = QString();
    QVERIFY2(optional.validate().isValid(), qPrintable(optional.validation().summary()));
}

void ValidationTests::normalizing()
{
    VUser u = goodUser(QStringLiteral("  Grace.Hopper@Example.COM "));
    u.name = QStringLiteral("  Grace    Brewster   Hopper ");
    QVERIFY2(u.save(), qPrintable(u.lastError().text()));
    QCOMPARE(u.email.get().toString(), QString("grace.hopper@example.com"));
    QCOMPARE(u.name.get().toString(), QString("Grace Brewster Hopper"));
}

void ValidationTests::numbersAndChoices()
{
    VUser u = goodUser("num@example.com");
    u.age = 9;
    u.plan = QStringLiteral("enterprise");
    u.price = 9.999;
    const QiValidation v = u.validate();
    QCOMPARE(v.error("age"), QString("must be between 13 and 130"));
    QCOMPARE(v.error("plan"), QString("must be one of free, pro"));
    QCOMPARE(v.error("price"), QString("can have at most 2 decimal places"));

    VProduct p;
    p.store_id = 1;
    p.sku = QStringLiteral("a1");
    p.price = 0;
    p.pack = 10;
    const QiValidation pv = p.validate();
    QCOMPARE(pv.error("price"), QString("must be more than 0"));
    QCOMPARE(pv.error("pack"), QString("must be a multiple of 6"));
    p.price = 4.95;
    p.pack = 12;
    QVERIFY2(p.validate().isValid(), qPrintable(p.validation().summary()));
}

void ValidationTests::enforcedChecks()
{
    // enforced() rules are in the table too: the database refuses what the app would.
    QiSqliteStatement statement;
    const QString sql = statement.createTableIfNotExists<VUser>();
    QVERIFY2(sql.contains("CHECK (plan IN ('free', 'pro'))"), qPrintable(sql));
    QVERIFY(sql.contains("CHECK (age >= 13 AND age <= 130)"));
    QVERIFY(!sql.contains("CHECK (length(name)"));          // not enforced(): app-side only

    QSqlQuery q = g_conn.query();
    QVERIFY(!q.exec("INSERT INTO v_user (email, plan) VALUES ('raw@example.com', 'gold')"));
    QVERIFY(q.exec("INSERT INTO v_user (email, plan) VALUES ('raw@example.com', 'pro')"));
}

void ValidationTests::conditionalAndCompare()
{
    VUser u = goodUser("us@example.com");
    u.country = QStringLiteral("US");
    QCOMPARE(u.validate().error("state"), QString("is required"));
    u.state = QStringLiteral("CA");
    QVERIFY(u.validate().isValid());
    u.country = QStringLiteral("FR");
    u.state = QString();
    QVERIFY(u.validate().isValid());                       // only for the US

    VUser m = goodUser("match@example.com");
    m.password_confirm = QStringLiteral("Analytic1");
    QVERIFY(!m.validate().hasError("password_confirm"));
}

void ValidationTests::warningsAndContexts()
{
    VUser u = goodUser("rich@example.com");
    u.price = 5000;
    const QiValidation v = u.validate();
    QVERIFY(v.isValid());                                   // a warning doesn't stop it
    QVERIFY(v.hasWarnings());
    QCOMPARE(v.warning("price"), QString("is unusually high"));
    QCOMPARE(v.warningMap().value("price").toString(), QString("is unusually high"));
    QVERIFY(u.save());

    // A rule only in the "upgrade" context.
    VUser free = goodUser("ctx@example.com");
    free.plan = QString();
    QVERIFY(free.validate().isValid());
    QCOMPARE(free.validate("upgrade").error("plan"), QString("is required"));
}

class VEvent : public QiModel {
    QI_MODEL
public:
    QiField<QDate>   starts;
    QiField<QDate>   ends;
    QiField<QString> title;

    bool clean() override {
        // A rule across fields, with clean().
        if (!starts->isNull() && !ends->isNull() && ends.get().toDate() < starts.get().toDate())
            addError("ends", "can't be before the start");
        return true;
    }
};
QI_DECLARE_MODEL(VEvent, "v_event", QI_FIELD(starts), QI_FIELD(ends), QI_FIELD(title, QiRequired()));

void ValidationTests::cleanAndPartial()
{
    g_conn.addModel<VEvent>();
    QVERIFY(g_conn.createTables());
    VEvent e;
    e.starts = QDate(2026, 5, 10);
    e.ends = QDate(2026, 5, 1);
    QiValidation v = e.validate();
    QCOMPARE(v.error("ends"), QString("can't be before the start"));
    QCOMPARE(v.error("title"), QString("is required"));
    QVERIFY(!e.save());
    QVERIFY(e.validation().hasError("ends"));

    // Only the field being typed in.
    v = e.validateFields({ "title" });
    QVERIFY(v.hasError("title"));
    QVERIFY(!v.hasError("ends"));
    e.title = QStringLiteral("Launch");
    QVERIFY(e.validateFields({ "title" }).isValid());
}

void ValidationTests::uniqueAndDatabaseErrors()
{
    VUser first = goodUser("taken@example.com");
    QVERIFY(first.save());

    VUser second = goodUser("TAKEN@example.com ");       // normalised to the same address
    QCOMPARE(second.validate().error("email"), QString("is already taken"));

    // save() leaves QiUnique to the database. Where it refuses (PostgreSQL, MySQL,
    // SQL Server), that error comes back on the field (fromDatabaseError, below);
    // SQLite's save() is a REPLACE, which removes the other row instead, so a form
    // calls validate() first (or the app uses upsert()).

    // Editing the record itself isn't a clash with itself.
    first.name = QStringLiteral("Taken Again");
    QVERIFY2(first.validate().isValid(), qPrintable(first.validation().summary()));
    QVERIFY(first.save());

    QCOMPARE(QiValidator::fromDatabaseError("duplicate key value violates unique constraint \"v_user_email_key\"\nDETAIL:  Key (email)=(x@y.z) already exists.",
                                            qiMetaInfo<VUser>()).field, QString("email"));
    QCOMPARE(QiValidator::fromDatabaseError("Duplicate entry 'x@y.z' for key 'v_user.email'", qiMetaInfo<VUser>()).field, QString("email"));
    QCOMPARE(QiValidator::fromDatabaseError("Column 'email' cannot be null", qiMetaInfo<VUser>()).rule, QString("required"));
    QVERIFY(QiValidator::fromDatabaseError("something else", qiMetaInfo<VUser>()).field.isEmpty());
}

void ValidationTests::scopedUniqueAndExists()
{
    VProduct a; a.store_id = 1; a.sku = "box-1"; a.price = 1; QVERIFY2(a.save(), qPrintable(a.lastError().text()));
    VProduct sameStore; sameStore.store_id = 1; sameStore.sku = " BOX-1"; sameStore.price = 1;
    QCOMPARE(sameStore.validate().error("sku"), QString("is already taken"));
    QVERIFY(!sameStore.save());                             // an explicit database rule runs in save()
    VProduct otherStore; otherStore.store_id = 2; otherStore.sku = "box-1"; otherStore.price = 1;
    QVERIFY(otherStore.validate().isValid());

    VBooking b;
    b.room_id = 99;
    b.starts_at = QDateTime(QDate(2030, 1, 7), QTime(9, 0));
    b.ends_at = QDateTime(QDate(2030, 1, 7), QTime(10, 0));
    QCOMPARE(b.validate().error("room_id"), QString("doesn't exist"));
}

void ValidationTests::overlappingBookings()
{
    VRoom room; room.name = "Blue"; QVERIFY(room.save());
    VRoom other; other.name = "Red"; QVERIFY(other.save());
    const QDate day(2030, 1, 7);
    auto booking = [&](int roomId, QTime from, QTime to) {
        VBooking b;
        b.room_id = roomId;
        b.starts_at = QDateTime(day, from);
        b.ends_at = QDateTime(day, to);
        return b;
    };
    VBooking nine = booking(room.id().toInt(), QTime(9, 0), QTime(10, 0));
    QVERIFY2(nine.save(), qPrintable(nine.lastError().text()));

    VBooking clash = booking(room.id().toInt(), QTime(9, 30), QTime(10, 30));
    QCOMPARE(clash.validate().error("ends_at"), QString("is booked by someone else"));
    QVERIFY(!clash.save());

    VBooking backToBack = booking(room.id().toInt(), QTime(10, 0), QTime(11, 0));   // touching isn't overlapping
    QVERIFY2(backToBack.validate().isValid(), qPrintable(backToBack.validation().summary()));
    VBooking elsewhere = booking(other.id().toInt(), QTime(9, 30), QTime(10, 30));  // another room
    QVERIFY(elsewhere.validate().isValid());

    // Moving a booking within its own slot isn't a clash with itself.
    nine.ends_at = QDateTime(day, QTime(9, 45));
    QVERIFY2(nine.validate().isValid(), qPrintable(nine.validation().summary()));

    // Times on the hour grid and in opening hours, and a sane length.
    VBooking odd = booking(room.id().toInt(), QTime(7, 10), QTime(7, 5));
    const QiValidation v = odd.validate();
    QCOMPARE(v.error("starts_at"), QString("must be on a 15-minute mark"));
    QVERIFY(v.errorsFor("starts_at").contains("must be between 08:00 and 20:00"));
    QCOMPARE(v.error("ends_at"), QString("must be after Starts at"));
    VBooking marathon = booking(room.id().toInt(), QTime(8, 0), QTime(13, 0));
    QCOMPARE(marathon.validate().error("ends_at"), QString("must be 15 to 240 minutes after Starts at"));
}

void ValidationTests::dates()
{
    VUser u = goodUser("dates@example.com");
    u.born = QDate::currentDate().addDays(1);
    QCOMPARE(u.validate().error("born"), QString("must be in the past"));
    u.born = QDate::currentDate().addYears(-12);
    QCOMPARE(u.validate().error("born"), QString("must be at least 13 years ago"));
    u.born = QDate::currentDate().addYears(-13);
    QVERIFY(!u.validate().hasError("born"));                // the 13th birthday counts

    // Strings that aren't dates.
    VStay s;
    s.check_in = QStringLiteral("2026-02-30");
    QCOMPARE(s.validate().error("check_in"), QString("is not a valid date"));

    VStay stay;
    stay.check_in = QDate::currentDate().addDays(3);
    stay.check_out = QDate::currentDate().addDays(3);
    QCOMPARE(stay.validate().error("check_out"), QString("must be 1 to 30 nights after check-in"));
    stay.check_out = QDate::currentDate().addDays(5);
    QVERIFY2(stay.validate().isValid(), qPrintable(stay.validation().summary()));
    stay.check_in = QDate::currentDate().addDays(-1);
    QCOMPARE(stay.validate().error("check_in"), QString("can't be in the past"));
    stay.check_in = QDate::currentDate().addYears(2);
    QVERIFY(stay.validate().error("check_in").startsWith("must be on or before "));

    QVERIFY(qiDateTimeOf("2026-10-03 14:30").isValid());     // a space instead of T
    QCOMPARE(qiDateTimeOf("2026-10-03").date(), QDate(2026, 10, 3));
    QVERIFY(!qiDateTimeOf("2026-13-01").isValid());
}

void ValidationTests::relativeDates()
{
    const QDate today = QDate::currentDate();
    QCOMPARE(qiResolveDate("today").date(), today);
    QCOMPARE(qiResolveDate("today+30d").date(), today.addDays(30));
    QCOMPARE(qiResolveDate("-18y").date(), today.addYears(-18));
    QCOMPARE(qiResolveDate("tomorrow").date(), today.addDays(1));
    QCOMPARE(qiResolveDate("today + 1m - 1d").date(), today.addMonths(1).addDays(-1));
    QCOMPARE(qiResolveDate("+2w").date(), today.addDays(14));
    const QDateTime soon = qiResolveDate("now+2h");
    QVERIFY(qAbs(QDateTime::currentDateTime().secsTo(soon) - 7200) < 5);
    QCOMPARE(qiResolveDate(QDate(2026, 1, 2)).date(), QDate(2026, 1, 2));
    QCOMPARE(qiResolveDate("2026-01-02").date(), QDate(2026, 1, 2));
}

void ValidationTests::calendarRules()
{
    VStay s;
    s.meeting = QDate(2030, 12, 25);                         // a Wednesday, and a holiday
    QCOMPARE(s.validate().error("meeting"), QString("must be a business day"));
    s.meeting = QDate(2030, 12, 28);                         // Saturday
    QVERIFY(s.validate().hasError("meeting"));
    s.meeting = QDate(2030, 12, 27);                         // Friday
    QVERIFY(!s.validate().hasError("meeting"));

    s.weekend_trip = QDate(2030, 12, 27);
    QCOMPARE(s.validate().error("weekend_trip"), QString("must be on a weekend"));
    s.weekend_trip = QDate(2030, 12, 29);
    QVERIFY(!s.validate().hasError("weekend_trip"));
}

void ValidationTests::expiryAndZones()
{
    VStay s;
    s.card = QStringLiteral("4111 1111 1111 1111");
    QVERIFY(!s.validate().hasError("card"));
    s.card = QStringLiteral("4111 1111 1111 1112");
    QCOMPARE(s.validate().error("card"), QString("is not a valid card number"));

    const QDate now = QDate::currentDate();
    s.card_expiry = QStringLiteral("%1/%2").arg(now.month(), 2, 10, QChar('0')).arg(now.year() % 100, 2, 10, QChar('0'));
    QVERIFY2(!s.validate().hasError("card_expiry"), qPrintable(s.validation().summary()));   // this month: still valid
    const QDate last = now.addMonths(-1);
    s.card_expiry = QStringLiteral("%1/%2").arg(last.month(), 2, 10, QChar('0')).arg(last.year());
    QCOMPARE(s.validate().error("card_expiry"), QString("has expired"));
    s.card_expiry = QStringLiteral("13/30");
    QCOMPARE(s.validate().error("card_expiry"), QString("is not a valid expiry date"));

    if (!QTimeZone("Europe/Berlin").isValid())
        QSKIP("no time zone data");
    // Clocks in Berlin went from 02:00 to 03:00 on 29 March 2026: 02:30 never happened.
    s.opens = QStringLiteral("2026-03-29 02:30");
    QCOMPARE(s.validate().error("opens"), QString("doesn't exist in Europe/Berlin: the clocks change then"));
    s.opens = QStringLiteral("2026-03-29 03:30");
    QVERIFY(!s.validate().hasError("opens"));

    // "today" in another zone.
    QiRule r = QiTodayOrLater().inZone("Pacific/Kiritimati");   // UTC+14: often already tomorrow
    QCOMPARE(r.params.value("zone").toString(), QString("Pacific/Kiritimati"));
}

void ValidationTests::lists()
{
    QList<VProduct *> lines;
    for (int i = 0; i < 3; ++i) {
        auto *p = new VProduct;
        p->store_id = 3;
        p->sku = QStringLiteral("line-%1").arg(i);
        p->price = i == 1 ? -1 : 2;
        lines << p;
    }
    const QiValidation v = qiValidateAll(lines, "lines");
    QVERIFY(!v.isValid());
    QCOMPARE(v.fields(), QStringList({ "lines[1].price" }));
    QVERIFY(v.summary().contains("lines[1] Price: must be more than 0"));

    QiValidation order;
    order.addError("customer", "is required");
    order.merge(v);
    QCOMPARE(order.errors().size(), 2);
    order.clear("lines");
    QCOMPARE(order.fields(), QStringList({ "customer" }));
    qDeleteAll(lines);
}

void ValidationTests::humanize()
{
    QCOMPARE(qiHumanize("first_name"), QString("First name"));
    QCOMPARE(qiHumanize("startsAt"), QString("Starts at"));
    QCOMPARE(qiHumanize("author_id"), QString("Author"));
    QCOMPARE(qiHumanize("email"), QString("Email"));
}
