#include "erpstore.h"

#include <QDate>
#include <QMap>
#include <QSet>
#include <QVector>
#include <QPair>
#include <algorithm>

// ---- formatters -------------------------------------------------------------
QString ErpStore::todayIso() const { return QDate::currentDate().toString("yyyy-MM-dd"); }

QString ErpStore::money(double amount) const {
    const bool neg = amount < 0;
    QString s = QString::number(qint64(qRound(qAbs(amount))));
    for (int i = s.size() - 3; i > 0; i -= 3) s.insert(i, ',');
    return (neg ? QStringLiteral("-$") : QStringLiteral("$")) + s;
}

QString ErpStore::stageLabel(int stage) const {
    switch (Erp::OppStage(stage)) {
        case Erp::Lead:      return "Lead";
        case Erp::Qualified: return "Qualified";
        case Erp::Proposal:  return "Proposal";
        case Erp::Won:       return "Won";
        case Erp::Lost:      return "Lost";
    }
    return "";
}

QString ErpStore::projectStatusLabel(int status) const {
    switch (Erp::ProjectStatus(status)) {
        case Erp::Planning:  return "Planning";
        case Erp::Active:    return "Active";
        case Erp::OnHold:    return "On Hold";
        case Erp::Completed: return "Completed";
    }
    return "";
}

bool ErpStore::isOverdue(int status, const QString &dueDate) const {
    if (Erp::InvoiceStatus(status) != Erp::Sent) return false;
    const QDate d = QDate::fromString(dueDate, "yyyy-MM-dd");
    return d.isValid() && d < QDate::currentDate();
}

QString ErpStore::invoiceStatusLabel(int status, const QString &dueDate) const {
    if (isOverdue(status, dueDate)) return "Overdue";
    switch (Erp::InvoiceStatus(status)) {
        case Erp::Draft: return "Draft";
        case Erp::Sent:  return "Sent";
        case Erp::Paid:  return "Paid";
    }
    return "";
}

// ---- ctor ---------------------------------------------------------------
ErpStore::ErpStore(QObject *parent) : QObject(parent) {
    for (QiListModel *m : { &m_employees, &m_clients, &m_contacts, &m_opportunities, &m_projects,
                            &m_projTimeEntries, &m_projExpenses, &m_projInvoices,
                            &m_timeEntries, &m_expenses, &m_invoices })
        m->setParent(this);   // C++ owns them; QML just reads them

    loadCaches();
    refreshClients();
    refreshProjects();
    refreshTimeExpense();
    refreshInvoices();
    rebuildOverview();

    QiList<Client> firstClient = QiQuery<Client>().orderBy("name asc").limit(1).all();
    if (firstClient.size()) selectClient(firstClient.at(0)->id().toInt());

    QiList<Project> firstProject = QiQuery<Project>().orderBy("startDate desc").limit(1).all();
    if (firstProject.size()) selectProject(firstProject.at(0)->id().toInt());
}

// Lookup caches used only where a *rendered row* needs a name without an extra
// query per row (avoiding N+1 while a list repaints). Kept fresh on every write.
// One-shot actions (toggleTeamMember, generateInvoice) use QiForeignKey's own
// auto-load instead — see the note above generateInvoice().
void ErpStore::loadCaches() {
    QiList<Employee> emps = QiQuery<Employee>().orderBy("name asc").all();
    m_empName.clear(); m_empBillRate.clear(); m_empCostRate.clear();
    for (int i = 0; i < emps.size(); i++) {
        Employee *e = emps.at(i);
        const int id = e->id().toInt();
        m_empName[id] = e->name.get().toString();
        m_empBillRate[id] = e->billRate.get().toDouble();
        m_empCostRate[id] = e->costRate.get().toDouble();
    }
    m_employees.setList(emps);   // employee ComboBoxes bind straight to this

    QiList<Client> cls = QiQuery<Client>().orderBy("name asc").all();
    m_clientName.clear();
    for (int i = 0; i < cls.size(); i++) m_clientName[cls.at(i)->id().toInt()] = cls.at(i)->name.get().toString();

    QiList<Project> projs = QiQuery<Project>().orderBy("name asc").all();
    m_projName.clear(); m_projCode.clear();
    for (int i = 0; i < projs.size(); i++) {
        Project *p = projs.at(i);
        m_projName[p->id().toInt()] = p->name.get().toString();
        m_projCode[p->id().toInt()] = p->code.get().toString();
    }
}

