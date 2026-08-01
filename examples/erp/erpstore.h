#ifndef ERPSTORE_H
#define ERPSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <QVariantMap>
#include <QHash>
#include <QList>
#include <qilistmodel.h>
#include "models.h"

/// The one controller QML talks to. Query results are exposed as QiListModels
/// (roles derived straight from the model's fields — no hand-mapping), the
/// selected client/project are exposed as raw gadget values, and dashboards /
/// joins are small invokables rather than columns copied onto every row.
class ErpStore : public QObject {
    Q_OBJECT

    // --- list models (bind directly to ListView / Repeater / ComboBox) ---
    Q_PROPERTY(QAbstractItemModel *employees          READ employees          CONSTANT)
    Q_PROPERTY(QAbstractItemModel *clients             READ clients             CONSTANT)
    Q_PROPERTY(QAbstractItemModel *contacts            READ contacts            CONSTANT)
    Q_PROPERTY(QAbstractItemModel *opportunities       READ opportunities       CONSTANT)
    Q_PROPERTY(QAbstractItemModel *projects            READ projects            CONSTANT)
    Q_PROPERTY(QAbstractItemModel *projectTimeEntries  READ projectTimeEntries  CONSTANT)
    Q_PROPERTY(QAbstractItemModel *projectExpenses     READ projectExpenses     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *projectInvoices     READ projectInvoices     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *timeEntries         READ timeEntries         CONSTANT)
    Q_PROPERTY(QAbstractItemModel *expenses            READ expenses            CONSTANT)
    Q_PROPERTY(QAbstractItemModel *invoices            READ invoices            CONSTANT)

    // --- selected records, as raw gadget values ---
    Q_PROPERTY(Client  client        READ client        NOTIFY clientChanged)
    Q_PROPERTY(int     selectedClientId  READ selectedClientId  NOTIFY clientChanged)
    Q_PROPERTY(Project project       READ project       NOTIFY projectChanged)
    Q_PROPERTY(int     selectedProjectId READ selectedProjectId NOTIFY projectChanged)
    Q_PROPERTY(QVariantMap projectSummary READ projectSummary NOTIFY projectChanged)

    // --- dashboards / filters ---
    Q_PROPERTY(QVariantMap overview       READ overview       NOTIFY overviewChanged)
    Q_PROPERTY(int         teFilterProject READ teFilterProject NOTIFY teFilterChanged)
    Q_PROPERTY(QString     invoiceFilter   READ invoiceFilter   NOTIFY invoicesChanged)
    Q_PROPERTY(QString     lastError       READ lastError       NOTIFY errorChanged)

public:
    explicit ErpStore(QObject *parent = nullptr);

    QAbstractItemModel *employees()         { return &m_employees; }
    QAbstractItemModel *clients()           { return &m_clients; }
    QAbstractItemModel *contacts()          { return &m_contacts; }
    QAbstractItemModel *opportunities()     { return &m_opportunities; }
    QAbstractItemModel *projects()          { return &m_projects; }
    QAbstractItemModel *projectTimeEntries(){ return &m_projTimeEntries; }
    QAbstractItemModel *projectExpenses()   { return &m_projExpenses; }
    QAbstractItemModel *projectInvoices()   { return &m_projInvoices; }
    QAbstractItemModel *timeEntries()       { return &m_timeEntries; }
    QAbstractItemModel *expenses()          { return &m_expenses; }
    QAbstractItemModel *invoices()          { return &m_invoices; }

    Client  client() const  { return m_client; }
    int     selectedClientId() const { return m_selectedClientId; }
    Project project() const { return m_project; }
    int     selectedProjectId() const { return m_selectedProjectId; }
    QVariantMap projectSummary() const { return m_projectSummary; }

    QVariantMap overview() const { return m_overview; }
    int     teFilterProject() const { return m_teFilterProject; }
    QString invoiceFilter() const { return m_invoiceFilter; }
    QString lastError() const { return m_lastError; }

