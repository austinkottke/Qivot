/** iOS-style Contacts, backed by Qivot.

    - Alphabetical sticky sections + an A–Z scrubber on the right (drag to jump).
    - Live search that filters as you type.
    - "+" adds a contact that appears in the correct section instantly (reactive).

    The list is windowed: 10,000 rows are counted up front, but only the pages
    you scroll to are fetched (LIMIT/OFFSET) — set QIVOT_LOG=1 to watch the SQL.

    QIVOT_SELFTEST=1 drives search/add/jump then quits (for headless checks).
 */
#include "contact.h"
#include "contactstore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QStringList>
#include <QTimer>

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    if (qEnvironmentVariableIsSet("QIVOT_LOG"))
        QiLog::enableAll();                       // print every SQL statement

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("contacts.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Contact>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

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

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        ContactStore *store = root->findChild<ContactStore *>();
        QTimer::singleShot(300, &app, [store]{
            if (!store) return;
            store->add("Zoe", "Zephyr", "(555) 010-2020");    // reactive add
            store->setFilter("smith");                          // live search
            store->indexForLetter("S");                         // jump index
        });
        QTimer::singleShot(900, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