// ---- CRM ------------------------------------------------------------------
void ErpStore::searchClients(const QString &text) { m_clientQuery = text.trimmed(); refreshClients(); }

void ErpStore::refreshClients() {
    QiQuery<Client> q;
    if (!m_clientQuery.isEmpty()) {
        const QString like = "%" + m_clientQuery + "%";
        q = q.filter(QiWhere("name LIKE ", like) || QiWhere("industry LIKE ", like)
                     || QiWhere("city LIKE ", like));
    }
    m_clients.setList(q.orderBy("name asc").all());
}

void ErpStore::selectClient(int id) {
    m_selectedClientId = id;
    QiList<Client> cl = QiQuery<Client>().filter(QiWhere("id = ", id)).limit(1).all();
    if (cl.size() == 0) {
        m_client = Client();
        m_contacts.setList(QiList<Contact>());
        m_opportunities.setList(QiList<Opportunity>());
        emit clientChanged();
        return;
    }
    Client *c = cl.at(0);
    m_client = *c;
    // QI_HAS_MANY: every Contact/Opportunity whose `client` foreign key points
    // back at this row — one query each, composed with .orderBy() like any query.
    m_contacts.setList(c->contacts().orderBy("name asc").all());
    m_opportunities.setList(c->opportunities().orderBy("stage desc, amount desc").all());
    emit clientChanged();
}

int ErpStore::addClient(const QString &name, const QString &industry, const QString &city,
                        const QString &state, const QString &phone) {
    Client c;
    c.name = name.trimmed().isEmpty() ? QString("New Client") : name.trimmed();
    c.industry = industry; c.city = city; c.state = state; c.phone = phone;
    c.save();
    const int id = c.id().toInt();
    loadCaches(); refreshClients(); rebuildOverview();
    selectClient(id);
    return id;
}

void ErpStore::addContact(int clientId, const QString &name, const QString &title,
                          const QString &email, const QString &phone) {
    if (name.trimmed().isEmpty()) return;
    Contact c;
    c.client = clientId; c.name = name.trimmed(); c.title = title; c.email = email; c.phone = phone;
    c.save();
    if (m_selectedClientId == clientId) selectClient(clientId);
}

void ErpStore::addOpportunity(int clientId, const QString &name, int stage, double amount,
                              const QString &closeDate) {
    if (name.trimmed().isEmpty()) return;
    Opportunity o;
    o.client = clientId; o.name = name.trimmed(); o.stage = Erp::OppStage(stage);
    o.amount = amount; o.closeDate = closeDate.isEmpty() ? todayIso() : closeDate;
    o.save();
    if (m_selectedClientId == clientId) selectClient(clientId);
    rebuildOverview();
}

void ErpStore::setOpportunityStage(int id, int stage) {
    QiList<Opportunity> ol = QiQuery<Opportunity>().filter(QiWhere("id = ", id)).limit(1).all();
    if (ol.size() == 0) return;
    Opportunity *o = ol.at(0);
    o->stage = Erp::OppStage(stage);
    o->save();
    if (m_selectedClientId == o->client.get().toInt()) selectClient(m_selectedClientId);
    rebuildOverview();
}

// ---- projects ---------------------------------------------------------------
void ErpStore::searchProjects(const QString &text) { m_projectQuery = text.trimmed(); refreshProjects(); }

void ErpStore::refreshProjects() {
    QiQuery<Project> q;
    if (!m_projectQuery.isEmpty()) {
        const QString like = "%" + m_projectQuery + "%";
        q = q.filter(QiWhere("name LIKE ", like) || QiWhere("code LIKE ", like));
    }
    m_projects.setList(q.orderBy("startDate desc").all());
}