    // --- CRM actions ---
    Q_INVOKABLE void searchClients(const QString &text);
    Q_INVOKABLE void selectClient(int id);
    Q_INVOKABLE int  addClient(const QString &name, const QString &industry,
                               const QString &city, const QString &state, const QString &phone);
    Q_INVOKABLE void addContact(int clientId, const QString &name, const QString &title,
                                const QString &email, const QString &phone);
    Q_INVOKABLE void addOpportunity(int clientId, const QString &name, int stage,
                                    double amount, const QString &closeDate);
    Q_INVOKABLE void setOpportunityStage(int id, int stage);   // stage = Erp::OppStage

    // --- project actions ---
    Q_INVOKABLE void searchProjects(const QString &text);
    Q_INVOKABLE void selectProject(int id);
    Q_INVOKABLE int  addProject(int clientId, int managerId, const QString &name,
                                const QString &startDate, double budget);
    Q_INVOKABLE void setProjectStatus(int id, int status);     // status = Erp::ProjectStatus
    // Toggles the formal QI_MANY_TO_MANY staffing link (project_team), independent
    // of who has actually logged hours — see projectSummary's "team" vs "assignedTeam".
    Q_INVOKABLE void toggleTeamMember(int projectId, int employeeId);

    // --- time & expense actions ---
    Q_INVOKABLE void setTeFilterProject(int projectId);        // 0 = all projects
    Q_INVOKABLE void addTimeEntry(int projectId, int employeeId, const QString &date,
                                  double hours, bool billable, const QString &notes);
    Q_INVOKABLE void addExpense(int projectId, int employeeId, const QString &date,
                                double amount, const QString &category, bool billable);

    // --- invoicing actions ---
    Q_INVOKABLE void setInvoiceFilter(const QString &status);  // "" = all
    Q_INVOKABLE bool generateInvoice(int projectId, const QString &dueDate);
    Q_INVOKABLE void setInvoiceStatus(int id, int status);     // status = Erp::InvoiceStatus

    // --- tiny joins / formatters for QML (kept off the row data) ---
    Q_INVOKABLE QString employeeName(int id) const  { return m_empName.value(id, "—"); }
    Q_INVOKABLE QString clientName(int id) const    { return m_clientName.value(id, "—"); }
    Q_INVOKABLE QString projectName(int id) const   { return m_projName.value(id, "—"); }
    Q_INVOKABLE QString projectCode(int id) const   { return m_projCode.value(id, "—"); }
    Q_INVOKABLE QString stageLabel(int stage) const;
    Q_INVOKABLE QString projectStatusLabel(int status) const;
    // status = Erp::InvoiceStatus; returns "Overdue" instead of "Sent" once dueDate has passed
    Q_INVOKABLE QString invoiceStatusLabel(int status, const QString &dueDate) const;
    Q_INVOKABLE bool    isOverdue(int status, const QString &dueDate) const;
    Q_INVOKABLE QString todayIso() const;
    Q_INVOKABLE QString money(double amount) const;         // "$12,400"

signals:
    void clientChanged();
    void projectChanged();
    void overviewChanged();
    void teFilterChanged();
    void invoicesChanged();
    void errorChanged();

private:
    void loadCaches();          // employee / client / project name lookups
    void refreshClients();
    void refreshProjects();
    void refreshTimeExpense();
    void refreshInvoices();
    void rebuildOverview();
    QString nextInvoiceNumber();

    QiListModel m_employees, m_clients, m_contacts, m_opportunities, m_projects,
                m_projTimeEntries, m_projExpenses, m_projInvoices,
                m_timeEntries, m_expenses, m_invoices;

    Client  m_client;
    int     m_selectedClientId = -1;
    Project m_project;
    int     m_selectedProjectId = -1;
    QVariantMap m_projectSummary, m_overview;

    QString m_clientQuery, m_projectQuery, m_invoiceFilter, m_lastError;
    int     m_teFilterProject = 0;

    QHash<int, QString> m_empName, m_clientName, m_projName, m_projCode;
    QHash<int, double>  m_empBillRate, m_empCostRate;
};

#endif // ERPSTORE_H
