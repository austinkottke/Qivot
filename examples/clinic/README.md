# Tutorial — Qivot Clinic: a basic EHR (scheduler + patient chart)

A small but real **electronic health record** across three views: a
**practice-overview dashboard** with live analytics, a searchable patient
directory with a full clinical **chart**, and a **day-calendar scheduler** you can
book into — with a booking rule that refuses to double-book a provider. You can
also **add patients, notes, and vitals**, and **full-text search every clinical
note**. It's the example that pulls most of Qivot together in one app. **All data
is synthetic** (generated at startup); there are no real patients.

<img src="../../docs/clinic-demo.gif" alt="Qivot Clinic — patient chart and day scheduler with transitions" width="820">

> **Run it**
> ```sh
> cd examples/clinic
> qmake && make
> ./clinic
> ```

## How QML gets the data (no hand-mapping)

Query results are exposed as **`QiListModel`s** — the roles come straight from the
model's fields, so there is **no per-row copying** in C++:

```cpp
m_problems.setList(QiQuery<Problem>().filter(QiWhere("patientId = ", id)).all());
```
```qml
Repeater { model: store.problems
    Text { text: model.name + "  ·  " + model.status } }   // roles == field names
```

The selected chart is exposed as **raw gadget values** (`Q_GADGET` + `QI_QML_FIELD`
in [`models.h`](models.h)), so QML reads fields directly:

```qml
Text { text: store.patient.firstName + " " + store.patient.lastName }
```

Joins and formatting — a provider's name, a time label, an age — are tiny
invokables (`store.providerName(id)`, `store.minuteLabel(min)`, `store.ageOf(dob)`),
not fields copied onto every row.

## What it shows off

| Feature | Where |
| --- | --- |
| **Seven related models** | `Patient`, `Provider`, `Appointment`, `Vital`, `Problem`, `Medication`, `Note` ([`models.h`](models.h)) |
| **Joins across tables** | every chart panel and the schedule stitch patient + provider + rows together ([`clinicstore.cpp`](clinicstore.cpp)) |
| **A transaction with a business rule** | `book()` opens a `QiTransaction` and **rolls back** rather than double-book a provider |
| **Aggregate analytics** | the Overview tab uses `count()`, `call("avg", …)`, and `select({"col","count(*)"}) + groupBy()` for per-day, per-provider, status, and top-condition rollups |
| **Full-text search (FTS5)** | a `QiFtsIndex` over clinical notes; "Search notes" runs `QiQuery::search()` and stays in sync as notes are added |
| **Writes from the UI** | add a patient, a note, or a set of vitals — each an `insert` that ripples to every view |
| **Filtered search** | the patient directory filters live with `LIKE` across name + MRN |
| **Status updates that ripple** | checking a patient in refreshes the schedule, the chart, and the directory |

## The schema (Step 1)

Seven plain-C++ models. `patientId` / `providerId` are foreign keys (plain ints).
Dates are stored as sortable `"yyyy-MM-dd"` strings and times as *minutes from
midnight*, which makes the day/agenda queries trivial:

```cpp
class Appointment : public QiModel {
    QI_MODEL
public:
    QiField<int>     patientId;
    QiField<int>     providerId;
    QiField<QString> day;         // "2026-07-24"
    QiField<int>     minute;      // 9:30am = 570
    QiField<int>     durationMin;
    QiField<QString> reason;
    QiField<QString> status;      // scheduled | arrived | completed | cancelled
};
```

## Building a panel from a query (Step 2)

The controller runs a query and hands QML a plain list of maps, so joined and
computed fields (provider name, the age, a time label) are easy to bind:

```cpp
// one day's schedule, ordered by time
QiList<Appointment> appts = QiQuery<Appointment>()
        .filter(QiWhere("day = ", m_day)).orderBy("minute asc").all();
for (int i = 0; i < appts.size(); i++) {
    Appointment *a = appts.at(i);
    QVariantMap m;
    m["patientName"] = patientName(a->patientId.get().toInt());   // a tiny join
    m["timeLabel"]   = minuteLabel(a->minute.get().toInt());
    m["status"]      = a->status.get().toString();
    m_schedule << m;
}
```