void ErpStore::selectProject(int id) {
    m_selectedProjectId = id;
    QiList<Project> pl = QiQuery<Project>().filter(QiWhere("id = ", id)).limit(1).all();
    if (pl.size() == 0) {
        m_project = Project();
        m_projTimeEntries.setList(QiList<TimeEntry>());
        m_projExpenses.setList(QiList<Expense>());
        m_projInvoices.setList(QiList<Invoice>());
        m_projectSummary.clear();
        emit projectChanged();
        return;
    }
    Project *p = pl.at(0);
    m_project = *p;

    // QI_HAS_MANY again: p->timeEntries()/expenses()/invoices() are composable
    // QiQuery<T>, already scoped to "belongs to this project" by the relation.
    QiList<TimeEntry> tes = p->timeEntries().orderBy("date desc").all();
    m_projTimeEntries.setList(tes);
    QiList<Expense> exs = p->expenses().orderBy("date desc").all();
    m_projExpenses.setList(exs);
    m_projInvoices.setList(p->invoices().orderBy("issueDate desc").all());

    double totalHours = 0, billableHours = 0, laborCost = 0, billableValue = 0, unbilled = 0;
    QHash<int, double> teamHours;
    for (int i = 0; i < tes.size(); i++) {
        TimeEntry *t = tes.at(i);
        const int empId = t->employee.get().toInt();
        const double h = t->hours.get().toDouble();
        totalHours += h;
        laborCost += h * m_empCostRate.value(empId);
        teamHours[empId] += h;
        if (t->billable.get().toBool()) {
            billableHours += h;
            const double v = h * m_empBillRate.value(empId);
            billableValue += v;
            if (t->invoiceId.get().toInt() == 0) unbilled += v;
        }
    }
    double expenseTotal = 0;
    for (int i = 0; i < exs.size(); i++) {
        Expense *e = exs.at(i);
        const double amt = e->amount.get().toDouble();
        expenseTotal += amt;
        if (e->billable.get().toBool() && e->invoiceId.get().toInt() == 0) unbilled += amt;
    }

    // "who actually logged hours" — derived purely from time entries, ranked by hours.
    QVector<QPair<int, double>> ranked;
    for (auto it = teamHours.begin(); it != teamHours.end(); ++it) ranked << qMakePair(it.key(), it.value());
    std::sort(ranked.begin(), ranked.end(), [](auto &a, auto &b) { return a.second > b.second; });
    QVariantList team;
    for (auto &pr : ranked) {
        QVariantMap m; m["name"] = employeeName(pr.first); m["hours"] = pr.second;
        team << m;
    }

    // "who is formally staffed" — the QI_MANY_TO_MANY roster (project_team join
    // table), which may include people who haven't logged any hours yet and
    // exclude people who have (e.g. a manager covering one day off-roster).
    QSet<int> assignedIds;
    QiList<Employee> assigned = p->teamMembers().all();
    for (int i = 0; i < assigned.size(); i++) assignedIds.insert(assigned.at(i)->id().toInt());
    QiList<Employee> allEmp = QiQuery<Employee>().filter(QiWhere("active = ", true)).orderBy("name asc").all();
    QVariantList assignedTeam;
    for (int i = 0; i < allEmp.size(); i++) {
        Employee *e = allEmp.at(i);
        const int eid = e->id().toInt();
        QVariantMap m;
        m["id"] = eid; m["name"] = e->name.get().toString(); m["title"] = e->title.get().toString();
        m["assigned"] = assignedIds.contains(eid);
        assignedTeam << m;
    }

    const double budget = m_project.budget.get().toDouble();
    const double spent = laborCost + expenseTotal;

    QVariantMap sum;
    sum["budget"] = budget;
    sum["hoursLogged"] = totalHours;
    sum["billableHours"] = billableHours;
    sum["laborCost"] = laborCost;
    sum["billableValue"] = billableValue;
    sum["expenseTotal"] = expenseTotal;
    sum["spent"] = spent;
    sum["remaining"] = budget - spent;
    sum["percentUsed"] = budget > 0 ? qRound(100.0 * spent / budget) : 0;
    sum["unbilled"] = unbilled;
    sum["team"] = team;
    sum["teamCount"] = teamHours.size();
    sum["assignedTeam"] = assignedTeam;
    sum["assignedCount"] = assignedIds.size();
    m_projectSummary = sum;

    emit projectChanged();
}

int ErpStore::addProject(int clientId, int managerId, const QString &name,
                         const QString &startDate, double budget) {
    Project p;
    p.client = clientId; p.manager = managerId;
    p.name = name.trimmed().isEmpty() ? QString("New Project") : name.trimmed();
    p.code = QString("PRJ-%1").arg(1000 + Project::objects().count());
    p.status = Erp::Planning;
    p.startDate = startDate.isEmpty() ? todayIso() : startDate;
    p.budget = budget;
    p.save();
    const int id = p.id().toInt();
    // The manager is staffed on their own project by default (QI_MANY_TO_MANY add()).
    QiList<Employee> mgr = QiQuery<Employee>().filter(QiWhere("id = ", managerId)).limit(1).all();
    if (mgr.size()) p.teamMembers().add(*mgr.at(0));
    loadCaches(); refreshProjects(); rebuildOverview();
    selectProject(id);
    return id;
}

