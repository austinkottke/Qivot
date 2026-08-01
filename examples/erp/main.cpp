/** Qivot ERP — a small professional-services ERP/PSA demo.

    CRM (clients, contacts, opportunities), projects, time & expense tracking,
    and invoicing over a small relational schema (eight models). All data is
    SYNTHETIC, generated at startup — no real companies or people.

    Showcases: joins/lookups across eight models, GROUP BY aggregate queries
    (pipeline by stage, revenue by month, top clients, AR aging), a transaction
    that turns a project's unbilled billable time + expenses into an invoice,
    and filtered search — all served to QML as plain QVariant maps/lists.

    QIVOT_SELFTEST=1 seeds, prints row counts + a couple of aggregates, and quits.
 */
#include "models.h"
#include "erpstore.h"
#include "theme.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>            // qmlRegisterUncreatableMetaObject
#include <QQmlContext>
#include <QSqlDatabase>
#include <QDate>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QDebug>

// ---------- synthetic data pools ----------
struct EmpDef { const char *name, *title; double bill, cost; };
static const QVector<EmpDef> kEmployees = {
    {"Rachel Kim",     "Principal",         275, 165},
    {"Marcus Webb",    "Principal",         260, 155},
    {"Sofia Alvarez",  "Project Manager",   175, 105},
    {"David Chen",     "Project Manager",   170, 100},
    {"Priya Nair",     "Senior Engineer",   155,  90},
    {"Tom Whitfield",  "Senior Architect",  160,  95},
    {"Grace Osei",     "Engineer",          120,  70},
    {"Liam Fischer",   "Designer",          110,  62},
    {"Nina Torres",    "Staff Accountant",   95,  55},
};

struct ClientDef { const char *name, *industry, *city, *state, *phone; };
static const QVector<ClientDef> kClients = {
    {"Meridian Health System",           "Healthcare",             "Portland",       "OR", "(503) 555-0142"},
    {"Blackstone Development Group",     "Real Estate Development","Denver",         "CO", "(303) 555-0198"},
    {"Cascade Transit Authority",        "Government",             "Seattle",        "WA", "(206) 555-0110"},
    {"Ashford University",               "Higher Education",       "Raleigh",        "NC", "(919) 555-0176"},
    {"Harborview Medical Partners",      "Healthcare",              "Tacoma",        "WA", "(253) 555-0133"},
    {"Sterling Retail Partners",         "Retail",                 "Austin",         "TX", "(512) 555-0121"},
    {"Greenfield Municipal Water",       "Government",             "Sacramento",     "CA", "(916) 555-0155"},
    {"Lakeside Regional Airport",        "Government",             "Minneapolis",    "MN", "(612) 555-0187"},
    {"Vantage Biotech Campus",           "Life Sciences",          "Boston",         "MA", "(617) 555-0164"},
    {"Northgate School District",        "K-12 Education",         "Columbus",       "OH", "(614) 555-0129"},
    {"Ironbridge Manufacturing Co.",     "Industrial",             "Nashville",      "TN", "(615) 555-0147"},
    {"Pinnacle Hospitality Group",       "Hospitality",            "Salt Lake City", "UT", "(801) 555-0108"},
    {"Crestview Senior Living",          "Senior Living",          "Richmond",       "VA", "(804) 555-0193"},
    {"Union Freight Logistics",          "Industrial",             "Boise",          "ID", "(208) 555-0116"},
};
static const QVector<const char *> kContactTitles = { "Facilities Director", "VP Operations", "Project Sponsor",
    "CFO", "Director of Planning", "Owner's Representative" };
static const QStringList kFirst = { "James","Maria","Robert","Linda","Michael","Susan","William","Karen",
    "Carlos","Jennifer","Ahmed","Patricia","Kevin","Laura","Daniel","Nancy" };
static const QStringList kLast  = { "Bennett","Alvarado","Morrison","Yates","Delgado","Whitman","Souza",
    "Park","Reyes","Holloway","Nash","Ferreira","Doyle","Iqbal","Castillo","Larsen" };

