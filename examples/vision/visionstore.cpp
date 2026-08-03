#include "visionstore.h"
#include "seeddata.h"

#include <QLocale>
#include <algorithm>

VisionStore::VisionStore(QObject *parent) : QObject(parent) {
    buildCaches();
    buildOptions();

    // The ledger is a LIVE model: it re-runs (coalesced) whenever the LD table
    // changes on the default connection — so postLabor() below just saves a row
    // and the ledger view updates itself.
    m_ledger.setLive<Ld>(QiConnection::defaultConnection(), [] {
        return Ld::objects().orderBy(Ld::col().id.desc()).all();
    });

    refreshReports();
}

void VisionStore::buildCaches() {
    QiList<Pr> prs = Pr::objects().all();
    for (int i = 0; i < prs.size(); i++)
        m_projName.insert(QString(prs.at(i)->WBS1), QString(prs.at(i)->Name));
    QiList<Em> ems = Em::objects().all();
    for (int i = 0; i < ems.size(); i++)
        m_empName.insert(QString(ems.at(i)->Employee),
                         QString("%1, %2").arg(QString(ems.at(i)->LastName), QString(ems.at(i)->FirstName)));
}

void VisionStore::buildOptions() {
    QiList<Pr> prs = Pr::objects().orderBy("WBS1").all();
    for (int i = 0; i < prs.size(); i++) {
        QVariantMap m;
        m["wbs1"] = QString(prs.at(i)->WBS1);
        m["name"] = QString(prs.at(i)->Name);
        m["charge"] = QString(prs.at(i)->ChargeType);
        m_projectOptions.append(m);
    }
    QiList<Em> ems = Em::objects().filter(QiWhere("Status = ", QString("A"))).orderBy("Employee").all();
    for (int i = 0; i < ems.size(); i++) {
        QVariantMap m;
        m["employee"] = QString(ems.at(i)->Employee);
        m["name"]     = QString("%1, %2").arg(QString(ems.at(i)->LastName), QString(ems.at(i)->FirstName));
        m["billRate"] = double(ems.at(i)->BillRate);
        m["costRate"] = double(ems.at(i)->CostRate);
        m_staffOptions.append(m);
    }
}