void ErpStore::setProjectStatus(int id, int status) {
    QiList<Project> pl = QiQuery<Project>().filter(QiWhere("id = ", id)).limit(1).all();
    if (pl.size() == 0) return;
    Project *p = pl.at(0);
    p->status = Erp::ProjectStatus(status);
    p->save();
    refreshProjects(); rebuildOverview();
    if (m_selectedProjectId == id) selectProject(id);
}

void ErpStore::toggleTeamMember(int projectId, int employeeId) {
    QiList<Project> pl = QiQuery<Project>().filter(QiWhere("id = ", projectId)).limit(1).all();
    QiList<Employee> el = QiQuery<Employee>().filter(QiWhere("id = ", employeeId)).limit(1).all();
    if (pl.size() == 0 || el.size() == 0) return;
    Project *p = pl.at(0);
    Employee *e = el.at(0);
    if (p->teamMembers().contains(*e)) p->teamMembers().remove(*e);   // QiRelationSet::contains/remove
    else                                p->teamMembers().add(*e);      // QiRelationSet::add
    if (m_selectedProjectId == projectId) selectProject(projectId);
}

// ---- time & expense -----------------------------------------------------------
void ErpStore::setTeFilterProject(int projectId) {
    m_teFilterProject = projectId;
    refreshTimeExpense();
    emit teFilterChanged();
}

void ErpStore::refreshTimeExpense() {
    QiQuery<TimeEntry> tq;
    if (m_teFilterProject > 0) tq = tq.filter(QiWhere("project = ", m_teFilterProject));
    m_timeEntries.setList(tq.orderBy("date desc").limit(300).all());

    QiQuery<Expense> eq;
    if (m_teFilterProject > 0) eq = eq.filter(QiWhere("project = ", m_teFilterProject));
    m_expenses.setList(eq.orderBy("date desc").limit(300).all());
}

void ErpStore::addTimeEntry(int projectId, int employeeId, const QString &date, double hours,
                            bool billable, const QString &notes) {
    if (hours <= 0) return;
    TimeEntry t;
    t.project = projectId; t.employee = employeeId;
    t.date = date.isEmpty() ? todayIso() : date;
    t.hours = hours; t.billable = billable; t.notes = notes; t.invoiceId = 0;
    t.save();
    refreshTimeExpense(); rebuildOverview();
    if (m_selectedProjectId == projectId) selectProject(projectId);
}

void ErpStore::addExpense(int projectId, int employeeId, const QString &date, double amount,
                          const QString &category, bool billable) {
    if (amount <= 0) return;
    Expense e;
    e.project = projectId; e.employee = employeeId;
    e.date = date.isEmpty() ? todayIso() : date;
    e.amount = amount; e.category = category.isEmpty() ? QString("Other") : category;
    e.billable = billable; e.invoiceId = 0;
    e.save();
    refreshTimeExpense(); rebuildOverview();
    if (m_selectedProjectId == projectId) selectProject(projectId);
}

// ---- invoicing ----------------------------------------------------------------
QString ErpStore::nextInvoiceNumber() { return QString("INV-%1").arg(1000 + Invoice::objects().count()); }

void ErpStore::setInvoiceFilter(const QString &status) {
    m_invoiceFilter = status;
    refreshInvoices();
    emit invoicesChanged();
}

void ErpStore::refreshInvoices() {
    QiQuery<Invoice> q;
    if (m_invoiceFilter == "Draft")        q = q.filter(QiWhere("status = ", int(Erp::Draft)));
    else if (m_invoiceFilter == "Sent")    q = q.filter(QiWhere("status = ", int(Erp::Sent))
                                                          && QiWhere("dueDate >= ", todayIso()));
    else if (m_invoiceFilter == "Overdue") q = q.filter(QiWhere("status = ", int(Erp::Sent))
                                                          && QiWhere("dueDate < ", todayIso()));
    else if (m_invoiceFilter == "Paid")    q = q.filter(QiWhere("status = ", int(Erp::Paid)));
    m_invoices.setList(q.orderBy("issueDate desc").all());
}