## The booking transaction (Step 3 — the important one)

Booking must be all-or-nothing *and* must not double-book a provider. That's a
textbook use of a transaction:

```cpp
bool ClinicStore::book(int patientId, int providerId, int minute, int dur, ...) {
    QiTransaction txn;                       // BEGIN

    // does this provider already have an overlapping, non-cancelled visit today?
    QiList<Appointment> same = QiQuery<Appointment>()
        .filter(QiWhere("day = ", m_day) && QiWhere("providerId = ", providerId)
                && QiWhere("status <> ", "cancelled")).all();
    for (int i = 0; i < same.size(); i++) {
        int s = same.at(i)->minute.get().toInt();
        int e = s + same.at(i)->durationMin.get().toInt();
        if (minute < e && s < minute + dur) {      // overlap
            txn.rollback();                          // ← undo, refuse the booking
            m_lastError = "…already booked at …";
            return false;
        }
    }

    Appointment a; /* set fields */ a.save();
    txn.commit();                            // COMMIT
    return true;
}
```

Try it in the app: open **New appointment**, pick a provider and a time they're
already booked — the booking is refused and tells you why.

## Analytics with aggregates (the Overview tab)

The dashboard rolls the data up with the ORM's aggregate API — `count()`, `call()`,
and `groupBy()` — no hand-written SQL:

```cpp
int patients = Patient::objects().count();
int arrived  = QiQuery<Appointment>()
    .filter(QiWhere("day = ", today) && QiWhere("status = ", "arrived")).count();

// appointments per day this week — a GROUP BY, read row by row
QiQuery<Appointment> q = QiQuery<Appointment>()
    .filter(QiWhere("day >= ", today) && QiWhere("status <> ", "cancelled"))
    .select(QStringList() << "day" << "count(*)").groupBy("day");
if (q.exec()) while (q.next())
    perDay[q.value(0).toString()] = q.value(1).toInt();   // value(0)=group, value(1)=count
```

## Searching notes (FTS5)

One index makes every clinical note searchable, and it **stays in sync
automatically** — a note added from the UI is findable a keystroke later:

```cpp
QiFtsIndex<Note> fts("note_fts");  fts << "body" << "kind";
connection.createFtsIndex(fts);

QiList<Note> hits = QiQuery<Note>().search("note_fts", "cholesterol*").all();  // ranked
```

## Try this in the app

1. **Overview tab** — live KPIs and charts (appointments per day, provider load,
   today's status, top conditions), all from aggregate queries. Then **search the
   clinical notes** — try "cholesterol", "asthma", or "physical therapy".
2. **Patients tab** — search a name, click a patient; the whole chart **animates in**.
   Hit **＋ Add vitals** or **＋ Add note** — the chart (and the note search, and the
   Overview counts) update immediately. **＋ New** adds a patient.
3. **Schedule tab** — the pill **slides**, the view **crossfades**; drag **‹ / ›** and
   watch the appointment blocks **stagger in**.
4. **Click an appointment** → *Check in* / *Complete* / *Cancel* (recolors instantly).
5. **＋ New appointment** → book a clash on purpose to see the transaction guard.

## Files

| File | Role |
|---|---|
| `models.h` | The seven models (the schema). |
| `main.cpp` | Opens the DB, generates the synthetic clinic, loads the UI. |
| `clinicstore.h` / `.cpp` | All the queries; builds chart + schedule; the booking transaction. |
| `main.qml` | The two views, the calendar grid, dialogs, and the transitions. |

## See also

- [`savepoints`](../savepoints) — nested transactions in depth.
- [`relations`](../relations) — first-class one-to-many / many-to-many helpers.
- [`reactive`](../reactive) — a list that redraws itself on every change.