struct ProjDef { int client; const char *type; int startMonthsAgo; int durationDays; Erp::ProjectStatus status; double budget; };
static const QVector<ProjDef> kProjects = {
    {0,  "Master Plan",             14, 240, Erp::Completed, 420000},
    {0,  "Renovation & Expansion",   5,   0, Erp::Active,    680000},
    {1,  "Feasibility Study",       10,  90, Erp::Completed,  95000},
    {1,  "New Construction",         6,   0, Erp::Active,   2400000},
    {2,  "Infrastructure Upgrade",  16, 300, Erp::Completed,1150000},
    {2,  "Site Assessment",          2,   0, Erp::Planning,   60000},
    {3,  "Design-Build",             8,   0, Erp::Active,   1800000},
    {4,  "Programming Study",       12, 120, Erp::Completed, 140000},
    {4,  "Renovation & Expansion",   4,   0, Erp::Active,    560000},
    {5,  "New Construction",         7,   0, Erp::Active,   3100000},
    {6,  "Master Plan",              3,   0, Erp::Active,    210000},
    {7,  "Infrastructure Upgrade",   9,   0, Erp::OnHold,    890000},
    {8,  "Feasibility Study",        1,   0, Erp::Planning,   75000},
    {9,  "New Construction",         5,   0, Erp::Active,   1950000},
    {10, "Renovation & Expansion",  11, 150, Erp::Completed, 320000},
    {10, "Design-Build",             6,   0, Erp::Active,   1400000},
};
static const QStringList kExpenseCategories = { "Travel","Meals","Printing","Mileage","Lodging","Software","Supplies" };
static const QStringList kNotes = {
    "Site walkthrough and existing conditions review.",
    "Client coordination meeting and progress update.",
    "Drawing set revisions per client comments.",
    "Code review and permitting coordination.",
    "Cost estimate refresh and budget reconciliation.",
    "Design development — floor plans and elevations.",
    "Specification writing and materials research.",
    "Structural / MEP coordination with consultants.",
    "Punch list walkthrough.",
    "Internal QA/QC review of deliverables." };

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    qRegisterMetaType<Client>("Client");
    qRegisterMetaType<Project>("Project");
    qmlRegisterUncreatableMetaObject(Erp::staticMetaObject, "ErpApp", 1, 0, "Erp",
                                     "Erp enums are not creatable");

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
#ifdef Q_OS_WASM
    db.setDatabaseName(":memory:");
#else
    db.setDatabaseName("erp.db");
