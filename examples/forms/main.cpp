/** Forms with Qivot: validation declared once on the models (models.h), shown
    live in QML by QiForm and the Qivot.Forms controls.

    Five pages: a sign-up, a checkout with its order lines, room booking (dates,
    opening hours, overlaps), a three-step wizard, and an editable inventory table.

    QIVOT_SELFTEST=1 runs the forms headlessly (QT_QPA_PLATFORM=offscreen) and
    exits non-zero if a check fails or QML warns.
 */
#include "models.h"
#include <qiform.h>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QTimer>
#include <QEventLoop>
#include <QThread>
#include <QQuickWindow>

/// What the pages need that isn't a form: lists to choose from, and the bookings so far.
class Demo : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE QStringList rooms() const {
        QStringList out;
        const QiList<Room> list = qiRawQuery<Room>("SELECT * FROM room ORDER BY name");
        for (int i = 0; i < list.size(); ++i)
            out << list.at(i)->name.get().toString();
        return out;
    }
    Q_INVOKABLE QVariantList bookings(const QString &room, int revision = 0) const {
        Q_UNUSED(revision)        // QML passes a counter so the list refreshes after a booking
        QVariantList out;
        const QiList<Booking> list = qiRawQuery<Booking>("SELECT * FROM booking WHERE room = ? ORDER BY starts_at", { room });
        for (int i = 0; i < list.size(); ++i) {
            const Booking *b = list.at(i);
            out << QVariantMap{ { "organiser", b->organiser.get() },
                                { "when", b->starts_at.get().toDateTime().toString("ddd d MMM, HH:mm") + " – "
                                              + b->ends_at.get().toDateTime().toString("HH:mm") } };
        }
        return out;
    }
    Q_INVOKABLE QVariantList products() const {
        QVariantList out;
        const QiList<Product> list = qiRawQuery<Product>("SELECT * FROM product ORDER BY id");
        for (int i = 0; i < list.size(); ++i)
            out << list.at(i)->id.get();
        return out;
    }
    /// The next weekday at `hour`:00, as the date-time fields write it.
    Q_INVOKABLE QString nextWeekday(int hour) const {
        QDate d = QDate::currentDate().addDays(1);
        while (d.dayOfWeek() > 5) d = d.addDays(1);
        return QDateTime(d, QTime(hour, 0)).toString("yyyy-MM-dd HH:mm");
    }
    Q_INVOKABLE QString inDays(int days) const {
        return QDate::currentDate().addDays(days).toString(Qt::ISODate);
    }
};

static int g_qmlWarnings = 0;
static QtMessageHandler g_previous = nullptr;
static void countWarnings(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    if (type >= QtWarningMsg && (message.contains(".qml:") || qstrcmp(context.category, "qml") == 0))
        ++g_qmlWarnings;
    g_previous(type, context, message);
}

static void seed() {
    for (const auto &r : { qMakePair(QStringLiteral("Atlas"), 12), qMakePair(QStringLiteral("Bow"), 6), qMakePair(QStringLiteral("Cedar"), 20) }) {
        Room room; room.name = r.first; room.seats = r.second; (void) room.save();
    }
    Member taken;
    taken.email = "taken@example.com"; taken.name = "Ada Lovelace"; taken.username = "ada";
    taken.password = "Analytic1"; taken.password_confirm = "Analytic1"; taken.born = QDate(1990, 12, 10);
    taken.plan = "Free"; taken.terms = true;
    if (!taken.save()) qWarning() << taken.lastError().text();

    QDate d = QDate::currentDate().addDays(1);
    while (d.dayOfWeek() > 5) d = d.addDays(1);
    Booking b; b.room = "Atlas"; b.organiser = "Grace Hopper"; b.people = 6;
    b.starts_at = QDateTime(d, QTime(10, 0)); b.ends_at = QDateTime(d, QTime(11, 0));
    if (!b.save()) qWarning() << b.lastError().text();

    const struct { const char *sku, *name; double price; int stock; } items[] = {
        { "MUG-01", "Enamel mug", 12.5, 40 }, { "TEE-M", "T-shirt, medium", 24, 12 },
        { "BAG-02", "Canvas tote", 18.99, 0 }, { "PEN-10", "Pens, pack of 10", 6.75, 120 },
    };
    for (const auto &i : items) {
        Product p; p.sku = i.sku; p.name = i.name; p.price = i.price; p.stock = i.stock; p.launched = QDate(2021, 3, 1);
        if (!p.save()) qWarning() << p.lastError().text();
    }
}