void VisionStore::refreshReports() {
    // ---- project earnings (per project, with Org) ----
    m_projects.clear();
    QHash<QString, QVariantMap> orgAgg;   // org -> running totals
    QStringList orgOrder;
    double firmRevenue = 0, firmComp = 0;
    double profitMin = 0, profitMax = 0;
    bool first = true;
    int activeCount = 0;

    QiList<ProjectEarnings> earnings = qiRawQuery<ProjectEarnings>(kEarningsSql);
    for (int i = 0; i < earnings.size(); i++) {
        ProjectEarnings *e = earnings.at(i);
        const double revenue = e->Billed;          // JTD labor value = revenue
        const double comp    = e->Cost;            // direct labor = compensation
        const double profit  = revenue - comp;
        const QString org = QString(e->Org);
        QVariantMap m;
        m["wbs1"]        = QString(e->WBS1);
        m["name"]        = QString(e->Name);
        m["org"]         = org;
        m["status"]      = QString(e->Status);
        m["manager"]     = employeeName(QString(e->ProjMgr));
        m["principal"]   = employeeName(QString(e->Principal));
        m["client"]      = QString(e->ClientID);
        m["fee"]         = e->Fee;
        m["revenue"]     = revenue;
        m["compensation"]= comp;
        m["profit"]      = profit;
        m["multiplier"]  = comp > 0 ? revenue / comp : 0.0;
        m["feePct"]      = e->Fee > 0 ? revenue / e->Fee * 100.0 : 0.0;
        m_projects.append(m);

        firmRevenue += revenue; firmComp += comp;
        if (QString(e->Status) == "A") activeCount++;
        if (first || profit < profitMin) profitMin = profit;
        if (first || profit > profitMax) profitMax = profit;
        first = false;

        QVariantMap &o = orgAgg[org];
        if (!orgOrder.contains(org)) { orgOrder << org; o["org"] = org; o["revenue"] = 0.0; o["compensation"] = 0.0; o["count"] = 0; }
        o["revenue"]      = o["revenue"].toDouble() + revenue;
        o["compensation"] = o["compensation"].toDouble() + comp;
        o["count"]        = o["count"].toInt() + 1;
    }

    // ---- by-org rollup ----
    m_orgs.clear();
    for (const QString &org : orgOrder) {
        QVariantMap o = orgAgg.value(org);
        const double rev = o["revenue"].toDouble(), comp = o["compensation"].toDouble();
        o["profit"]     = rev - comp;
        o["profitPct"]  = rev > 0 ? (rev - comp) / rev * 100.0 : 0.0;
        o["multiplier"] = comp > 0 ? rev / comp : 0.0;
        m_orgs.append(o);
    }

    // ---- treemap tiles (size = compensation, colour = profit) ----
    m_treemap.clear();
    for (const QVariant &pv : m_projects) {
        const QVariantMap p = pv.toMap();
        QVariantMap t;
        t["name"]    = p["name"];
        t["wbs1"]    = p["wbs1"];
        t["org"]     = p["org"];
        t["size"]    = p["compensation"];   // Vision: Size = Total Compensation
        t["profit"]  = p["profit"];         // Vision: Color = JTD Profit
        t["revenue"] = p["revenue"];
        // normalise profit 0..1 across the range so QML can colour a red→blue ramp
        const double span = profitMax - profitMin;
        t["heat"] = span > 0 ? (p["profit"].toDouble() - profitMin) / span : 0.5;
        m_treemap.append(t);
    }

    // ---- billing / AR (from the BI ledger) ----
    double invoiced = 0, received = 0;
    QiList<Bi> bis = Bi::objects().all();
    for (int i = 0; i < bis.size(); i++) { invoiced += double(bis.at(i)->Amount); received += double(bis.at(i)->Received); }
    const double ar       = invoiced - received;
    const double unbilled = qMax(0.0, firmRevenue - invoiced);

    // ---- employee utilization ----
    m_staff.clear();
    double sumUtil = 0; int utilN = 0;
    QiList<EmpUtil> util = qiRawQuery<EmpUtil>(kUtilSql);
    for (int i = 0; i < util.size(); i++) {
        EmpUtil *u = util.at(i);
        const double total = u->TotalHrs, billable = u->BillableHrs;
        const double pct = total > 0 ? billable / total * 100.0 : 0.0;
        QVariantMap m;
        m["employee"]    = QString(u->Employee);
        m["name"]        = QString("%1, %2").arg(QString(u->LastName), QString(u->FirstName));
        m["title"]       = QString(u->Title);
        m["totalHrs"]    = total;
        m["billableHrs"] = billable;
        m["util"]        = pct;
        m_staff.append(m);
        if (total > 0) { sumUtil += pct; utilN++; }
    }

    // ---- Key Financial Metrics (firm-wide, Vision-style) ----
    const double profit = firmRevenue - firmComp;
    m_metrics.clear();
    m_metrics["ytdRevenue"]     = firmRevenue;
    m_metrics["compensation"]   = firmComp;
    m_metrics["ytdProfit"]      = profit;
    m_metrics["profitPct"]      = firmRevenue > 0 ? profit / firmRevenue * 100.0 : 0.0;
    m_metrics["multiplier"]     = firmComp > 0 ? firmRevenue / firmComp : 0.0;   // JTD Effective Multiplier
    m_metrics["ytdBilled"]      = invoiced;
    m_metrics["unbilled"]       = unbilled;
    m_metrics["outstandingAr"]  = ar;
    m_metrics["cashOnHand"]     = kCashBase + received;
    m_metrics["backlog"]        = 0.0;   // filled below
    m_metrics["activeProjects"] = activeCount;
    m_metrics["avgUtil"]        = utilN > 0 ? sumUtil / utilN : 0.0;
    m_metrics["headcount"]      = utilN;

    // backlog = remaining contract value (fee not yet earned as revenue)
    double backlog = 0;
    for (const QVariant &pv : m_projects) {
        const QVariantMap p = pv.toMap();
        backlog += qMax(0.0, p["fee"].toDouble() - p["revenue"].toDouble());
    }
    m_metrics["backlog"] = backlog;

    emit reportsChanged();
}

bool VisionStore::postLabor(const QString &wbs1, const QString &employee,
                            const QString &date, double hours,
                            double billRate, double costRate) {
    if (wbs1.isEmpty() || employee.isEmpty() || hours <= 0) {
        m_lastError = "Pick a project and employee, and enter hours > 0.";
        emit errorChanged();
        return false;
    }
    Ld l;
    l.WBS1 = wbs1; l.Employee = employee; l.TransDate = date;
    l.RegHrs = hours; l.BillRate = billRate; l.CostRate = costRate;
    if (!l.save()) {
        m_lastError = l.lastError().text();
        emit errorChanged();
        return false;
    }
    refreshReports();     // the ledger model refreshes itself (setLive)
    return true;
}

QString VisionStore::money(double amount) const {
    QLocale loc(QLocale::English, QLocale::UnitedStates);
    return "$" + loc.toString(amount, 'f', 0);
}
