#ifndef SEEDDATA_H
#define SEEDDATA_H

// Shared synthetic data + report SQL for the vision demo, used by both the
// console proof (main.cpp: --script / --report) and the QML dashboard
// (visionstore.cpp). All data is SYNTHETIC — no real firms, people, or Deltek
// content. The schema shape (table codes, natural keys, column names, the
// Organization dimension, JTD/YTD metrics) mirrors Deltek Vision; see README.md.

// ---- reports, as portable ANSI SQL (identical on SQLite & SQL Server) ---------

// Project earnings, now carrying the Organization (Org) so results roll up by org
// the way Vision's "Project Earnings by Org" report does. "Billed" here is the
// job-to-date value of labor (Σ hours × bill rate); "Compensation" is direct
// labor cost (Σ hours × cost rate), in Vision's terminology.
static const char *kEarningsSql =
    "SELECT pr.WBS1 AS WBS1, pr.Name AS Name, pr.Org AS Org, pr.Status AS Status, "
    "pr.ProjMgr AS ProjMgr, pr.Principal AS Principal, pr.ClientID AS ClientID, pr.Fee AS Fee, "
    "COALESCE(SUM(ld.RegHrs * ld.BillRate), 0) AS Billed, "
    "COALESCE(SUM(ld.RegHrs * ld.CostRate), 0) AS Cost "
    "FROM PR pr LEFT JOIN LD ld ON ld.WBS1 = pr.WBS1 "
    "WHERE pr.ChargeType = 'R' "
    "GROUP BY pr.WBS1, pr.Name, pr.Org, pr.Status, pr.ProjMgr, pr.Principal, pr.ClientID, pr.Fee "
    "ORDER BY pr.Org, pr.WBS1";

static const char *kUtilSql =
    "SELECT em.Employee AS Employee, em.LastName AS LastName, em.FirstName AS FirstName, "
    "em.Title AS Title, "
    "COALESCE(SUM(ld.RegHrs), 0) AS TotalHrs, "
    "COALESCE(SUM(CASE WHEN ld.BillRate > 0 THEN ld.RegHrs ELSE 0 END), 0) AS BillableHrs "
    "FROM EM em LEFT JOIN LD ld ON ld.Employee = em.Employee "
    "WHERE em.Status = 'A' "
    "GROUP BY em.Employee, em.LastName, em.FirstName, em.Title "
    "ORDER BY em.Employee";

// ---- one source of truth for the synthetic demo firm --------------------------

struct ClRow { const char *ClientID, *Name, *Type, *Status; };
struct EmRow { const char *Employee, *LastName, *FirstName, *Title, *Status; double BillRate, CostRate; };
// PR now has Org (organization the project rolls up to) and Principal (Principal-
// In-Charge), both real Vision fields, in addition to the Project Manager.
struct PrRow { const char *WBS1, *Name, *Org, *Status, *ProjMgr, *Principal, *ClientID; double Fee; const char *ChargeType; };
struct LdRow { const char *WBS1, *Employee, *TransDate; double RegHrs, BillRate, CostRate; };
// BI — billing/AR ledger: an issued invoice and how much has been received.
struct BiRow { const char *WBS1, *InvoiceDate; double Amount, Received; };

