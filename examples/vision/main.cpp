/** vision — a mock "Deltek Vision" reporting demo on Qivot.

    Deltek Vision (the A/E/C professional-services ERP) stores its data in
    Microsoft SQL Server. Qivot now speaks SQL Server (QODBC -> QiMsSqlStatement),
    so the same models + reporting code run unchanged against SQLite here and
    against a real Vision database. See models.h for the schema mapping.

    Default: launches the QML dashboard (Overview, Projects, Utilization, Ledger)
    on an in-memory SQLite database.

    Console modes (no GUI):
        ./vision --report            # print the two reports to the terminal
        ./vision --ddl <dialect>     # the CREATE TABLE DDL Qivot generates
        ./vision --script <dialect>  # a full runnable script: DDL + seed + reports

      <dialect> = sqlite | sqlserver | postgres | mysql | oracle | duckdb

    The --script output for `sqlserver` is what proves the integration end-to-end:
    pipe it into a real SQL Server (e.g. `sqlcmd`) and the two reports come back
    from the genuine engine, over a schema whose T-SQL DDL Qivot emitted.
 */
#include "models.h"
#include "seeddata.h"
#include "visionstore.h"
#include "theme.h"

#include <qisqlstatement.h>
#include <qimodelmetainfo.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QTimer>
#include <QImage>
#include <QSqlDatabase>
#include <QSqlError>
#include <QString>
#include <QVector>
#include <QDebug>
#include <cstdio>

// ---- seed the demo firm through the ORM (shared by the UI and --report) -------

static bool seedViaOrm() {
    for (const auto &r : kClients) {
        Cl c; c.ClientID = r.ClientID; c.Name = r.Name; c.Type = r.Type; c.Status = r.Status;
        if (!c.save()) return false;
    }
    for (const auto &r : kEmployees) {
        Em e; e.Employee = r.Employee; e.LastName = r.LastName; e.FirstName = r.FirstName;
        e.Title = r.Title; e.Status = r.Status; e.BillRate = r.BillRate; e.CostRate = r.CostRate;
        if (!e.save()) return false;
    }
    for (const auto &r : kProjects) {
        Pr p; p.WBS1 = r.WBS1; p.Name = r.Name; p.Org = r.Org; p.Status = r.Status;
        p.ProjMgr = r.ProjMgr; p.Principal = r.Principal; p.ClientID = r.ClientID;
        p.Fee = r.Fee; p.ChargeType = r.ChargeType;
        if (!p.save()) return false;
    }
    for (const auto &r : kLabor) {
        Ld l; l.WBS1 = r.WBS1; l.Employee = r.Employee; l.TransDate = r.TransDate;
        l.RegHrs = r.RegHrs; l.BillRate = r.BillRate; l.CostRate = r.CostRate;
        if (!l.save()) return false;
    }
    for (const auto &r : kBilling) {
        Bi b; b.WBS1 = r.WBS1; b.InvoiceDate = r.InvoiceDate; b.Amount = r.Amount; b.Received = r.Received;
        if (!b.save()) return false;
    }
    return true;
}

// ---- console reports ----------------------------------------------------------

static void printReports() {
    std::printf("\n  PROJECT EARNINGS  (contract fee vs. billed labor vs. cost)\n");
    std::printf("  %-12s %-30s %10s %10s %10s %10s  %6s\n",
                "WBS1", "Project", "Fee", "Billed", "Cost", "Margin", "%Fee");
    std::printf("  %s\n", QString(94, '-').toLocal8Bit().constData());
    QiList<ProjectEarnings> earnings = qiRawQuery<ProjectEarnings>(kEarningsSql);
    for (int i = 0; i < earnings.size(); i++) {
        ProjectEarnings *e = earnings.at(i);
        const double fee = e->Fee, billed = e->Billed, cost = e->Cost;
        std::printf("  %-12s %-30s %10.0f %10.0f %10.0f %10.0f  %5.0f%%\n",
                    QString(e->WBS1).toLocal8Bit().constData(),
                    QString(e->Name).toLocal8Bit().constData(),
                    fee, billed, cost, billed - cost, fee > 0 ? billed / fee * 100.0 : 0.0);
    }

    std::printf("\n  EMPLOYEE UTILIZATION  (active staff)\n");
    std::printf("  %-6s %-22s %10s %10s %8s\n", "Emp", "Name", "Total h", "Billable", "Util%");
    std::printf("  %s\n", QString(60, '-').toLocal8Bit().constData());
    QiList<EmpUtil> util = qiRawQuery<EmpUtil>(kUtilSql);
    for (int i = 0; i < util.size(); i++) {
        EmpUtil *u = util.at(i);
        const double total = u->TotalHrs, billable = u->BillableHrs;
        const QString name = QString("%1, %2").arg(QString(u->LastName), QString(u->FirstName));
        std::printf("  %-6s %-22s %10.1f %10.1f %7.1f%%\n",
                    QString(u->Employee).toLocal8Bit().constData(), name.toLocal8Bit().constData(),
                    total, billable, total > 0 ? billable / total * 100.0 : 0.0);
    }
    std::printf("\n");
}