// Note: QiQuery::filter() *replaces* the query's WHERE clause rather than
// AND-ing onto it (each call assigns a fresh expression) — so a relation
// accessor's own filter (e.g. project.timeEntries()'s "project = X") would be
// silently lost by chaining another .filter() on top of it. When more than one
// condition is needed, combine them into a single QiWhere with && instead, as
// below — that is why this one query is hand-built rather than layered on
// project.timeEntries().
bool ErpStore::generateInvoice(int projectId, const QString &dueDate) {
    QiTransaction txn;

    QiList<Project> pl = QiQuery<Project>().filter(QiWhere("id = ", projectId)).limit(1).all();
    if (pl.size() == 0) {
        txn.rollback(); m_lastError = "Unknown project."; emit errorChanged(); return false;
    }
    const int clientId = pl.at(0)->client.get().toInt();

    QiList<TimeEntry> tes = QiQuery<TimeEntry>().filter(QiWhere("project = ", projectId)
        && QiWhere("billable = ", true) && QiWhere("invoiceId = ", 0)).all();
    QiList<Expense> exs = QiQuery<Expense>().filter(QiWhere("project = ", projectId)
        && QiWhere("billable = ", true) && QiWhere("invoiceId = ", 0)).all();

    if (tes.size() == 0 && exs.size() == 0) {
        txn.rollback();
        m_lastError = "Nothing unbilled to invoice on this project.";
        emit errorChanged();
        return false;
    }

    // One-shot action (runs once per click, not per rendered row), so this uses
    // QiForeignKey's auto-load (`->`) instead of the m_empBillRate cache — no
    // N+1 concern here the way there would be while a list of rows repaints.
    double total = 0;
    for (int i = 0; i < tes.size(); i++) {
        TimeEntry *t = tes.at(i);
        total += t->hours.get().toDouble() * t->employee->billRate.get().toDouble();
    }
    for (int i = 0; i < exs.size(); i++) total += exs.at(i)->amount.get().toDouble();

    Invoice inv;
    inv.client = clientId; inv.project = projectId;
    inv.number = nextInvoiceNumber();
    inv.issueDate = todayIso();
    inv.dueDate = dueDate.isEmpty() ? QDate::currentDate().addDays(30).toString("yyyy-MM-dd") : dueDate;
    inv.status = Erp::Draft;
    inv.amount = total;
    if (!inv.save()) {
        txn.rollback(); m_lastError = "Could not save the invoice."; emit errorChanged(); return false;
    }
    const int invId = inv.id().toInt();

    for (int i = 0; i < tes.size(); i++) { tes.at(i)->invoiceId = invId; tes.at(i)->save(); }
    for (int i = 0; i < exs.size(); i++) { exs.at(i)->invoiceId = invId; exs.at(i)->save(); }

    txn.commit();
    m_lastError.clear(); emit errorChanged();
    refreshInvoices(); refreshTimeExpense(); rebuildOverview();
    if (m_selectedProjectId == projectId) selectProject(projectId);
    return true;
}

void ErpStore::setInvoiceStatus(int id, int status) {
    QiList<Invoice> il = QiQuery<Invoice>().filter(QiWhere("id = ", id)).limit(1).all();
    if (il.size() == 0) return;
    Invoice *inv = il.at(0);
    inv->status = Erp::InvoiceStatus(status);
    inv->save();
    refreshInvoices(); rebuildOverview();
    if (m_selectedProjectId == inv->project.get().toInt()) selectProject(m_selectedProjectId);
}

