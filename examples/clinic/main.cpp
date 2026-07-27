/** Qivot Clinic — a basic EHR: appointment scheduler + patient chart.

    A two-view clinical app over a small relational schema (patients, providers,
    appointments, vitals, problems, medications, notes). All data is SYNTHETIC,
    generated at startup — no real patients.

    Showcases: relations/joins across seven models, a transaction that refuses to
    double-book a provider, filtered patient search, and rich dashboard queries —
    all served to QML as plain QVariant maps/lists.

    QIVOT_SELFTEST=1 seeds, prints row counts + today's schedule size, and quits.
 */
#include "models.h"
#include "clinicstore.h"
#include "theme.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>            // qmlRegisterUncreatableMetaObject
#include <QQmlContext>
#include <QSqlDatabase>
#include <QDate>
#include <QTime>
#include <QVector>
#include <QDebug>

// ---------- synthetic data pools ----------
static const QStringList kFirst = {
    "Emma","Liam","Olivia","Noah","Ava","Ethan","Sophia","Mason","Isabella","Lucas",
    "Mia","Amir","Zoe","Chen","Aisha","Diego","Nina","Omar","Priya","Marcus",
    "Elena","Kwame","Yuki","Rosa","Ivan","Leila","Sam","Grace","Hassan","Maya",
    "Theo","Farah","Jonah","Anaya","Cole","Vera" };
static const QStringList kLast = {
    "Nguyen","Patel","Johnson","Garcia","Kim","Okafor","Silva","Cohen","Rossi","Haddad",
    "Andersson","Mbeki","Yamamoto","Reyes","Novak","Fischer","Costa","Ali","Petrov","Duarte",
    "Larsson","Osei","Tanaka","Flores","Volkov","Nasser","Brooks","Bennett","Ahmed"," Porter",
    "Weber","Karimi","Foster","Sharma","Doyle","Vega" };
static const QStringList kSex = { "F","M" };
static const QStringList kBlood = { "O+","A+","B+","AB+","O-","A-","B-" };
static const QStringList kAllergy = { "","","","","Penicillin","Peanuts","Latex","Sulfa drugs","Bee stings" };
static const QStringList kProblem = {
    "Hypertension","Type 2 Diabetes","Asthma","Hyperlipidemia","GERD","Osteoarthritis",
    "Anxiety","Migraine","Hypothyroidism","Seasonal allergies","Lower back pain","Eczema" };
struct MedDef { const char *name, *dose, *freq; };
static const QVector<MedDef> kMed = {
    {"Lisinopril","10 mg","once daily"}, {"Metformin","500 mg","twice daily"},
    {"Atorvastatin","20 mg","once daily"}, {"Albuterol HFA","90 mcg","as needed"},
    {"Omeprazole","20 mg","once daily"}, {"Levothyroxine","50 mcg","once daily"},
    {"Sertraline","50 mg","once daily"}, {"Amlodipine","5 mg","once daily"} };
static const QStringList kReason = {
    "Annual physical","Follow-up","New patient visit","Blood pressure check","Medication refill",
    "Lab review","Consult","Sick visit","Wellness check","Post-op follow-up" };
static const QStringList kNoteBody = {
    "Patient reports feeling well. Blood pressure controlled on current medication. Continue lisinopril and recheck in three months.",
    "Discussed diabetes management and diet. A1C improved since the last visit. Ordered fasting labs and a lipid panel.",
    "Follow-up for asthma. Using albuterol inhaler as needed, symptoms stable, no nighttime cough reported.",
    "Reviewed cholesterol results with patient; LDL mildly elevated. Started atorvastatin and advised lifestyle changes.",
    "Annual physical exam completed. All systems reviewed, no acute concerns. Immunizations up to date.",
    "Patient presents with lower back pain after lifting. Exam benign; advised rest, ibuprofen, and a physical therapy referral.",
    "Medication refill for hypothyroidism. Levothyroxine dose unchanged, TSH within range. Recheck in six months.",
    "Telehealth visit for seasonal allergies. Prescribed an antihistamine and discussed avoidance and follow-up if worsening.",
    "Post-operative check after knee arthroscopy. Incision healing well, range of motion improving. Continue physical therapy.",
    "Anxiety follow-up. Reports improvement on sertraline; sleep and appetite normal. Continue current dose and counseling." };