// ---- script/DDL emitters (Qivot dialect layer) --------------------------------

static QString driverToken(const QString &d) {
    if (d == "sqlserver" || d == "mssql")   return "QODBC";
    if (d == "postgres"  || d == "pg")      return "QPSQL";
    if (d == "mysql"     || d == "mariadb") return "QMYSQL";
    if (d == "oracle")                      return "QOCI";
    if (d == "duckdb")                      return "QDUCKDB";
    return "QSQLITE";
}

static QString sqlStr(const char *s) { QString v(s); v.replace("'", "''"); return "'" + v + "'"; }
static QString sqlNum(double d) { return QString::number(d, 'f', 2); }

static void emitDdl(const QString &dialect) {
    QiSqlStatement *s = QiSqlStatement::forDriver(driverToken(dialect));
    const QVector<QiModelMetaInfo*> tables = {
        qiMetaInfo<Cl>(), qiMetaInfo<Em>(), qiMetaInfo<Pr>(), qiMetaInfo<Ld>()
    };
    for (QiModelMetaInfo *info : tables) {
        QString stmt = s->createTableIfNotExists(info);
        if (!stmt.trimmed().endsWith(';')) stmt += ';';
        std::printf("%s\n", stmt.toLocal8Bit().constData());
    }
    delete s;
}

static void emitScript(const QString &dialect) {
    std::printf("-- Qivot 'vision' demo — %s schema generated by QiSqlStatement::forDriver\n\n",
                dialect.toLocal8Bit().constData());
    emitDdl(dialect);
    std::printf("\n-- seed data\n");
    for (const auto &r : kClients)
        std::printf("INSERT INTO CL (ClientID, Name, Type, Status) VALUES (%s, %s, %s, %s);\n",
                    sqlStr(r.ClientID).toLocal8Bit().constData(), sqlStr(r.Name).toLocal8Bit().constData(),
                    sqlStr(r.Type).toLocal8Bit().constData(), sqlStr(r.Status).toLocal8Bit().constData());
    for (const auto &r : kEmployees)
        std::printf("INSERT INTO EM (Employee, LastName, FirstName, Title, Status, BillRate, CostRate) "
                    "VALUES (%s, %s, %s, %s, %s, %s, %s);\n",
                    sqlStr(r.Employee).toLocal8Bit().constData(), sqlStr(r.LastName).toLocal8Bit().constData(),
                    sqlStr(r.FirstName).toLocal8Bit().constData(), sqlStr(r.Title).toLocal8Bit().constData(),
                    sqlStr(r.Status).toLocal8Bit().constData(), sqlNum(r.BillRate).toLocal8Bit().constData(),
                    sqlNum(r.CostRate).toLocal8Bit().constData());
    for (const auto &r : kProjects)
        std::printf("INSERT INTO PR (WBS1, Name, Status, ProjMgr, ClientID, Fee, ChargeType) "
                    "VALUES (%s, %s, %s, %s, %s, %s, %s);\n",
                    sqlStr(r.WBS1).toLocal8Bit().constData(), sqlStr(r.Name).toLocal8Bit().constData(),
                    sqlStr(r.Status).toLocal8Bit().constData(), sqlStr(r.ProjMgr).toLocal8Bit().constData(),
                    sqlStr(r.ClientID).toLocal8Bit().constData(), sqlNum(r.Fee).toLocal8Bit().constData(),
                    sqlStr(r.ChargeType).toLocal8Bit().constData());
    for (const auto &r : kLabor)
        std::printf("INSERT INTO LD (WBS1, Employee, TransDate, RegHrs, BillRate, CostRate) "
                    "VALUES (%s, %s, %s, %s, %s, %s);\n",
                    sqlStr(r.WBS1).toLocal8Bit().constData(), sqlStr(r.Employee).toLocal8Bit().constData(),
                    sqlStr(r.TransDate).toLocal8Bit().constData(), sqlNum(r.RegHrs).toLocal8Bit().constData(),
                    sqlNum(r.BillRate).toLocal8Bit().constData(), sqlNum(r.CostRate).toLocal8Bit().constData());
    std::printf("\n-- Project Earnings\n%s;\n", kEarningsSql);
    std::printf("\n-- Employee Utilization\n%s;\n", kUtilSql);
}