// ---- overview dashboard (aggregate queries) ------------------------------------
void ErpStore::rebuildOverview() {
    m_overview.clear();

    m_overview["clients"] = Client::objects().count();
    m_overview["activeProjects"] = QiQuery<Project>().filter(QiWhere("status = ", int(Erp::Active))).count();
    m_overview["employees"] = QiQuery<Employee>().filter(QiWhere("active = ", true)).count();

    const QiWhere openStages = QiWhere("stage = ", int(Erp::Lead)) || QiWhere("stage = ", int(Erp::Qualified))
                                || QiWhere("stage = ", int(Erp::Proposal));
    m_overview["openPipelineCount"] = QiQuery<Opportunity>().filter(openStages).count();
    {
        QiQuery<Opportunity> q = QiQuery<Opportunity>().filter(openStages).select(QStringList() << "SUM(amount)");
        double v = 0; if (q.exec() && q.next()) v = q.value(0).toDouble();
        m_overview["openPipeline"] = v;
    }

    // pipeline by stage (GROUP BY)
    QMap<int, int> stageCounts; QMap<int, double> stageValues;
    {
        QiQuery<Opportunity> q = QiQuery<Opportunity>()
            .select(QStringList() << "stage" << "count(*)" << "SUM(amount)").groupBy("stage");
        if (q.exec()) while (q.next()) { stageCounts[q.value(0).toInt()] = q.value(1).toInt();
                                          stageValues[q.value(0).toInt()] = q.value(2).toDouble(); }
    }
    QVariantList byStage;
    for (int s = int(Erp::Lead); s <= int(Erp::Lost); s++) {
        QVariantMap m; m["stage"] = s; m["label"] = stageLabel(s);
        m["count"] = stageCounts.value(s, 0); m["value"] = stageValues.value(s, 0.0);
        byStage << m;
    }
    m_overview["byStage"] = byStage;

    // unbilled billable work across all projects
    {
        double unbilled = 0;
        QiList<TimeEntry> tes = QiQuery<TimeEntry>()
            .filter(QiWhere("billable = ", true) && QiWhere("invoiceId = ", 0)).all();
        for (int i = 0; i < tes.size(); i++)
            unbilled += tes.at(i)->hours.get().toDouble() * m_empBillRate.value(tes.at(i)->employee.get().toInt());
        QiList<Expense> exs = QiQuery<Expense>()
            .filter(QiWhere("billable = ", true) && QiWhere("invoiceId = ", 0)).all();
        for (int i = 0; i < exs.size(); i++) unbilled += exs.at(i)->amount.get().toDouble();
        m_overview["unbilled"] = unbilled;
    }

    // AR aging: not-yet-paid invoices, bucketed by days past due
    {
        QiList<Invoice> open = QiQuery<Invoice>().filter(QiWhere("status = ", int(Erp::Sent))).all();
        double current = 0, d30 = 0, d60 = 0, d90 = 0, d90p = 0, arTotal = 0;
        const QDate today = QDate::currentDate();
        for (int i = 0; i < open.size(); i++) {
            Invoice *inv = open.at(i);
            const double amt = inv->amount.get().toDouble();
            arTotal += amt;
            const QDate due = QDate::fromString(inv->dueDate.get().toString(), "yyyy-MM-dd");
            const int days = due.isValid() ? due.daysTo(today) : 0;
            if (days <= 0)      current += amt;
            else if (days <= 30) d30 += amt;
            else if (days <= 60) d60 += amt;
            else if (days <= 90) d90 += amt;
            else                 d90p += amt;
        }
        m_overview["arTotal"] = arTotal;
        QVariantMap aging; aging["current"] = current; aging["d30"] = d30; aging["d60"] = d60;
        aging["d90"] = d90; aging["d90p"] = d90p;
        m_overview["aging"] = aging;
    }

    // invoiced revenue by month, last 6 months (substr() on the stored yyyy-MM-dd string)
    QMap<QString, double> byMonth;
    {
        QiQuery<Invoice> q = QiQuery<Invoice>().filter(QiWhere("status <> ", int(Erp::Draft)))
            .select(QStringList() << "substr(issueDate,1,7) m" << "SUM(amount)").groupBy("m");
        if (q.exec()) while (q.next()) byMonth[q.value(0).toString()] = q.value(1).toDouble();
    }
    QVariantList revByMonth; double maxRev = 1;
    for (int i = 5; i >= 0; i--) {
        const QDate d = QDate::currentDate().addMonths(-i);
        const double v = byMonth.value(d.toString("yyyy-MM"), 0.0);
        maxRev = qMax(maxRev, v);
        QVariantMap m; m["label"] = d.toString("MMM"); m["amount"] = v;
        revByMonth << m;
    }
    m_overview["revenueByMonth"] = revByMonth;
    m_overview["revenueByMonthMax"] = maxRev;

    // top clients by invoiced amount (GROUP BY)
    QVariantList topClients;
    {
        QiQuery<Invoice> q = QiQuery<Invoice>().filter(QiWhere("status <> ", int(Erp::Draft)))
            .select(QStringList() << "client" << "SUM(amount) t").groupBy("client").orderBy("t desc").limit(5);
        if (q.exec()) while (q.next()) {
            QVariantMap m; m["name"] = clientName(q.value(0).toInt()); m["amount"] = q.value(1).toDouble();
            topClients << m;
        }
    }
    m_overview["topClients"] = topClients;

    emit overviewChanged();
}