struct ProvDef { const char *name, *spec, *color; };
static const QVector<ProvDef> kProviders = {
    {"Dr. Sarah Chen","Family Medicine","#3B82F6"},
    {"Dr. James Okafor","Internal Medicine","#10B981"},
    {"Dr. Maria Santos","Pediatrics","#8B5CF6"},
    {"Dr. David Kim","Cardiology","#EF4444"},
    {"Dr. Aisha Patel","Dermatology","#F59E0B"},
    {"Dr. Tom Nguyen","Orthopedics","#06B6D4"} };

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    // Register the gadget value types so QML can read patient.firstName / latestVital.systolic.
    qRegisterMetaType<Patient>("Patient");
    qRegisterMetaType<Vital>("Vital");
    // Expose the enums to QML as `Clinic.Arrived`, `Clinic.Active`, `Clinic.OfficeVisit`, …
    // (a separate URI so it doesn't collide with the QML_ELEMENT-managed "Qivot" module).
    qmlRegisterUncreatableMetaObject(Clinic::staticMetaObject, "ClinicApp", 1, 0, "Clinic",
                                     "Clinic enums are not creatable");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
#ifdef Q_OS_WASM
    // The browser sandbox has no persistent filesystem; keep the DB in memory.
    // (The schema is dropped and reseeded every launch anyway — see below.)
    db.setDatabaseName(":memory:");
#else
    db.setDatabaseName("clinic.db");