// The self-test: drive the real forms and check what they say.
static int selfTest(QObject *root) {
    int failures = 0;
    auto expect = [&](bool ok, const QString &what) {
        qInfo().noquote() << (ok ? "  PASS  " : "  FAIL  ") + what;
        if (!ok) ++failures;
    };
    auto form = [&](const char *name) { return root->findChild<QiForm *>(name); };

    QiForm *signup = form("signupForm");
    expect(signup, "the sign-up form is there");
    if (signup) {
        signup->set("email", "  TAKEN@example.com ");
        signup->touch("email");
        expect(signup->errors().value("email").toString() == "is already taken", "sign-up: a taken email, once the field is left");
        signup->set("email", "new@example.com");
        signup->touch("email");
        expect(!signup->errors().contains("email"), "sign-up: a free email");
        signup->set("password", "short");
        signup->touch("password");
        expect(signup->errors().value("password").toString().startsWith("must be at least 8 characters"), "sign-up: a weak password");
        signup->set("plan", "Team");
        expect(!signup->submit(), "sign-up: submit with gaps fails");
        expect(signup->errors().value("team_name").toString() == "is required", "sign-up: a team needs a name");
        expect(signup->errors().value("terms").toString() == "must be accepted", "sign-up: the terms");
        // The attached QiForm.field: a plain TextField writes the form, and the form writes it.
        QObject *usernameField = root->findChild<QObject *>("usernameField");
        expect(usernameField, "sign-up: the TextField wired with QiForm.field");
        if (usernameField) {
            usernameField->setProperty("text", "typed_in_qml");
            expect(signup->values().value("username").toString() == "typed_in_qml", "sign-up: TextField -> form");
            signup->set("username", "from_the_form");
            expect(usernameField->property("text").toString() == "from_the_form", "sign-up: form -> TextField");
        }
        // A JavaScript validator from the page (validators: { password: … }).
        signup->set("username", "orbit");
        signup->set("password", "Orbit2026x");
        signup->touch("password");
        expect(signup->errors().value("password").toString() == "can't contain your username", "sign-up: a validator written in QML");
        signup->set("name", "Ada Lovelace");
        signup->set("username", "ada_lovelace");
        signup->touch("username");
        expect(signup->warnings().value("username").toString().startsWith("is just your name"), "sign-up: a QML validator's warning");

        signup->set("name", "  Katherine   Johnson ");
        signup->set("username", "kj");
        expect(!signup->submit() && signup->errors().value("username").toString() == "3 to 20 of a-z, 0-9 and _", "sign-up: a short username");
        signup->set("username", "Katherine_J");
        signup->set("password", "Orbital9x");
        signup->set("password_confirm", "Orbital9x");
        signup->set("born", "1918-08-26");
        signup->set("team_name", "Flight research");
        signup->set("terms", true);
        expect(signup->submit(), "sign-up: submit when it's all right — " + signup->summary());
        expect(signup->values().value("name").toString() == "Katherine Johnson", "sign-up: saved as cleaned up");
        expect(signup->values().value("username").toString() == "katherine_j", "sign-up: username lower-cased");
    }

    QiForm *booking = form("bookingForm");
    expect(booking, "the booking form is there");
    if (booking) {
        QDate d = QDate::currentDate().addDays(1);
        while (d.dayOfWeek() > 5) d = d.addDays(1);
        const QString day = d.toString(Qt::ISODate);
        booking->set("room", "Atlas");
        booking->set("organiser", "Mary Jackson");
        booking->set("people", "4");
        booking->set("starts_at", day + " 10:30");
        booking->set("ends_at", day + " 11:30");
        booking->touch("ends_at");
        expect(booking->errors().value("ends_at").toString() == "overlaps another booking of this room", "booking: overlaps Grace's 10:00-11:00");
        booking->set("starts_at", day + " 11:00");
        booking->set("ends_at", day + " 11:10");
        booking->touch("ends_at");
        expect(booking->errors().value("ends_at").toString() == "a booking is 30 minutes to 4 hours", "booking: too short");
        booking->set("ends_at", day + " 12:00");
        booking->touch("ends_at");
        expect(!booking->errors().contains("ends_at"), "booking: right after Grace's is fine");
        booking->set("starts_at", day + " 07:07");
        booking->touch("starts_at");
        expect(booking->allErrors().value("starts_at").toString() == "we're open 08:00 to 20:00", "booking: before opening");
        booking->set("people", "abc");
        booking->touch("people");
        expect(booking->errors().value("people").toString() == "must be a whole number", "booking: people isn't a number");
        booking->set("people", "16");
        booking->touch("people");
        expect(booking->warnings().value("people").toString() == "more than the small rooms seat", "booking: a warning, not an error");
        booking->set("starts_at", day + " 11:00");
        expect(booking->submit(), "booking: saves — " + booking->summary());
    }

    QiForm *checkout = form("checkoutForm");
    expect(checkout, "the checkout form is there");
    if (checkout) {
        checkout->set("billing_same", false);
        checkout->set("card_number", "4111 1111 1111 1112");
        checkout->touch("card_number");
        expect(checkout->errors().value("card_number").toString() == "is not a valid card number", "checkout: a mistyped card");
        checkout->set("card_expiry", "01/20");
        checkout->touch("card_expiry");
        expect(checkout->errors().value("card_expiry").toString() == "has expired", "checkout: an old card");
        checkout->set("deliver_on", QDate::currentDate().addDays(1).toString(Qt::ISODate));
        checkout->touch("deliver_on");
        expect(checkout->errors().value("deliver_on").toString() == "is too soon: we need 2 days", "checkout: delivery too soon");
        expect(!checkout->submit() && checkout->errors().value("bill_address").toString() == "is required", "checkout: billing address when it differs");
        checkout->set("billing_same", true);
        checkout->validate();
        expect(!checkout->errors().contains("bill_address"), "checkout: not when it's the same");
    }

    QiForm *wizard = form("wizardForm");
    expect(wizard, "the wizard form is there");
    if (wizard) {
        wizard->set("first_name", "Dorothy");
        wizard->set("last_name", "Vaughan");
        wizard->set("email", "dorothy@example.com");
        wizard->set("born", QDate::currentDate().addYears(-17).toString(Qt::ISODate));
        expect(!wizard->validateFields({ "first_name", "last_name", "email", "born" }), "wizard: step 1 stops at under 18");
        expect(wizard->errors().value("born").toString() == "you must be 18 or over to apply", "wizard: says why");
        expect(!wizard->errors().contains("country"), "wizard: step 2 isn't checked yet");
        wizard->set("born", "1910-09-20");
        expect(wizard->validateFields({ "first_name", "last_name", "email", "born" }), "wizard: step 1 passes");
        wizard->set("country", "United States");
        expect(!wizard->validateFields({ "country", "state", "postcode", "phone" }) && wizard->errors().value("state").toString() == "is required",
               "wizard: a US address needs a state");
    }

    QObject *inventory = root->findChild<QObject *>("inventoryList");
    expect(inventory && inventory->property("count").toInt() == 4, "inventory: four rows, a form each");

    expect(g_qmlWarnings == 0, QStringLiteral("no QML warnings (%1)").arg(g_qmlWarnings));
    qInfo().noquote() << (failures ? QStringLiteral("%1 FAILED").arg(failures) : QStringLiteral("ALL PASSED"));
    return failures ? 1 : 0;
}

