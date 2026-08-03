#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// ---------------------------------------------------------------------------
// A mock of Deltek Vision's core schema, mapped to Qivot models.
//
// Deltek Vision (the A/E/C professional-services ERP) runs on Microsoft SQL
// Server, which Qivot speaks via QODBC -> QiMsSqlStatement. These four models
// mirror Vision's real table codes and column names so the same Qivot code that
// runs here on SQLite maps 1:1 onto a real Vision database — you'd point the
// connection at VisionDemo76 (Deltek's downloadable sample DB) and drop the
// mock. See README.md for exactly which parts are faithful vs. simplified.
//
// The Vision-faithful bits:
//   - Table codes are Vision's:  CL (client), EM (employee), PR (project),
//     LD (labor ledger detail).
//   - Master tables key on NATURAL string keys (ClientID, Employee, WBS1) with
//     NO identity column — exactly like Vision. In Qivot that's
//     QI_DECLARE_MODEL_NOID + a QiPrimary string field.
//   - Column names match Vision (WBS1, ProjMgr, RegHrs, TransDate, ...).
//
// The simplifications (called out honestly):
//   - Vision projects key on the WBS triplet (WBS1/WBS2/WBS3 = project/phase/
//     task). We model at the WBS1 (project) grain only.
//   - Vision derives bill/cost rates from rate tables; we store one rate per
//     employee. The posted-labor table LD stores hours + the rate in effect;
//     real Vision LD also carries the pre-extended BillExt/CostExt amounts.
//   - Status is Vision's single-char code: 'A' active, 'I' inactive, 'D' dormant.
// ---------------------------------------------------------------------------

/// CL — Client master. Natural key: ClientID (no identity column, like Vision).
class Cl : public QiModel {
    QI_MODEL
public:
    QiField<QString> ClientID;   // e.g. "C001"  (primary key)
    QiField<QString> Name;
    QiField<QString> Type;       // "Government" / "Private" / ...
    QiField<QString> Status;     // 'A' / 'I'
};
QI_DECLARE_MODEL_NOID(Cl, "CL",
    QI_FIELD(ClientID, QiPrimary | QiNotNull),
    QI_FIELD(Name), QI_FIELD(Type), QI_FIELD(Status));

/// EM — Employee master. Natural key: Employee (the employee number).
class Em : public QiModel {
    QI_MODEL
public:
    QiField<QString> Employee;   // e.g. "E001"  (primary key)
    QiField<QString> LastName;
    QiField<QString> FirstName;
    QiField<QString> Title;
    QiField<QString> Status;     // 'A' / 'I'
    QiField<double>  BillRate;   // $/hr billed  (Vision: derived from rate tables)
    QiField<double>  CostRate;   // $/hr internal cost
};
QI_DECLARE_MODEL_NOID(Em, "EM",
    QI_FIELD(Employee, QiPrimary | QiNotNull),
    QI_FIELD(LastName), QI_FIELD(FirstName), QI_FIELD(Title),
    QI_FIELD(Status), QI_FIELD(BillRate), QI_FIELD(CostRate));

/// PR — Project master. Natural key: WBS1 (project number). ProjMgr and ClientID
/// are string references into EM.Employee and CL.ClientID (Vision joins on these
/// natural keys, not surrogate ids).
class Pr : public QiModel {
    QI_MODEL
public:
    QiField<QString> WBS1;       // e.g. "2024001.00"  (primary key)
    QiField<QString> Name;
    QiField<QString> Org;        // organization the project rolls up to
    QiField<QString> Status;     // 'A' active / 'I' inactive / 'D' dormant
    QiField<QString> ProjMgr;    // -> EM.Employee (Project Manager)
    QiField<QString> Principal;  // -> EM.Employee (Principal-In-Charge)
    QiField<QString> ClientID;   // -> CL.ClientID
    QiField<double>  Fee;        // negotiated contract fee (budget)
    QiField<QString> ChargeType; // 'R' regular / 'N' non-billable / 'H' overhead
};
QI_DECLARE_MODEL_NOID(Pr, "PR",
    QI_FIELD(WBS1, QiPrimary | QiNotNull),
    QI_FIELD(Name), QI_FIELD(Org), QI_FIELD(Status), QI_FIELD(ProjMgr),
    QI_FIELD(Principal), QI_FIELD(ClientID), QI_FIELD(Fee), QI_FIELD(ChargeType));

/// LD — Ledger Detail (posted labor). Transactional, so unlike the masters it
/// carries a surrogate auto-increment id — which on SQL Server exercises the
/// IDENTITY / @@IDENTITY path. WBS1 and Employee reference PR and EM.
class Ld : public QiModel {
    QI_MODEL
public:
    QiField<QString> WBS1;       // -> PR.WBS1
    QiField<QString> Employee;   // -> EM.Employee
    QiField<QString> TransDate;  // yyyy-MM-dd
    QiField<double>  RegHrs;     // regular hours posted
    QiField<double>  BillRate;   // rate in effect at posting
    QiField<double>  CostRate;
};
QI_DECLARE_MODEL(Ld, "LD",
    QI_FIELD(WBS1), QI_FIELD(Employee), QI_FIELD(TransDate),
    QI_FIELD(RegHrs), QI_FIELD(BillRate), QI_FIELD(CostRate));

/// BI — billing/AR ledger: an issued invoice against a project and the cash
/// received against it. Drives JTD Billed, Unbilled, and Outstanding AR.
class Bi : public QiModel {
    QI_MODEL
public:
    QiField<QString> WBS1;        // -> PR.WBS1
    QiField<QString> InvoiceDate; // yyyy-MM-dd
    QiField<double>  Amount;      // invoiced
    QiField<double>  Received;    // cash collected against this invoice
};
QI_DECLARE_MODEL(Bi, "BI",
    QI_FIELD(WBS1), QI_FIELD(InvoiceDate), QI_FIELD(Amount), QI_FIELD(Received));

// ---- Report row shapes (used only as typed containers for qiRawQuery) --------
// These are never created as tables; their fields just name the SELECT columns.

/// One row of the Project Earnings report: contract fee vs. billed vs. cost.
class ProjectEarnings : public QiModel {
    QI_MODEL
public:
    QiField<QString> WBS1;
    QiField<QString> Name;
    QiField<QString> Org;
    QiField<QString> Status;
    QiField<QString> ProjMgr;
    QiField<QString> Principal;
    QiField<QString> ClientID;
    QiField<double>  Fee;
    QiField<double>  Billed;
    QiField<double>  Cost;
};
QI_DECLARE_MODEL(ProjectEarnings, "project_earnings",
    QI_FIELD(WBS1), QI_FIELD(Name), QI_FIELD(Org), QI_FIELD(Status), QI_FIELD(ProjMgr),
    QI_FIELD(Principal), QI_FIELD(ClientID), QI_FIELD(Fee), QI_FIELD(Billed), QI_FIELD(Cost));

/// One row of the Employee Utilization report.
class EmpUtil : public QiModel {
    QI_MODEL
public:
    QiField<QString> Employee;
    QiField<QString> LastName;
    QiField<QString> FirstName;
    QiField<QString> Title;
    QiField<double>  TotalHrs;
    QiField<double>  BillableHrs;
};
QI_DECLARE_MODEL(EmpUtil, "emp_util",
    QI_FIELD(Employee), QI_FIELD(LastName), QI_FIELD(FirstName),
    QI_FIELD(Title), QI_FIELD(TotalHrs), QI_FIELD(BillableHrs));

#endif // MODELS_H