#endif
    if (!db.open()) return 1;

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Provider>();  connection.addModel<Patient>();
    connection.addModel<Appointment>(); connection.addModel<Vital>();
    connection.addModel<Problem>(); connection.addModel<Medication>();
    connection.addModel<Note>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    const QDate today = QDate::currentDate();
    const int   nowMin = QTime::currentTime().hour() * 60 + QTime::currentTime().minute();

    // --- providers ---
    QVector<int> provIds;
    for (const ProvDef &pd : kProviders) {
        Provider p; p.name = pd.name; p.specialty = pd.spec; p.color = pd.color;
        p.save(); provIds << p.id().toInt();
    }

    // --- patients + their chart data ---
    const int N = 36;
    QVector<int> patIds;
    for (int i = 0; i < N; i++) {
        Patient p;
        p.mrn = QString("MRN-%1").arg(100000 + i * 37);
        p.firstName = kFirst.at(i % kFirst.size());
        p.lastName  = kLast.at((i * 5 + 3) % kLast.size()).trimmed();
        const int age = 4 + (i * 17) % 82;
        p.dob = today.addDays(-(age * 365 + (i * 53) % 360)).toString("yyyy-MM-dd");
        p.sex = kSex.at(i % kSex.size());
        p.phone = QString("(555) %1-%2").arg(200 + i % 700, 3, 10, QChar('0')).arg((i * 4127) % 10000, 4, 10, QChar('0'));
        p.bloodType = kBlood.at((i * 3) % kBlood.size());
        p.allergies = kAllergy.at((i * 7) % kAllergy.size());
        p.save();
        const int pid = p.id().toInt();
        patIds << pid;

        const int nProb = 1 + (i % 3);
        for (int k = 0; k < nProb; k++) {
            Problem x; x.patientId = pid;
            x.name = kProblem.at((i * 5 + k * 3) % kProblem.size());
            x.status = (k == 0 || (i + k) % 4 != 0) ? Clinic::Active : Clinic::Resolved;
            x.onset = today.addDays(-((i + 1) * 90 + k * 200)).toString("yyyy-MM-dd");
            x.save();
        }
        const int nMed = 1 + ((i + 1) % 3);
        for (int k = 0; k < nMed; k++) {
            const MedDef &md = kMed.at((i * 3 + k) % kMed.size());
            Medication x; x.patientId = pid;
            x.name = md.name; x.dose = md.dose; x.frequency = md.freq;
            x.active = ((i + k) % 5 != 0);
            x.save();
        }
        const int nVit = 2 + (i % 3);
        for (int k = 0; k < nVit; k++) {
            Vital v; v.patientId = pid;
            v.takenOn = today.addDays(-(k * 120 + (i % 30))).toString("yyyy-MM-dd");
            v.systolic = 108 + (i * 3 + k) % 45;
            v.diastolic = 66 + (i * 2 + k) % 26;
            v.heartRate = 58 + (i + k * 5) % 40;
            v.tempC = 36.4 + ((i + k) % 12) / 10.0;
            v.spo2 = 95 + (i + k) % 5;
            v.weightKg = 52.0 + (i * 2 + k) % 55;
            v.heightCm = 150 + (i * 2) % 45;
            v.save();
        }
        const int nNote = 1 + (i % 3);
        for (int k = 0; k < nNote; k++) {
            Note n; n.patientId = pid;
            n.providerId = provIds.at((i + k) % provIds.size());
            n.date = today.addDays(-(k * 75 + (i % 40))).toString("yyyy-MM-dd");
            n.kind = Clinic::NoteKind((i + k) % 4);
            n.body = kNoteBody.at((i * 2 + k) % kNoteBody.size());
            n.save();
        }
    }

    // --- appointments across a window around today ---
    int ctr = 0;
    for (int off = -4; off <= 12; off++) {
        const QString day = today.addDays(off).toString("yyyy-MM-dd");
        for (int pv = 0; pv < provIds.size(); pv++) {
            int minute = 480 + (pv % 2) * 30;                 // stagger start 8:00 / 8:30
            const int nSlots = 3 + (pv + qAbs(off) * 3) % 4;  // 3..6 per provider
            for (int s = 0; s < nSlots; s++) {
                const int dur = (s % 3 == 0) ? 30 : (s % 3 == 1 ? 20 : 45);
                Appointment a;
                a.patientId = patIds.at((ctr * 7 + pv * 3) % patIds.size());
                a.providerId = provIds.at(pv);
                a.day = day;
                a.minute = minute;
                a.durationMin = dur;
                a.reason = kReason.at((ctr * 5) % kReason.size());
                if (off < 0)      a.status = (ctr % 9 == 0) ? Clinic::Cancelled : Clinic::Completed;
                else if (off > 0) a.status = Clinic::Scheduled;
                else {  // today: reflect the current time of day
                    if (minute + dur <= nowMin)          a.status = Clinic::Completed;
                    else if (minute <= nowMin + 30)      a.status = Clinic::Arrived;
                    else                                 a.status = (ctr % 11 == 0) ? Clinic::Cancelled : Clinic::Scheduled;
                }
                a.save();
                minute += dur + 10 + (ctr % 3) * 10;          // gap to next slot
                ctr++;
            }
        }
    }

    // Full-text index over clinical notes — search stays in sync on every save().
    QiFtsIndex<Note> nfts("note_fts");
    nfts << "body";        // kind is an enum (int) now, so only the free text is indexed
    (void) connection.createFtsIndex(nfts);

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        qInfo().noquote() << QString("Clinic seeded: %1 patients, %2 providers, %3 appointments, %4 notes")
                              .arg(Patient::objects().count())
                              .arg(Provider::objects().count())
                              .arg(Appointment::objects().count())
                              .arg(Note::objects().count());
        const QString td = today.toString("yyyy-MM-dd");
        qInfo().noquote() << QString("Today (%1): %2 appointments")
                              .arg(td)
                              .arg(QiQuery<Appointment>().filter(QiWhere("day = ", td)).count());
        qInfo().noquote() << QString("Note search 'physical*': %1 hits")
                              .arg(QiQuery<Note>().search("note_fts", "physical*").count());
        return 0;
    }

    // The two C++ backends every QML component talks to.
    Theme theme;
    ClinicStore store;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("Theme", &theme);
    engine.rootContext()->setContextProperty("store", &store);
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