int main(int argc, char *argv[]) {
    g_previous = qInstallMessageHandler(countWarnings);
    QGuiApplication app(argc, argv);
    // The Qivot.Forms controls draw themselves; the plain style keeps the platform out of it.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickStyle::setStyle(QStringLiteral("Basic"));
#else
    QQuickStyle::setStyle(QStringLiteral("Default"));
#endif

    // A fresh database each run.
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("qivot-forms.db");
    QFile::remove(path);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) return 1;
    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Member>();
    connection.addModel<Purchase>();
    connection.addModel<OrderLine>();
    connection.addModel<Room>();
    connection.addModel<Booking>();
    connection.addModel<Applicant>();
    connection.addModel<Product>();
    if (!connection.createTables()) return 1;
    seed();

    Demo demo;                                    // outlives the engine (its bindings use it)
    QQmlApplicationEngine engine;
    qiRegisterQml(&engine);                       // QiForm, and the Qivot.Forms controls
    engine.rootContext()->setContextProperty("demo", &demo);
    engine.rootContext()->setContextProperty("startTheme", qEnvironmentVariable("QIVOT_THEME"));   // light / dark
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    // QIVOT_SHOTS=<folder>: a picture of each page (after a submit, so its errors show).
    const QString shots = qEnvironmentVariable("QIVOT_SHOTS");
    if (!shots.isEmpty()) {
        QTimer::singleShot(400, [&]() {
            QObject *root = engine.rootObjects().first();
            QObject *tabs = root->findChild<QObject *>("tabs");
            auto *window = qobject_cast<QQuickWindow *>(root);
            window->resize(1000, qEnvironmentVariableIntValue("QIVOT_SHOT_HEIGHT") > 0 ? qEnvironmentVariableIntValue("QIVOT_SHOT_HEIGHT") : 1500);
            auto wait = [](int ms) { QEventLoop l; QTimer::singleShot(ms, &l, &QEventLoop::quit); l.exec(); };
            const char *names[] = { "signup", "checkout", "booking", "wizard", "inventory" };
            for (QiForm *f : root->findChildren<QiForm *>()) {
                if (f->objectName() == "signupForm") { f->set("email", "taken@example.com"); f->set("name", "Ada"); f->set("password", "Analytic1"); f->set("plan", "Team"); f->submit(); }
                if (f->objectName() == "checkoutForm") { f->set("card_number", "4111 1111 1111 1112"); f->set("card_expiry", "01/20"); f->submit(); }
                if (f->objectName() == "bookingForm") { f->set("organiser", "Mary Jackson"); f->touch("organiser"); }
            }
            wait(300);
            const QString suffix = qEnvironmentVariable("QIVOT_THEME");
            for (int i = 0; i < 5; ++i) {
                tabs->setProperty("currentIndex", i);
                wait(450);
                window->grabWindow().save(QDir(shots).filePath(QStringLiteral("%1-%2%3.png").arg(i).arg(names[i])
                                                                .arg(suffix.isEmpty() ? QString() : QLatin1Char('-') + suffix)));
            }
            app.exit(0);
        });
    }

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QTimer::singleShot(200, [&]() { app.exit(selfTest(engine.rootObjects().first())); });
    }
    return app.exec();
}

#include "main.moc"
