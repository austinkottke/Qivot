/** iOS-style Contacts, backed by Qivot.

    - Alphabetical sticky sections + an A–Z scrubber on the right (drag to jump).
    - Live search that filters as you type.
    - "+" adds a contact; Edit renames one. Only that row changes: it slides
      in, moves, or refreshes, and the rest of the list stays put.
    - "Sync" makes changes on screen the way another device would.

    The list is live (QiLiveListModel): the 10,000 ids are read once, records a
    batch at a time as you scroll, and each change costs one small query —
    set QIVOT_LOG=1 to watch the SQL.

    QIVOT_SELFTEST=1 checks that a rename, an add and a delete each reach the
    view as one row signal, then quits (for headless checks).
    QIVOT_SHOTS=<dir> walks through an edit and a sync and saves screenshots.
 */
#include "contact.h"
#include "contactstore.h"
#include "safearea.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QMap>
#include <cstdio>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    if (qEnvironmentVariableIsSet("QIVOT_LOG"))
        QiLog::enableAll();                       // print every SQL statement

    // The screens draw their own controls, so every platform gets the same
    // look rather than the host style's (Material on Android, iOS on iOS).
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QQuickStyle::setStyle("Basic");
#else
    QQuickStyle::setStyle("Default");
#endif

    // A phone app starts in "/", which it can't write to; keep the database in
    // the app's own data directory there. On desktop it sits next to the binary.
    QString dbPath = "contacts.db";
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    dbPath = dataDir + "/contacts.db";
#endif

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    db.open();

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Contact>();

    // Contacts persist between launches, so adds and deletes survive a restart.
    // Seed only a fresh database — or when QIVOT_SEED asks for a new set.
    const bool reseed = qEnvironmentVariableIsSet("QIVOT_SEED");
    if (reseed && !connection.dropTables()) return 1;
    if (!connection.createTables()) return 1;
    const bool needSeed = reseed || Contact::objects().count() == 0;

    // --- Seed a realistic, alphabet-spanning address book ---
    // 100 first names x 100 surnames, enumerated as every distinct pair,
    // so all 10,000 contacts have a unique full name.
    const QStringList firsts = {
        "James","Mary","John","Patricia","Robert","Jennifer","Michael","Linda",
        "William","Elizabeth","David","Barbara","Richard","Susan","Joseph","Jessica",
        "Thomas","Sarah","Charles","Karen","Christopher","Nancy","Daniel","Lisa",
        "Matthew","Betty","Anthony","Margaret","Mark","Sandra","Donald","Ashley",
        "Steven","Kimberly","Paul","Emily","Andrew","Donna","Joshua","Michelle",
        "Kenneth","Carol","Kevin","Amanda","Brian","Dorothy","George","Melissa",
        "Timothy","Deborah","Ronald","Stephanie","Edward","Rebecca","Jason","Sharon",
        "Jeffrey","Laura","Ryan","Cynthia","Jacob","Kathleen","Gary","Amy",
        "Nicholas","Angela","Eric","Shirley","Jonathan","Anna","Stephen","Brenda",
        "Larry","Pamela","Justin","Emma","Scott","Nicole","Brandon","Helen",
        "Benjamin","Samantha","Samuel","Katherine","Gregory","Christine","Alexander","Debra",
        "Patrick","Rachel","Frank","Carolyn","Raymond","Janet","Jack","Maria",
        "Dennis","Olivia","Jerry","Grace" };
    const QStringList lasts = {
        "Smith","Johnson","Williams","Brown","Jones","Garcia","Miller","Davis",
        "Rodriguez","Martinez","Hernandez","Lopez","Gonzalez","Wilson","Anderson","Thomas",
        "Taylor","Moore","Jackson","Martin","Lee","Perez","Thompson","White",
        "Harris","Sanchez","Clark","Ramirez","Lewis","Robinson","Walker","Young",
        "Allen","King","Wright","Scott","Torres","Nguyen","Hill","Flores",
        "Green","Adams","Nelson","Baker","Hall","Rivera","Campbell","Mitchell",
        "Carter","Roberts","Gomez","Phillips","Evans","Turner","Diaz","Parker",
        "Cruz","Edwards","Collins","Reyes","Stewart","Morris","Morales","Murphy",
        "Cook","Rogers","Gutierrez","Ortiz","Morgan","Cooper","Peterson","Bailey",
        "Reed","Kelly","Howard","Ramos","Kim","Cox","Ward","Richardson",
        "Watson","Brooks","Chavez","Wood","James","Bennett","Gray","Mendoza",
        "Ruiz","Hughes","Price","Alvarez","Castillo","Sanders","Patel","Myers",
        "Long","Ross","Foster","Jimenez" };

    if (needSeed) {
        QiList<Contact> seed;
        QiListWriter writer(&seed);
        // Every (first, last) pair is distinct, so the default 10,000 are all unique.
        // Override with QIVOT_SEED=<n> (n <= firsts.size()*lasts.size()) for a smaller set.
        const int maxUnique = firsts.size() * lasts.size();
        int N = qEnvironmentVariableIsSet("QIVOT_SEED")
                    ? qEnvironmentVariableIntValue("QIVOT_SEED")
                    : 10000;
        if (N > maxUnique) N = maxUnique;
        for (int i = 0; i < N; i++) {
            const QString first = firsts.at(i % firsts.size());   // enumerate every
            const QString last  = lasts.at (i / firsts.size());   // distinct pair
            const QString phone = QString("(%1) %2-%3")
                                      .arg(200 + (i * 3) % 700, 3, 10, QChar('0'))
                                      .arg(100 + (i * 17) % 900, 3, 10, QChar('0'))
                                      .arg(1000 + (i * 29) % 9000, 4, 10, QChar('0'));
            writer << first << last << phone << writer.next();
        }
        seed.save();
    }

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

