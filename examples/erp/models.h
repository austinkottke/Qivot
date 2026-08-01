#ifndef MODELS_H
#define MODELS_H
#include <QObject>      // Q_NAMESPACE / Q_ENUM_NS
#include <qivot.h>
#include <qigadget.h>   // Q_GADGET + QI_QML_FIELD — exposes fields to QML directly

// Finite-set fields are real enums, not magic strings. Qivot stores a QiField<enum>
// in an INTEGER column; Q_ENUM_NS also exposes the names to QML (Erp.Won, …).
namespace Erp {
Q_NAMESPACE
enum OppStage      { Lead = 0, Qualified = 1, Proposal = 2, Won = 3, Lost = 4 };
Q_ENUM_NS(OppStage)
enum ProjectStatus { Planning = 0, Active = 1, OnHold = 2, Completed = 3 };
Q_ENUM_NS(ProjectStatus)
enum InvoiceStatus { Draft = 0, Sent = 1, Paid = 2 };   // "Overdue" is derived: Sent + past due date
Q_ENUM_NS(InvoiceStatus)
}
Q_DECLARE_METATYPE(Erp::OppStage)
Q_DECLARE_METATYPE(Erp::ProjectStatus)
Q_DECLARE_METATYPE(Erp::InvoiceStatus)

// A small professional-services ERP schema: CRM (clients/contacts/opportunities),
// projects, time & expense tracking, and invoicing over EIGHT related models.
// All data is SYNTHETIC. Dates are "yyyy-MM-dd" strings; money is double (a real
// ledger would use integer cents).
//
// Relations use qivot's full vocabulary, not hand-rolled integer columns:
//   - QiForeignKey<T>   the "one" side of a relation (child -> parent), with
//                       lazy auto-loading: `entry.project->name` fetches the
//                       parent row the first time it's dereferenced.
//   - QI_HAS_MANY       the reverse: a composable QiQuery<Child> of every row
//                       that points back at this one (client.projects().all()).
//   - QI_MANY_TO_MANY   a set across a join table, with add()/remove()/contains()
//                       (Project <-> Employee, for the *formal* staffing roster —
//                       see TimeEntry below for the *actual worked hours* signal,
//                       which is deliberately a separate, looser relationship).
//
// Only Client and Project are exposed to QML as single gadget records (`store.client`,
// `store.project`), so only those two carry Q_GADGET + QI_QML_FIELD. The rest
// (Contact, Opportunity, TimeEntry, Expense, Invoice) are plain QiModel classes —
// QiListModel reads their fields by name via reflection, no Q_GADGET required —
// which is also why their foreign keys can be real `QiForeignKey<T>` members
// instead of a QML-property-shaped `int clientId`.

class Client; class Contact; class Opportunity; class Project;
class TimeEntry; class Expense; class Invoice;

/// A staff member: the FK target for who managed/logged/staffed what. List-only
/// (via `store.employees`) — never a single gadget record, so plain QiModel.
class Employee : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> title;
    QiField<QString> email;
    QiField<double>  billRate;   // $/hr charged to clients
    QiField<double>  costRate;   // $/hr internal cost
    QiField<bool>    active;

    QI_HAS_MANY(TimeEntry, timeEntries, "employee")   // employee.timeEntries().all()/.count()
    QI_HAS_MANY(Expense,   expenses,    "employee")
    QI_MANY_TO_MANY(Project, staffedProjects, "project_team")   // reverse of Project::teamMembers()
};
QI_DECLARE_MODEL(Employee, "employee",
    QI_FIELD(name), QI_FIELD(title), QI_FIELD(email), QI_FIELD(billRate),
    QI_FIELD(costRate), QI_FIELD(active));

/// A customer company (the CRM "account"). Exposed to QML as a single gadget
/// (`store.client`) as well as via QiListModel rows (`store.clients`).
class Client : public QiModel {
    Q_GADGET
    QI_MODEL
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(QString, industry)
    QI_QML_FIELD(QString, city)
    QI_QML_FIELD(QString, state)
    QI_QML_FIELD(QString, phone)

    QI_HAS_MANY(Contact,      contacts,      "client")   // client.contacts().all()
    QI_HAS_MANY(Opportunity,  opportunities, "client")
    QI_HAS_MANY(Project,      projects,      "client")
};
QI_DECLARE_MODEL(Client, "client",
    QI_FIELD(name), QI_FIELD(industry), QI_FIELD(city), QI_FIELD(state), QI_FIELD(phone));
Q_DECLARE_METATYPE(Client)          // so it can be a Q_PROPERTY value in QML