#endif
    if (!db.open()) return 1;

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Employee>();    connection.addModel<Client>();
    connection.addModel<Contact>();     connection.addModel<Opportunity>();
    connection.addModel<Project>();     connection.addModel<TimeEntry>();
    connection.addModel<Expense>();     connection.addModel<Invoice>();
    if (!connection.dropTables() || !connection.createTables()) return 1;

    const QDate today = QDate::currentDate();
    const auto iso = [](const QDate &d) { return d.toString("yyyy-MM-dd"); };

    // --- employees ---
    QVector<int> empIds;
    QHash<int, double> empBillRate;
    for (const EmpDef &ed : kEmployees) {
        Employee e; e.name = ed.name; e.title = ed.title; e.billRate = ed.bill; e.costRate = ed.cost;
        e.active = true;
        e.email = QString(ed.name).toLower().replace(' ', '.') + "@studio-ae.example";
        e.save();
        const int id = e.id().toInt();
        empIds << id; empBillRate[id] = ed.bill;
    }

    // --- clients + contacts ---
    QVector<int> clientIds;
    for (int i = 0; i < kClients.size(); i++) {
        const ClientDef &cd = kClients.at(i);
        Client c; c.name = cd.name; c.industry = cd.industry; c.city = cd.city; c.state = cd.state; c.phone = cd.phone;
        c.save();
        const int cid = c.id().toInt();
        clientIds << cid;

        const int nContacts = 1 + (i % 2);
        for (int k = 0; k < nContacts; k++) {
            Contact ct; ct.client = cid;
            ct.name = kFirst.at((i * 3 + k) % kFirst.size()) + " " + kLast.at((i * 5 + k * 2) % kLast.size());
            ct.title = kContactTitles.at((i + k) % kContactTitles.size());
            ct.email = QString(ct.name.get().toString()).toLower().replace(' ', '.') + "@"
                     + QString(cd.name).toLower().split(' ').first() + ".example";
            ct.phone = cd.phone;
            ct.save();
        }
    }

    // --- opportunities: some open pipeline, some already won/lost ---
    static const QVector<Erp::OppStage> kStageCycle = { Erp::Lead, Erp::Qualified, Erp::Proposal, Erp::Won, Erp::Lost };
    int oppCtr = 0;
    for (int i = 0; i < clientIds.size(); i++) {
        const int nOpp = 1 + (i % 3);
        for (int k = 0; k < nOpp; k++) {
            Opportunity o; o.client = clientIds.at(i);
            o.name = QString("%1 — %2").arg(kClients.at(i).name)
                       .arg(kProjects.at((oppCtr * 3) % kProjects.size()).type);
            o.stage = kStageCycle.at(oppCtr % kStageCycle.size());
            o.amount = 80000 + ((oppCtr * 53171) % 900000);
            o.closeDate = iso(today.addDays((oppCtr % 5 == 0 ? -1 : 1) * (10 + (oppCtr * 17) % 90)));
            o.save();
            oppCtr++;
        }
    }

    // --- projects, then time entries / expenses / invoices per project ---
    int invCtr = 0;
    for (int pi = 0; pi < kProjects.size(); pi++) {
        const ProjDef &pd = kProjects.at(pi);
        const int clientId = clientIds.at(pd.client);

        // 2-4 person team, deterministic but spread across the roster
        QSet<int> teamSet;
        const int teamSize = 2 + (pi % 3);
        for (int k = 0; k < teamSize && teamSet.size() < teamSize; k++)
            teamSet.insert(empIds.at((pi * 2 + k * 3 + k) % empIds.size()));
        const QList<int> team = teamSet.values();

        Project p; p.client = clientId; p.manager = team.first();
        p.code = QString("PRJ-%1").arg(1000 + pi);
        p.name = QString("%1 %2").arg(kClients.at(pd.client).name).arg(pd.type);
        p.status = pd.status;
        const QDate start = today.addDays(-pd.startMonthsAgo * 30);
        p.startDate = iso(start);
        p.budget = pd.budget;
        p.save();
        const int projectId = p.id().toInt();

        // Formal staffing roster (QI_MANY_TO_MANY, project_team join table) —
        // a declared assignment, independent of who ends up actually logging hours.
        for (int empId : team) {
            QiList<Employee> el = QiQuery<Employee>().filter(QiWhere("id = ", empId)).limit(1).all();
            if (el.size()) p.teamMembers().add(*el.at(0));
        }

        if (pd.status == Erp::Planning) continue;   // no work logged yet

        const QDate workEnd = pd.durationDays > 0 ? start.addDays(pd.durationDays) : today;

        // time entries: roughly twice a week per team member
        for (QDate d = start; d <= workEnd; d = d.addDays(3)) {
            for (int ti = 0; ti < team.size(); ti++) {
                const int empId = team.at(ti);
                if ((d.dayOfYear() + empId + ti) % 5 >= 3) continue;   // ~40% of steps
                TimeEntry t; t.project = projectId; t.employee = empId; t.date = iso(d);
                t.hours = 2 + ((d.dayOfYear() + empId * 3) % 7);
                t.billable = ((d.dayOfYear() + empId) % 6 != 0);
                t.notes = kNotes.at((d.dayOfYear() + empId + ti) % kNotes.size());
                t.invoiceId = 0;
                t.save();
            }
        }
        // expenses: roughly monthly
        for (QDate d = start; d <= workEnd; d = d.addDays(12)) {
            const int empId = team.at((d.dayOfYear() / 12) % team.size());
            Expense ex; ex.project = projectId; ex.employee = empId; ex.date = iso(d);
            ex.amount = 60 + ((d.dayOfYear() * 37 + empId * 91) % 900);
            ex.category = kExpenseCategories.at((d.dayOfYear() + empId) % kExpenseCategories.size());
            ex.billable = (d.dayOfYear() % 4 != 0);
            ex.invoiceId = 0;
            ex.save();
        }

        // invoicing: bill in ~30-day cycles. Completed projects are billed all
        // the way through; active projects are billed up to a cutoff so some
        // recent billable work is left unbilled (for the "Generate Invoice" demo).
        const QDate billCutoff = pd.status == Erp::Completed ? workEnd : today.addDays(-18);
        int cycleIdx = 0;
        for (QDate cs = start; cs.addDays(30) <= billCutoff; cs = cs.addDays(30)) {
            const QDate ce = cs.addDays(30);
            const QString csIso = iso(cs), ceIso = iso(ce);

            QiList<TimeEntry> tes = QiQuery<TimeEntry>().filter(
                QiWhere("project = ", projectId) && QiWhere("billable = ", true)
                && QiWhere("invoiceId = ", 0) && QiWhere("date >= ", csIso) && QiWhere("date < ", ceIso)).all();
            QiList<Expense> exs = QiQuery<Expense>().filter(
                QiWhere("project = ", projectId) && QiWhere("billable = ", true)
                && QiWhere("invoiceId = ", 0) && QiWhere("date >= ", csIso) && QiWhere("date < ", ceIso)).all();
            if (tes.size() == 0 && exs.size() == 0) { cycleIdx++; continue; }

            double amount = 0;
            for (int i = 0; i < tes.size(); i++)
                amount += tes.at(i)->hours.get().toDouble() * empBillRate.value(tes.at(i)->employee.get().toInt());
            for (int i = 0; i < exs.size(); i++) amount += exs.at(i)->amount.get().toDouble();

            const QDate dueDate = ce.addDays(30);
            const bool isLastCycle = ce.addDays(30) > billCutoff;
            Erp::InvoiceStatus status;
            if (isLastCycle && pd.status != Erp::Completed && pi % 4 == 0) status = Erp::Draft;
            else if (dueDate < today)  status = ((pi + cycleIdx) % 3 != 0) ? Erp::Paid : Erp::Sent;  // overdue if still Sent
            else                        status = Erp::Sent;   // current, not yet due

            Invoice inv; inv.client = clientId; inv.project = projectId;
            inv.number = QString("INV-%1").arg(1000 + invCtr);
            inv.issueDate = ceIso; inv.dueDate = iso(dueDate); inv.status = status; inv.amount = amount;
            inv.save();
            const int invId = inv.id().toInt();
            for (int i = 0; i < tes.size(); i++) { tes.at(i)->invoiceId = invId; tes.at(i)->save(); }
            for (int i = 0; i < exs.size(); i++) { exs.at(i)->invoiceId = invId; exs.at(i)->save(); }
            invCtr++; cycleIdx++;
        }
    }

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        qInfo().noquote() << QString("Erp seeded: %1 clients, %2 projects, %3 time entries, %4 expenses, %5 invoices")
                              .arg(Client::objects().count()).arg(Project::objects().count())
                              .arg(TimeEntry::objects().count()).arg(Expense::objects().count())
                              .arg(Invoice::objects().count());
        qInfo().noquote() << QString("Open opportunities: %1, Active projects: %2")
                              .arg(QiQuery<Opportunity>().filter(QiWhere("stage = ", int(Erp::Lead))
                                     || QiWhere("stage = ", int(Erp::Qualified))
                                     || QiWhere("stage = ", int(Erp::Proposal))).count())
                              .arg(QiQuery<Project>().filter(QiWhere("status = ", int(Erp::Active))).count());
        return 0;
    }

    Theme theme;
    ErpStore store;

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &e : warnings) fprintf(stderr, "QML ERROR: %s\n", qPrintable(e.toString()));
    });
    engine.rootContext()->setContextProperty("Theme", &theme);
    engine.rootContext()->setContextProperty("store", &store);
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