#ifdef Q_OS_IOS
    // Draw under the status bar (instead of leaving a black band above the app)
    // and pad the screens by what the status bar / home indicator cover. UIKit
    // only knows the insets once the window is on screen, so read them after.
    if (auto *w = qobject_cast<QQuickWindow *>(engine.rootObjects().first())) {
        w->setFlag(Qt::MaximizeUsingFullscreenGeometryHint, true);
        w->showMaximized();
        auto apply = [w] {
            const QMargins m = iosSafeAreaMargins();
            w->setProperty("safeTop",    m.top());
            w->setProperty("safeBottom", m.bottom());
        };
        for (int ms : {0, 150, 600})             // settle once the window is laid out
            QTimer::singleShot(ms, w, apply);
    }
#endif

    QObject *root = engine.rootObjects().first();
    ContactStore *store = root->findChild<ContactStore *>();

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST") && store) {
        // Each change must reach the view as one row signal, never a reset.
        auto *model = store->contacts();
        auto *counts = new QMap<QString, int>();
        QObject::connect(model, &QAbstractItemModel::modelReset, [counts] { (*counts)["reset"]++; });
        QObject::connect(model, &QAbstractItemModel::rowsInserted, [counts] { (*counts)["inserted"]++; });
        QObject::connect(model, &QAbstractItemModel::rowsRemoved, [counts] { (*counts)["removed"]++; });
        QObject::connect(model, &QAbstractItemModel::rowsMoved, [counts] { (*counts)["moved"]++; });
        auto *failures = new int(0);
        auto check = [failures](bool ok, const QString &what) {
            std::printf("  %s  %s\n", ok ? "PASS" : "FAIL", qPrintable(what));
            if (!ok) ++*failures;
        };
        QTimer::singleShot(300, &app, [store, counts, check] {
            const QVariantMap first = store->contactAt(1);
            store->update(first.value("id").toInt(), first.value("firstName").toString(),
                          "Zimmerman", first.value("phone").toString());
            QTimer::singleShot(50, [store, counts, check, first] {
                check((*counts)["moved"] == 1 && (*counts)["reset"] == 0,
                      "a rename moves one row, without a reset");
                check(store->rowOf(first.value("id").toInt()) == store->count() - 1,
                      "and it lands at the end, in Z");
                check(store->activity().contains("Moved row 2"), "activity: " + store->activity().replace('\n', " | "));
                store->add("Zoe", "Aardvark", "(555) 010-2020");
            });
            QTimer::singleShot(100, [store, counts, check] {
                check((*counts)["inserted"] == 1 && store->rowOf(store->contactAt(0).value("id").toInt()) == 0
                      && store->contactAt(0).value("lastName") == "Aardvark", "a new contact is inserted at row 1");
                store->remove(store->contactAt(0).value("id").toInt());
            });
            QTimer::singleShot(150, [store, counts, check] {
                check((*counts)["removed"] == 1, "a deletion removes one row");
                check(store->activity().contains("no query"), "activity: " + store->activity().replace('\n', " | "));
                store->setFilter("smith");
                check(store->count() == 100, QString("search: %1 Smiths").arg(store->count()));
                store->indexForLetter("S");
                check((*counts)["reset"] == 1, "only the new search reset the list");
            });
        });
        QTimer::singleShot(900, &app, [failures] {
            std::printf("%s\n", *failures ? "SELFTEST FAILED" : "SELFTEST PASSED");
            QCoreApplication::exit(*failures ? 1 : 0);
        });
    }

    // QIVOT_SHOTS=<dir>: walk through an edit and a sync, saving screenshots.
    // Run offscreen at 2x:  QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 QIVOT_SEED=10000 QIVOT_SHOTS=. ./contacts
    const QString shots = qEnvironmentVariable("QIVOT_SHOTS");
    if (!shots.isEmpty() && store) {
        auto *window = qobject_cast<QQuickWindow *>(root);
        auto grab = [window, shots, store](const QString &name) {
            window->grabWindow().save(QDir(shots).filePath(name + ".png"));
            std::printf("saved %s.png  (%s)\n", qPrintable(name), qPrintable(store->activity().replace('\n', " | ")));
        };
        auto call = [root](const char *fn, QVariant arg = QVariant()) {
            if (arg.isValid())
                QMetaObject::invokeMethod(root, fn, Q_ARG(QVariant, arg));
            else
                QMetaObject::invokeMethod(root, fn);
        };
        auto sheet = root->findChild<QObject *>("contactSheet");
        QTimer::singleShot(450,  &app, [grab] { grab("contacts"); });
        QTimer::singleShot(500,  &app, [call] { call("openRow", 1); });
        QTimer::singleShot(1000, &app, [call] { call("editOpenCard"); });
        QTimer::singleShot(1400, &app, [sheet] {
            QMetaObject::invokeMethod(sheet, "fill", Q_ARG(QVariant, "Patricia"),
                                      Q_ARG(QVariant, "Young"), Q_ARG(QVariant, "(415) 555-0142"));
        });
        QTimer::singleShot(1700, &app, [grab] { grab("contacts-edit"); });
        QTimer::singleShot(1800, &app, [sheet, call] {
            QMetaObject::invokeMethod(sheet, "save");
            call("goBack");                      // back to the list, which shows where it went
        });
        QTimer::singleShot(2500, &app, [grab] { grab("contacts-moved"); });
        QTimer::singleShot(5200, &app, [call] { call("scrollTo", 0); });
        QTimer::singleShot(5600, &app, [store, root] {
            root->setProperty("syncing", true);  // its first random change comes 1.4 s later
            store->simulate("relative", 1);      // a relative of Amanda's: right on screen
        });
        QTimer::singleShot(6500, &app, [grab] { grab("contacts-sync"); });
        QTimer::singleShot(6700, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