static const ClRow kClients[] = {
    { "C001", "Riverside Municipality",    "Government", "A" },
    { "C002", "Harbor Development LLC",     "Private",    "A" },
    { "C003", "Northgate School District",  "Government", "A" },
};
static const EmRow kEmployees[] = {
    { "E001", "Vance", "Dana",  "Principal",         "A", 225, 95 },
    { "E002", "Ruiz",  "Marco", "Project Architect", "A", 165, 72 },
    { "E003", "Cole",  "Priya", "Engineer",          "A", 140, 60 },
    { "E004", "Osei",  "Kwame", "Designer",          "A", 110, 48 },
    { "E005", "Bauer", "Lena",  "Admin",             "I",   0, 40 },  // inactive
    { "E006", "Nash",  "Ivy",   "Senior Engineer",   "A", 155, 82 },
};
// Three organizations (Vision groups projects under an Org, e.g. "Chicago
// Engineering"); here they're by discipline. Overhead sits outside the orgs.
static const PrRow kProjects[] = {
    { "2024001.00", "Riverside Bridge Rehab",       "Civil Engineering", "A", "E001", "E001", "C001", 480000, "R" },
    { "2024007.00", "Route 9 Interchange",          "Civil Engineering", "A", "E006", "E001", "C001", 260000, "R" },
    { "2023045.00", "Downtown Transit Study",       "Civil Engineering", "D", "E001", "E001", "C001",  95000, "R" },  // dormant
    { "2024002.00", "Harbor Mixed-Use Master Plan", "Architecture",      "A", "E002", "E002", "C002", 320000, "R" },
    { "2024003.00", "Northgate STEM Wing Addition", "Architecture",      "A", "E006", "E002", "C003", 615000, "R" },
    { "2024011.00", "Midtown Library Renovation",   "Architecture",      "A", "E004", "E002", "C003", 180000, "R" },
    { "2024005.00", "Cedar Creek Watershed Study",  "Environmental",     "A", "E003", "E006", "C001", 240000, "R" },
    { "2024009.00", "Portside Terminal Expansion",  "Environmental",     "A", "E006", "E006", "C002", 390000, "R" },
    { "OVH.00",     "Overhead / PTO",               "Overhead",          "A", "E001", "E001", "",           0, "H" },
};
static const LdRow kLabor[] = {
    // Riverside Bridge Rehab
    { "2024001.00", "E001", "2026-06-02",  40, 225, 95 },
    { "2024001.00", "E002", "2026-06-05", 120, 165, 72 },
    { "2024001.00", "E003", "2026-06-09", 200, 140, 60 },
    { "2024001.00", "E004", "2026-06-12", 160, 110, 48 },
    // Route 9 Interchange
    { "2024007.00", "E006", "2026-06-04",  90, 155, 82 },
    { "2024007.00", "E003", "2026-06-11",  60, 140, 60 },
    { "2024007.00", "E004", "2026-06-17",  40, 110, 48 },
    // Downtown Transit Study (dormant)
    { "2023045.00", "E001", "2026-05-14",  30, 225, 95 },
    { "2023045.00", "E003", "2026-05-20",  80, 140, 60 },
    // Harbor Mixed-Use Master Plan
    { "2024002.00", "E002", "2026-06-16",  90, 165, 72 },
    { "2024002.00", "E003", "2026-06-19",  60, 140, 60 },
    { "2024002.00", "E004", "2026-06-23", 220, 110, 48 },
    // Northgate STEM Wing Addition
    { "2024003.00", "E006", "2026-06-03", 150, 155, 82 },
    { "2024003.00", "E002", "2026-06-10",  70, 165, 72 },
    { "2024003.00", "E004", "2026-06-18", 130, 110, 48 },
    // Midtown Library Renovation
    { "2024011.00", "E004", "2026-06-06", 120, 110, 48 },
    { "2024011.00", "E002", "2026-06-13",  40, 165, 72 },
    // Cedar Creek Watershed Study
    { "2024005.00", "E003", "2026-06-07", 100, 140, 60 },
    { "2024005.00", "E006", "2026-06-14",  60, 155, 82 },
    // Portside Terminal Expansion
    { "2024009.00", "E006", "2026-06-08", 130, 155, 82 },
    { "2024009.00", "E003", "2026-06-15",  90, 140, 60 },
    { "2024009.00", "E004", "2026-06-20",  60, 110, 48 },
    // Overhead / PTO (non-billable)
    { "OVH.00",     "E001", "2026-06-27",  10,   0, 95 },
    { "OVH.00",     "E003", "2026-06-26",  20,   0, 60 },
    { "OVH.00",     "E004", "2026-06-25",  40,   0, 48 },
    { "OVH.00",     "E006", "2026-06-24",  16,   0, 82 },
};
// Issued invoices + cash received (drives JTD Billed, Unbilled, and Outstanding AR).
static const BiRow kBilling[] = {
    { "2024001.00", "2026-06-15", 60000, 40000 },
    { "2024007.00", "2026-06-20", 20000, 15000 },
    { "2023045.00", "2026-05-31", 17950, 17950 },   // dormant: billed & collected in full
    { "2024002.00", "2026-06-25", 38000, 30000 },
    { "2024003.00", "2026-06-22", 40000, 25000 },
    { "2024011.00", "2026-06-18", 15000, 12000 },
    { "2024005.00", "2026-06-19", 18000, 10000 },
    { "2024009.00", "2026-06-24", 30000, 24000 },
};
// Illustrative firm-level cash position (Vision reads this from the GL; synthetic here).
static const double kCashBase = 1200000.0;

#endif // SEEDDATA_H
