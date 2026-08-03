# vision — a mock "Deltek Vision" reporting demo

> ⚠️ **This does NOT connect to a real Deltek Vision database.** All data here is
> **synthetic** — invented firms, people, and numbers generated at startup. It runs
> on **SQLite** (and is proven against a plain SQL Server container). There is no
> Deltek software, no `VisionDemo76`, and no Deltek data anywhere in this example.
>
> What's "Vision" about it is the **schema shape**: the table codes (`CL`/`EM`/`PR`/
> `LD`), the natural string keys, and the column names mirror Vision's, so the *same
> Qivot models* would map onto a real Vision database. Making that connection is on
> you and needs a licensed Vision DB — see
> [Pointing it at a real Vision database](#pointing-it-at-a-real-vision-database) below.

Deltek Vision (the A/E/C professional-services ERP) stores its data in **Microsoft
SQL Server**. Qivot speaks SQL Server (`QODBC` → `QiMsSqlStatement`), so the same
models and reporting code run **unchanged** against the SQLite mock here and against
a real Vision database.

This example maps four of Vision's core tables to Qivot models and produces two
classic Vision reports:

- **Project Earnings** — contract fee vs. billed labor vs. cost vs. margin, per project.
- **Employee Utilization** — total hours, billable hours, and utilization % per active staffer.

## The desktop app

`./vision` (no arguments) launches a Qt Quick dashboard on an in-memory SQLite
database, using Deltek Vision's own vocabulary — **Revenue** (job-to-date labor
value), **Compensation** (direct labor cost), **Profit**, and the **Effective
Multiplier** (revenue ÷ compensation):

- **Overview** — a Vision-style **Key Financial Metrics** panel (YTD Revenue/Profit,
  Effective Multiplier, Compensation, YTD Billed, Total Unbilled, Outstanding A/R,
  Cash on Hand, Backlog) plus earnings and utilization snapshots.
- **Projects** — earnings **grouped by Organization** (Vision's org structure), with
  per-org profit and multiplier subtotals, Principal-In-Charge vs. Project Manager,
  and status pills. This is Vision's "Project Earnings by Org" report.
- **Visualization** — a **Company Project Visualization** treemap: tiles grouped by
  organization, **sized by Compensation** and **coloured by Profit** (red → blue),
  mirroring Vision's dashboard widget.
- **Utilization** — per-employee gauges (billable ÷ total hours).
- **Labor Ledger** — a **live `QiListModel`** over the LD table. Hit **Post labor**,
  add an entry, and every metric, report, and the treemap recompute instantly — the
  reactive-model path.

It ships in **dark mode** with a light/dark toggle (top-right). Reports are `GROUP BY`
aggregate queries (`qiRawQuery`), the billing/AR metrics come from the `BI` ledger,
the labor list is a reactive `QiListModel`, and the dialog's controls are themed via
Qt Quick Controls' Material style. The schema adds an **Organization** on each project
and a **`BI` billing/AR table** (invoiced + received) to drive the JTD metrics.

## Console modes (no GUI)

```bash
qmake && make

./vision --report           # print the two reports to the terminal
./vision --ddl sqlserver    # the CREATE TABLE T-SQL Qivot generates (no DB needed)
./vision --script sqlserver # a full runnable script: DDL + seed + reports
```

`<dialect>` = `sqlite` | `sqlserver` | `postgres` | `mysql` | `oracle` | `duckdb`.

### Proving it on a real SQL Server

The `--script` output for `sqlserver` is entirely Qivot-generated DDL plus seed and
report SQL. Pipe it into a genuine SQL Server and the reports come back from the
real engine:

```bash
# a throwaway SQL Server 2022 (Apple Silicon runs the amd64 image under emulation)
docker run -d --name mssql -e ACCEPT_EULA=Y -e MSSQL_SA_PASSWORD=Qivot_Test1 \
  -p 1433:1433 mcr.microsoft.com/mssql/server:2022-latest

docker exec mssql /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P Qivot_Test1 -C \
  -Q "IF DB_ID('vision_demo') IS NULL CREATE DATABASE vision_demo;"

./vision --script sqlserver | docker exec -i mssql \
  /opt/mssql-tools18/bin/sqlcmd -S localhost -U sa -P Qivot_Test1 -C -d vision_demo
```

The numbers match the SQLite run exactly — same models, same reports, different engine.

## The schema mapping

Faithful to Vision:

| Qivot model | Table | Key | Notes |
|-------------|-------|-----|-------|
| `Cl` | `CL` | `ClientID` (natural string) | client master |
| `Em` | `EM` | `Employee` (natural string) | employee master |
| `Pr` | `PR` | `WBS1` (natural string) | project master — carries `Org` and `Principal` (Principal-In-Charge) alongside `ProjMgr` |
| `Ld` | `LD` | `id` (auto-increment) | posted labor ledger |
| `Bi` | `BI` | `id` (auto-increment) | billing/AR ledger (invoiced + received) |

- The masters key on **natural string keys with no identity column** — exactly like
  Vision. On SQL Server that generates `NVARCHAR(450) NOT NULL PRIMARY KEY`; the
  transactional `LD` gets `id INT IDENTITY(1,1)`, exercising the SQL Server
  `IDENTITY` / `@@IDENTITY` path.
- Column names match Vision (`WBS1`, `ProjMgr`, `RegHrs`, `TransDate`, …).

Simplified (called out honestly):

- Vision projects key on the **WBS triplet** (`WBS1`/`WBS2`/`WBS3` = project/phase/
  task). This models the `WBS1` (project) grain only.
- Vision derives bill/cost rates from **rate tables**; here each employee carries one
  rate, and `LD` stores the rate in effect at posting. Real Vision `LD` also stores
  the pre-extended `BillExt`/`CostExt` amounts.
- `Status` is Vision's single-char code: `A` active, `I` inactive, `D` dormant.

## Pointing it at a real Vision database

Nothing here talks to Deltek — but because the models mirror Vision's schema, the
swap is a **connection change**, not a rewrite. If you have a Vision install (or
Deltek's downloadable `VisionDemo76` sample DB, restored to SQL Server):

1. Replace the in-memory SQLite connection in `main.cpp` with a QODBC connection to
   your Vision SQL Server (see the top-level README / `examples/multidb` for the
   `QODBC` connection string), using a **read-only** account.
2. **Do not** call `createTables()` / `dropTables()` — the Vision schema already
   exists and is owned by Deltek. Just `addModel<>()` and run the report queries.
3. Read from Vision's **reporting views** where you can rather than the base tables,
   and treat writes as off-limits — Vision's tables have triggers and posting logic;
   direct writes are unsupported by Deltek. For writes, go through Vision's Web
   Services API, not Qivot.

The demo models simplify Vision (see *Simplified* above), so a real DB may need
column tweaks — but the reporting shape (`GROUP BY` over `LD`, joined to `PR`/`EM`)
is the same.

> All data in this example is **synthetic** — a schema-shaped teaching mock, not
> Deltek code or data.