/// A client engagement. Exposed to QML as a single gadget (`store.project`) as
/// well as via QiListModel rows (`store.projects`). `client`/`manager` are real
/// QiForeignKey fields; `clientId`/`managerId` below are thin read-only
/// Q_PROPERTYs over them so `store.project.clientId` keeps working from QML
/// (QiForeignKey<T> itself isn't a QML-representable value type).
class Project : public QiModel {
    Q_GADGET
    QI_MODEL
    Q_PROPERTY(int clientId  READ clientIdRole)
    Q_PROPERTY(int managerId READ managerIdRole)
    QI_QML_FIELD(QString, code)       // short code, e.g. "PRJ-1042"
    QI_QML_FIELD(QString, name)
    QI_QML_FIELD(Erp::ProjectStatus, status)
    QI_QML_FIELD(QString, startDate)  // yyyy-MM-dd
    QI_QML_FIELD(double,  budget)
public:
    QiForeignKey<Client>   client;     // -> client(id); client->name auto-loads
    QiForeignKey<Employee> manager;    // -> employee(id)
    int clientIdRole()  const { return client.get().toInt(); }
    int managerIdRole() const { return manager.get().toInt(); }

    QI_HAS_MANY(TimeEntry, timeEntries, "project")   // project.timeEntries().all()
    QI_HAS_MANY(Expense,   expenses,    "project")
    QI_HAS_MANY(Invoice,   invoices,    "project")
    QI_MANY_TO_MANY(Employee, teamMembers, "project_team")   // formal staffing roster
};
QI_DECLARE_MODEL(Project, "project",
    QI_FIELD(client), QI_FIELD(manager), QI_FIELD(code), QI_FIELD(name),
    QI_FIELD(status), QI_FIELD(startDate), QI_FIELD(budget));
Q_DECLARE_METATYPE(Project)

/// A person at a client company. List-only (via `store.contacts`) — no gadget
/// exposure needed, so it's a plain QiModel with a real foreign key.
class Contact : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Client> client;      // -> client(id)
    QiField<QString> name;
    QiField<QString> title;
    QiField<QString> email;
    QiField<QString> phone;
};
QI_DECLARE_MODEL(Contact, "contact",
    QI_FIELD(client), QI_FIELD(name), QI_FIELD(title), QI_FIELD(email), QI_FIELD(phone));

/// A deal in the sales pipeline. List-only, plain QiModel.
class Opportunity : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Client> client;      // -> client(id)
    QiField<QString> name;
    QiField<Erp::OppStage> stage;
    QiField<double> amount;
    QiField<QString> closeDate;       // yyyy-MM-dd (expected or actual)
};
QI_DECLARE_MODEL(Opportunity, "opportunity",
    QI_FIELD(client), QI_FIELD(name), QI_FIELD(stage), QI_FIELD(amount), QI_FIELD(closeDate));

/// An hours entry against a project. `invoiceId` is a plain int (0 = unbilled),
/// not a QiForeignKey — it is an optional back-reference filled in after the
/// fact, not a required "this row must point somewhere" relationship.
class TimeEntry : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Project>  project;   // -> project(id)
    QiForeignKey<Employee> employee;  // -> employee(id)
    QiField<QString> date;            // yyyy-MM-dd
    QiField<double>  hours;
    QiField<bool>    billable;
    QiField<QString> notes;
    QiField<int>     invoiceId;       // 0 = not yet invoiced
};
QI_DECLARE_MODEL(TimeEntry, "time_entry",
    QI_FIELD(project), QI_FIELD(employee), QI_FIELD(date), QI_FIELD(hours),
    QI_FIELD(billable), QI_FIELD(notes), QI_FIELD(invoiceId));

/// A reimbursable cost entry against a project. Same invoiceId convention as TimeEntry.
class Expense : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Project>  project;
    QiForeignKey<Employee> employee;
    QiField<QString> date;            // yyyy-MM-dd
    QiField<double>  amount;
    QiField<QString> category;
    QiField<bool>    billable;
    QiField<int>     invoiceId;       // 0 = not yet invoiced
};
QI_DECLARE_MODEL(Expense, "expense",
    QI_FIELD(project), QI_FIELD(employee), QI_FIELD(date), QI_FIELD(amount),
    QI_FIELD(category), QI_FIELD(billable), QI_FIELD(invoiceId));

/// A bill sent to a client, generated from a project's unbilled time + expenses.
class Invoice : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Client>  client;
    QiForeignKey<Project> project;
    QiField<QString> number;          // e.g. "INV-1007"
    QiField<QString> issueDate;       // yyyy-MM-dd
    QiField<QString> dueDate;         // yyyy-MM-dd
    QiField<Erp::InvoiceStatus> status;
    QiField<double>  amount;
};
QI_DECLARE_MODEL(Invoice, "invoice",
    QI_FIELD(client), QI_FIELD(project), QI_FIELD(number), QI_FIELD(issueDate),
    QI_FIELD(dueDate), QI_FIELD(status), QI_FIELD(amount));

#endif // MODELS_H