// -------------------------------------------------------------------------------

int main(int argc, char **argv) {
    // --script / --ddl need no DB or GUI — handle them before anything else.
    if (argc >= 3 && QString(argv[1]) == "--ddl")    { emitDdl(QString(argv[2]).toLower());    return 0; }
    if (argc >= 3 && QString(argv[1]) == "--script") { emitScript(QString(argv[2]).toLower()); return 0; }

    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Material");   // so the dialog's controls follow Theme (dark/light)

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) { qWarning() << "cannot open sqlite"; return 1; }

    QiConnection connection;
    if (!connection.open(db)) { qWarning() << "QiConnection::open failed"; return 1; }
    connection.addModel<Cl>(); connection.addModel<Em>();
    connection.addModel<Pr>(); connection.addModel<Ld>(); connection.addModel<Bi>();
    if (!connection.dropTables() || !connection.createTables()) {
        qWarning().noquote() << "schema setup failed:" << connection.lastError().text();
        return 1;
    }
    if (!seedViaOrm()) { qWarning() << "seed failed"; return 1; }

    if (argc >= 2 && QString(argv[1]) == "--report") {
        std::printf("Deltek-Vision-style reporting on Qivot — SQLite "
                    "(same models map to a real Vision SQL Server DB).\n");
        printReports();
        return 0;
    }

    Theme theme;
    VisionStore store;

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &ws) {
        for (const QQmlError &e : ws) fprintf(stderr, "QML: %s\n", qPrintable(e.toString()));
    });
    engine.rootContext()->setContextProperty("Theme", &theme);
    engine.rootContext()->setContextProperty("store", &store);
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    // QIVOT_SHOT=<prefix> grabs a screenshot of each tab and quits (used to
    // verify the UI headlessly; run with QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software).
    if (qEnvironmentVariableIsSet("QIVOT_SHOT")) {
        const QString base = qEnvironmentVariable("QIVOT_SHOT");
        QObject *root = engine.rootObjects().first();
        QQuickWindow *w = qobject_cast<QQuickWindow *>(root);
        if (qEnvironmentVariableIsSet("QIVOT_W")) {   // e.g. QIVOT_W=390 QIVOT_H=844 for a phone
            root->setProperty("width", qEnvironmentVariable("QIVOT_W").toInt());
            root->setProperty("height", qEnvironmentVariable("QIVOT_H", "844").toInt());
        }
        auto grab = [w](const QString &path) { QImage i = w->grabWindow(); if (!i.isNull()) i.save(path); };
        const char *names[] = { "overview", "projects", "visualization", "utilization", "ledger" };
        int t = 1000;
        for (int tab = 0; tab < 5; tab++) {
            QTimer::singleShot(t, [root, tab] { root->setProperty("tab", tab); });
            QTimer::singleShot(t + 300, [grab, base, tab, names] { grab(base + names[tab] + ".png"); });
            t += 500;
        }
        QTimer::singleShot(t, [] { qApp->quit(); });
    }
    return app.exec();
}
