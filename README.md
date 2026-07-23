<h1 align="center">Qivot</h1>

<p align="center"><strong>The Qt ORM with JSON-over-HTTP built in.</strong></p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white" alt="C++17">
  <img src="https://img.shields.io/badge/Qt-5.15%20%7C%206-41CD52?logo=qt&logoColor=white" alt="Qt 5.15 or 6">
  <img src="https://img.shields.io/badge/SQLite-FTS5-003B57?logo=sqlite&logoColor=white" alt="SQLite">
  <img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT">
</p>

<p align="center">
  <img src="docs/contacts-hero.png" alt="Qivot Contacts example — an iOS-style address book over 10,000 records" width="460">
  <br>
  <em><a href="examples/contacts">Contacts example</a> — an iOS-style address book over 10,000 live records: sticky A–Z sections, drag-to-jump index, and reactive search, all backed by SQLite through Qivot.</em>
</p>

Qivot is a modern **C++17 ORM for Qt and SQLite**. Declare your models as plain
C++/Qt classes — no SQL, no `QObject` — then query, join, and full-text search
them through a typed C++ API. And where a plain ORM stops, Qivot keeps going: it
**maps a JSON schema straight into your models** and **pulls a REST/JSON API over
HTTP on a worker thread**, writing the results into your database.

## ✨ Highlights

- 🧩 **Plain-C++ models** — declare a class, get a table. No SQL, no `QObject`.
- 🔑 **[Flexible primary keys](#custom-primary-keys-string-ids-no-auto-id)** — an
  auto-increment `id` by default, or key the table on your own string id (a ULID,
  a `userId`) or a composite key with `QiPrimary`; foreign keys reference it and
  support `ON DELETE` actions.
- 🔎 **Rich, typed queries** — filters, ordering, aggregates, and full
  **[JOINs](#joins)** (INNER / LEFT / RIGHT / FULL / CROSS, auto-foreign-key,
  `distinct()`) — all `auto`-friendly.
- 🌐 **[JSON & REST](#json-mapping)** — map JSON to/from models, or
  **[fetch an API over HTTP on a worker thread](#loading-json-over-http-on-a-worker-thread)**
  and save it, without blocking your UI.
- 🔁 **[Upsert](#updating-and-deleting)** — non-destructive insert-or-update on a
  natural key, perfect for re-syncing an API.
- 📝 **[Full-text search](#full-text-search-fts5)** — SQLite FTS5, relevance-ranked
  and kept in sync automatically.
- ⚡ **[Fast, safe writes](#transactions-and-batch-writes)** — transactions, batch
  prepared statements, foreign-key enforcement, SQL-injection-safe binding.
- 🧬 **[Migrations](#creating-and-dropping-tables)** — a new model field becomes an
  `ALTER TABLE ADD COLUMN`.
- 🪵 **[Debug logging](#debug-logging)** — timestamped, colorized, filterable by
  level and category; see every statement with its params, row count and timing.
- 🖥️ **[QML-ready](#exposing-models-to-qml)** — opt models into `Q_GADGET`, or bind
  a query result to a `ListView` with `QiListModel`.
- ⚡ **[Reactive queries](#reactive-queries-live-models)** — a live `QiListModel`
  re-runs itself on any change, so bound views update automatically. No reload.
- ♾️ **[Infinite scroll](#infinite-scroll-lazy-paging)** — `QiLazyListModel` pages
  the DB in as a `ListView` scrolls, so huge tables load lazily, not all at once.
- 🎯 **Modern & portable** — Qt **5.15 and 6** from one codebase, C++17,
  `[[nodiscard]]` on the operations that matter.
- 📦 **[Header-only option](#install)** — drop in a single generated
  [`dist/qivot.hpp`](dist/qivot.hpp); no library to build. (Or use qmake / CMake
  as a static lib.)

## 🆕 What's new

Recent additions that take Qivot from "capable" to "scales" — each with a
step-by-step example:

- ⚙️ **Async queries** — run any query on a worker thread and get a `QFuture`
  back, so the UI never blocks. `QiAsync::run([](QiConnection &c){ … })` opens an
  isolated per-thread connection for you. → [`examples/asyncquery`](examples/asyncquery),
  [`examples/dashboard`](examples/dashboard)
- 🪜 **Keyset (cursor) pagination** — `QiKeyset<T>` seeks past the last key
  (`WHERE id > :cursor`) instead of `OFFSET`, so deep paging stays fast; cursors
  are resumable. → [`examples/keyset`](examples/keyset)
- 🪟 **Windowed list model** — `QiWindowedListModel` counts once, then fetches only
  the pages you scroll to (and evicts old ones) — a 10k-row list on a tiny memory
  footprint, with a working A–Z jump. → [`examples/contacts`](examples/contacts)
- 🧱 **Versioned migrations** — `QiMigrator` tracks the schema version in
  `PRAGMA user_version` and runs pending migrations in order, transactionally and
  idempotently. → [`examples/migrations`](examples/migrations)
- 🔗 **Relations** — **one-to-many** with `QI_HAS_MANY(Song, songs, "artist")`
  → `artist.songs()`; **many-to-many** with `QI_MANY_TO_MANY(Tag, tags, "photo_tag")`
  → `photo.tags().add(tag)` / `.all()` / `.remove()` / `.contains()`, join table
  auto-created. → [`examples/relations`](examples/relations), [`examples/manytomany`](examples/manytomany)
- 🎛️ **Custom type converters** — `QI_DECLARE_CONVERTER(Type, toStorage, fromStorage)`
  stores any value type in a column. → [`examples/relations`](examples/relations)
- 🪝 **Lifecycle hooks, timestamps & soft delete** — `clean()` / `afterSave()` /
  `beforeRemove()`, automatic `createdAt` / `updatedAt`, and `softRemove()` with
  `qiAlive<T>()` / `qiTrashed<T>()`. → [`examples/relations`](examples/relations)
- 🧵 **Nested transactions** — `QiTransaction` nests via `SAVEPOINT`, so an inner
  rollback undoes only its own work while the outer transaction continues.
  → [`examples/savepoints`](examples/savepoints)
- 🧩 **Raw typed queries** — `qiRawQuery<T>("… WITH/OVER/subquery …")` runs any SQL
  the builder can't express (CTEs, window functions) and maps rows back into typed
  models. → [`examples/dashboard`](examples/dashboard)
- 🚀 **Batched eager-loading** — `qiPrefetchHasMany` / `qiPrefetchManyToMany` load
  a whole list's relations in a fixed number of queries (no N+1).
  → [`examples/prefetch`](examples/prefetch)
- 🧰 **Connection pool + cancellable async** — `QiConnectionPool` hands each thread
  its own connection and `QiAsync` uses it (see the [playground](examples/dashboard)'s
  off-thread step); `QiCancelToken` makes long jobs cooperatively cancellable.

See the full, runnable set — most as tutorials — in
**[`examples/`](examples/README.md)**.

## 🚀 At a glance

```c++
// 1 — Declare a model. Plain C++; the table is derived from it.
class Post : public QiModel {
    QI_MODEL
public:
    QiField<int>     remoteId;
    QiField<QString> title;
};
QI_DECLARE_MODEL(Post, "post",
                 QI_FIELD(remoteId, QiUnique | QiNotNull),
                 QI_FIELD(title));

// 2 — Pull a REST API straight into SQLite, off the UI thread, upserted by id.
auto *req = qiJsonRequest<Post>();
req->setConnection(connection);
req->setUpsertKeys({"remoteId"});                    // idempotent on every refetch
QObject::connect(req, &QiJsonRequest::loaded, this, [](QiSharedList rows) {
    qDebug() << "synced" << rows.size() << "posts";
});
req->get(QUrl("https://api.example.com/posts"));

// 3 — Query and full-text-search it. Typed, no SQL.
auto recent = Post::objects().filter(Post::col().remoteId > 100)
                             .orderBy(Post::col().remoteId.desc()).limit(10).all();   // QiList<Post>
auto hits   = QiQuery<Post>().search("post_fts", "sqlite AND orm").all();
```

## Install

```sh
git clone https://github.com/austinkottke/qivot.git
```

Qivot can be consumed three ways: as **source** or a **static library** (via
qmake or CMake), or as a single **header-only** drop-in
([`dist/qivot.hpp`](dist/qivot.hpp)). Then `#include <qivot.h>` (or
`#include <qivot.hpp>` for the header-only build).

**qmake — include the sources** (simplest): add one line to your `.pro`.

```pro
QT += core sql
include(path/to/qivot/src/qivot.pri)          # or qivot-network.pri for the HTTP loader
```

**CMake — link the static library** (modern): build/install, then
`find_package(Qivot)`.

```cmake
find_package(Qivot REQUIRED)
target_link_libraries(app PRIVATE Qivot::qivot)
```

```sh
cmake -S qivot -B build -DCMAKE_PREFIX_PATH=/path/to/Qt   # -DQIVOT_WITH_NETWORK=OFF to drop QtNetwork
cmake --build build
cmake --install build --prefix /usr/local
```

**qmake — build the static library**: `qmake qivot.pro && make`
(add `CONFIG+=qivot_network` to bundle the HTTP loader).

**Header-only — a single header, no library to build**. Drop
[`dist/qivot.hpp`](dist/qivot.hpp) into your project, link
Qt Core + Sql, and in **exactly one** `.cpp` define the implementation:

```c++
// in one .cpp only:
#define QIVOT_IMPLEMENTATION
#include <qivot.hpp>

// in every other file:
#include <qivot.hpp>
```

The single header is **generated** from `src/` by
[`tools/amalgamate.py`](tools/amalgamate.py) — never edited by hand — so re-run
`python3 tools/amalgamate.py` after changing the sources. It covers the full core
ORM (models, `col()`, queries, joins, transactions, FTS, the synchronous JSON
mapper). The two `Q_OBJECT` add-ons — the threaded `QiJsonRequest` and the QML
`QiListModel` — need moc-generated code, so they aren't header-only; use the
compiled build for those.

See [Project setup](#project-setup-qmake) for more.

## Guide

Every snippet assumes an open connection. Runnable programs live in
[`examples/`](examples) — notably [`schema`](examples/schema), a single program
that shows string & composite keys, `CHECK`, enums, cascading foreign keys,
has-many relations, bulk update and column migrations end to end;
[`jsonnested`](examples/jsonnested), which round-trips a nested JSON document
through a graph of related models; [`contacts`](examples/contacts), a polished **iOS-style Contacts** app —
alphabetical sticky sections, an A–Z scrubber, live search, avatar initials, and
reactive add, all backed by SQLite; [`reactive`](examples/reactive), a Qt Quick
to-do list whose view updates itself via live models;
[`infinitescroll`](examples/infinitescroll), a 1,000-row list that pages in as you
scroll; and
[`jsonhttp`](examples/jsonhttp), which imports a REST API into SQLite on a worker
thread (with a tiny local server so it runs offline).

**Getting started** ·
[Setup](#project-setup-qmake) ·
[Connection](#opening-a-connection) ·
[Models](#declaring-a-model) ·
[Field options](#field-options) ·
[Primary keys](#custom-primary-keys-string-ids-no-auto-id)

**Reading** ·
[Load one](#loading-a-single-record) ·
[Filter](#querying-with-filters) ·
[Iterate](#iterating-a-query-result) ·
[WHERE cookbook](#where-clause-cookbook) ·
[Order & limit](#ordering-limiting-and-selecting-fields) ·
[Aggregates](#aggregate-functions)

**Writing** ·
[Insert / save](#insert--save-a-single-record) ·
[Bulk insert](#bulk-insert-with-qilistwriter) ·
[Update & delete](#updating-and-deleting) ·
[Transactions & batch](#transactions-and-batch-writes)

**Relationships & search** ·
[Foreign keys](#foreign-keys) ·
[Joins](#joins) ·
[Full-text search](#full-text-search-fts5)

**JSON, HTTP & QML** ·
[JSON mapping](#json-mapping) ·
[HTTP import](#loading-json-over-http-on-a-worker-thread) ·
[Expose to QML](#exposing-models-to-qml)

**Schema & lifecycle** ·
[Indexes](#indexes) ·
[Validation](#validation-with-clean) ·
[Error handling](#error-handling) ·
[Seeding](#seeding-initial-data) ·
[Tables & migrations](#creating-and-dropping-tables)

### Project setup (qmake)

Include the `qivot.pri` file from your `.pro` project file. It adds the Qivot
sources, sets the include path and pulls in the `sql` Qt module:

```pro
QT += core sql

include(path/to/qivot/src/qivot.pri)
```

Then include the single umbrella header wherever you use Qivot:

```c++
#include <qivot.h>
```

### Opening a connection

A `QiConnection` wraps a `QSqlDatabase`. The first connection you open becomes
the *default connection* shared by all models, so in the common single-database
case you never have to pass it around explicitly.

```c++
#include <qivot.h>
#include <QSqlDatabase>

// 1. Open the database with the normal Qt API
QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
db.setDatabaseName("app.db");
db.open();

// 2. Wrap it in a QiConnection
QiConnection connection;
connection.open(db);        // becomes the default connection

// 3. Register your models and create their tables
connection.addModel<User>();
connection.createTables();  // CREATE TABLE IF NOT EXISTS for every model

// ... use the database ...

connection.close();         // always close on exit
```

### Declaring a model

A model is a plain C++ class that inherits `QiModel`, carries the `QI_MODEL`
macro, declares its columns as `QiField<T>` members, and is registered with
`QI_DECLARE_MODEL`. `QiModel` already provides an auto-increment `id` primary
key, so you never declare it yourself — or opt out and use your own string key
(see [Custom primary keys](#custom-primary-keys-string-ids-no-auto-id)).

```c++
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString>   userId;
    QiField<QDateTime> creationDate;
    QiField<qreal>     karma;
};

QI_DECLARE_MODEL(User,
                 "user",                                        // table name
                 QI_FIELD(userId, QiNotNull | QiUnique),
                 QI_FIELD(creationDate, QiDefault("CURRENT_TIMESTAMP")),
                 QI_FIELD(karma));
```

Common field types: `QString`, `int`, `qreal` / `double`, `bool`, `QDate`,
`QDateTime`, `QByteArray` — anything `QVariant` can hold. An **`enum`** field
(`QiField<MyEnum>`) is stored as its underlying integer (`INTEGER` column):

```c++
enum Priority { Low, Medium, High };
class Ticket : public QiModel {
    QI_MODEL
public:
    QiField<QString>  subject;
    QiField<Priority> priority;   // -> priority INTEGER
};
QI_DECLARE_MODEL(Ticket, "ticket", QI_FIELD(subject), QI_FIELD(priority));
Q_DECLARE_METATYPE(Priority)      // so Qivot can see the type

ticket.priority = High;                                   // stores 2
Ticket::objects().filter( Ticket::col().priority == High ).all();
```

### Field options

Options are passed as the optional second argument of `QI_FIELD` and combined
with `|`:

| Option                         | SQL effect                          |
| ------------------------------ | ----------------------------------- |
| `QiNotNull`                    | `NOT NULL`                          |
| `QiUnique`                     | `UNIQUE`                            |
| `QiDefault(value)`             | `DEFAULT value`                     |
| `QiPrimary`                    | `PRIMARY KEY` (see below)           |
| `QiCheck("expr")`              | `CHECK (expr)`                      |
| `QiNotNull | QiUnique`         | combine several with `|`            |

```c++
QI_DECLARE_MODEL(User,
                 "user",
                 QI_FIELD(userId,       QiNotNull | QiUnique),
                 QI_FIELD(creationDate, QiDefault("CURRENT_TIMESTAMP")),
                 QI_FIELD(karma));   // no option needed
```

**Override the SQL column type** with `QI_FIELD_AS(field, "TYPE", clauses...)`
when the type Qivot infers from C++ isn't what you want — a different literal
(`REAL` vs `DOUBLE`), a domain type (`STRING`), or `""` for a **typeless** column
(no type between the name and its constraints):

```c++
QI_DECLARE_MODEL(Activity, "activity",
                 QI_FIELD_AS(ticketNumber, "STRING", QiNotNull),  // -> ticketNumber STRING NOT NULL
                 QI_FIELD_AS(billed,       "REAL"),               // -> billed REAL   (not DOUBLE)
                 QI_FIELD_AS(closedUtc,    ""),                   // -> closedUtc     (typeless)
                 QI_FIELD(seq));                                  // -> seq INTEGER   (inferred)
```

Values still round-trip under SQLite's flexible typing; the override only changes
the declared type/affinity, so you can match an existing schema exactly.

### Custom primary keys (string ids, no auto `id`)

By default every table gets a hidden `id INTEGER PRIMARY KEY AUTOINCREMENT`. When
your rows are keyed by a **meaningful string** instead — a ULID, a `userId`, a
server-assigned id — declare the model with **`QI_DECLARE_MODEL_NOID`** and mark
your own field `QiPrimary`. No auto `id` column is added, and the string key gets
a plain `PRIMARY KEY` (never the integer-only `AUTOINCREMENT`):

```c++
class Contact : public QiModel {
    QI_MODEL
public:
    QiField<QString> contactId;
    QiField<QString> name;
};

QI_DECLARE_MODEL_NOID(Contact, "contact",
                      QI_FIELD(contactId, QiPrimary | QiNotNull),
                      QI_FIELD(name));
```

```sql
CREATE TABLE IF NOT EXISTS contact (
    contactId TEXT NOT NULL PRIMARY KEY,
    name TEXT
);
```

`save()` (REPLACE on the key), `load()`, queries and `remove()` all work off the
declared primary key — e.g. `contact.load( Contact::col().contactId == id )`. A
foreign key can reference this string key too (see
[Foreign keys](#foreign-keys)). Because there's no `id` column, `Contact::col()`
has no `.id` member (referencing it is a compile error), and such a model can't
be an FTS content table; use the default (auto-`id`)
form for that.

**Composite primary keys.** Mark more than one field `QiPrimary` to get a
table-level composite key:

```c++
QI_DECLARE_MODEL_NOID(Enrollment, "enrollment",
                      QI_FIELD(studentId, QiPrimary | QiNotNull),
                      QI_FIELD(courseId,  QiPrimary | QiNotNull),
                      QI_FIELD(grade));
// -> CREATE TABLE enrollment (... , PRIMARY KEY (studentId, courseId))
```

`save()`/`remove()` match on the whole key, so `save()` replaces the row with the
same `(studentId, courseId)` rather than inserting a duplicate.

**`WITHOUT ROWID`.** A model that has a primary key can drop SQLite's shadow
rowid — a good fit for string/composite keys. Call `qiWithoutRowid<Model>()` once
before `createTables()`:

```c++
qiWithoutRowid<Session>();     // -> CREATE TABLE session (...) WITHOUT ROWID
connection.createTables();
```

> **Worked example.** [`examples/schema`](examples/schema) puts these together —
> a string-keyed `Customer` (WITHOUT ROWID), an `Order` with a cascading foreign
> key, an enum status and a `CHECK`, and a composite-keyed `OrderItem` — then
> exercises has-many, bulk update and migrations. Its generated DDL:
>
> ```sql
> CREATE TABLE customer ( customerId TEXT NOT NULL PRIMARY KEY, name TEXT ) WITHOUT ROWID;
> CREATE TABLE orders   ( id INTEGER PRIMARY KEY AUTOINCREMENT, customer INTEGER, status INTEGER,
>                         total INTEGER CHECK (total >= 0), note TEXT,
>                         FOREIGN KEY(customer) REFERENCES customer(customerId) ON DELETE CASCADE );
> CREATE TABLE order_item ( orderId INTEGER NOT NULL, sku TEXT NOT NULL, qty INTEGER CHECK (qty > 0),
>                           PRIMARY KEY (orderId, sku) );
> ```

### Insert / save a single record

Assign fields directly (a `QiField` accepts values by assignment) then call
`save()`. For a new record the `id` primary key is filled in for you.

```c++
User user;
user.userId = "anonymous";
user.karma  = 0;

user.save();          // INSERT

qDebug() << user.id;  // e.g. QVariant(int, 1) — assigned by save()
```

`save()` on a record that already has an `id` performs an `UPDATE` instead of an
`INSERT`. Pass `save(true)` to *force* an insert (useful when reusing one model
instance to write many rows).

### Bulk insert with QiListWriter

`QiList<T>` holds many records; `QiListWriter` fills it field-by-field in
declaration order and `save()` writes them all at once.

```c++
QiList<User> data;
QiListWriter writer(&data);

// userId, karma  — one pair per row, in field-declaration order
writer << "tester1" << 50
       << "tester2" << 10
       << "tester3" << 60;

data.save();          // insert all three rows
```

If a model has several fields you can use `writer.next()` to mark the end of a
row explicitly and keep things readable:

```c++
writer << "tester1" << "12345678" << writer.next()
       << "tester2" << "12345678" << writer.next();
```

### Loading a single record

`load()` fills a model instance from the first row matching a filter. It returns
`false` if no row matched.

```c++
User user;
if (!user.load( User::col().userId == "anonymous" )) {
    // not found — create it
    user.userId = "anonymous";
    user.karma  = 0;
    user.save();
}
```

### Referencing columns

A filter compares a **column** to a value. There are three equivalent ways to
name the column — each produces the same result, so you can mix them freely:

```c++
User::col().karma > 50          // ← recommended: type-safe, no string, no &
qiField(&User::karma) > 50      // pointer-to-member (from <qifieldref.h>)
QiWhere("karma") > 50           // raw string (always works, incl. computed names)
```

`Model::col()` is generated automatically from `QI_DECLARE_MODEL` — every column
becomes a named member (plus `.id` for a default model; a
[`QI_DECLARE_MODEL_NOID`](#custom-primary-keys-string-ids-no-auto-id) model has no
`.id` since its table has none). Rename a column and a stale `User::col().karma`
becomes a **compile error at the use site** instead of a query that silently
breaks. Cache the descriptor once and reuse it:

```c++
auto U = User::col();
auto top = User::objects()
               .filter( U.karma > 100 && U.userId != "admin" )
               .all();
```

Reach for the `QiWhere("...")` string form when the column name is computed at
runtime, is table-qualified in a join (`QiWhere("user.id")`), or is an aggregate
expression (`QiWhere("count(*)")`).

> Why the wrapper instead of `&User::karma > 100` directly? C++ only considers
> overloaded operators when an operand is a class type — a raw pointer-to-member
> isn't one, so the bare form can never compile. `col()` and `qiField()` turn the
> field into a class-type `QiWhere`, which is what makes `> 100` legal.

### Querying with filters

`QiQuery<T>` (or the `Model::objects()` shortcut) builds `SELECT` queries.
`filter()` narrows the result; `all()` returns a `QiList<T>`.

```c++
auto U = User::col();

// Every user with karma > 50
auto top = User::objects()
               .filter( U.karma > 50 )
               .all();

qDebug() << top.size();
```

`objects()` and an explicit `QiQuery<User>` are interchangeable:

```c++
QiQuery<User> query;
auto top = query.filter( User::col().karma > 50 ).all();
```

### Iterating a query result

For large result sets you can stream rows one at a time instead of materializing
a whole `QiList`:

```c++
QiQuery<User> query;
query = query.filter( User::col().karma > 50 );

if (query.exec()) {
    User user;
    while (query.next()) {   // advance to the next row
        query.recordTo(user);
        qDebug() << user.userId << user.karma;
    }
}
```

### WHERE clause cookbook

A column reference overloads the usual comparison operators and combines with
`&&` (AND) and `||` (OR):

```c++
auto U = User::col();

U.karma == 50                         // karma = 50
U.karma != 50                         // karma <> 50
U.karma >  50                         // karma > 50
U.karma <  80                         // karma < 80
U.karma >= 50                         // karma >= 50
U.karma <= 80                         // karma <= 80

U.karma > 50 && U.karma < 80          // 50 < karma < 80
U.karma < 10 || U.karma > 90          // karma < 10 OR karma > 90

U.karma.between(50, 80)               // karma BETWEEN 50 AND 80

// Fall back to a raw operator the wrapper doesn't cover
U.name.expr("like", "test%")          // name LIKE 'test%'
```

The identical operators work on `qiField(&User::karma)` and `QiWhere("karma")` —
pick whichever column reference suits the call site.

```c++
auto mid = User::objects()
               .filter( User::col().karma.between(50, 80) )
               .all();
```

Qivot parameterizes values internally, so building filters this way protects
you from SQL injection.

### Ordering, limiting and selecting fields

A column reference's `.asc()` / `.desc()` produce the ordering term for
`orderBy()` (`U.karma.desc()` → `"karma desc"`), so sort keys stay type-safe too.
`orderBy()` still accepts a raw `"karma desc"` string if you prefer.

```c++
auto U = User::col();

// Top 10 users by karma, highest first
auto leaders = User::objects()
                         .filter( U.karma > 0 )
                         .orderBy( U.karma.desc() )        // "karma desc", type-safe
                         .limit(10)
                         .all();

// Order by several columns
User::objects().orderBy(QStringList() << U.karma.desc() << U.userId.asc()).all();

// Paginate with limit() + offset()  (page 3, 20 per page)
auto page3 = User::objects().orderBy( U.userId.asc() ).limit(20).offset(40).all();
```

### Aggregate functions

Use `count()` for row counts and `call()` for any SQL aggregate
(`sum`, `avg`, `min`, `max`, …). `call()` returns a `QVariant`.

```c++
int n = User::objects().filter( User::col().karma > 50 ).count();   // COUNT(*)

int total   = User::objects().call("sum", "karma").toInt();      // SUM(karma)
double avg  = User::objects().call("avg", "karma").toDouble();   // AVG(karma)
int highest = User::objects().call("max", "karma").toInt();      // MAX(karma)
```

**Group and filter aggregates** with `groupBy()` / `having()`. Select the group
column(s) plus the aggregate, then read each row with `value(i)`:

```c++
// SELECT author, count(*) FROM post GROUP BY author HAVING count(*) > 5
QiQuery<Post> q = QiQuery<Post>()
        .select(QStringList() << "author" << "count(*)")
        .groupBy("author")
        .having(QiWhere("count(*)") > 5)
        .orderBy("author asc");

if (q.exec()) {
    while (q.next())
        qDebug() << q.value(0).toString()   // author
                 << q.value(1).toInt();     // post count
}
```

(Grouped queries return aggregate rows, not model records, so iterate with
`exec()` / `next()` / `value(i)` rather than `all()`.)

### Updating and deleting

Update a single record by loading it, changing a field and saving:

```c++
User user;
if (user.load( User::col().userId == "anonymous" )) {
    user.karma = user.karma->toInt() + 1;
    user.save();          // UPDATE (record already has an id)
}
```

Delete a single loaded record with `remove()`:

```c++
user.remove();            // DELETE this row
```

Delete many rows in one statement by filtering a query and calling `remove()`:

```c++
QiQuery<User> query;
query.filter( User::col().karma < 0 ).remove();   // DELETE ... WHERE karma < 0
```

**Update many rows in one statement** with `update()` — no per-row load. It takes
a column→value map and returns the number of rows changed:

```c++
int changed = User::objects()
                  .filter( User::col().karma < 0 )
                  .update({ {"karma", 0}, {"name", "reset"} });
// UPDATE user SET karma = 0, name = 'reset' WHERE karma < 0
```

**Upsert** — `save()` is already *insert-or-replace* by primary/unique key. For a
**non-destructive** update-or-insert on a natural key (keeping the row's primary
key rather than deleting and re-inserting it), use `upsert()`:

```c++
User user;
user.userId = "anonymous";    // a UNIQUE column
user.karma  = 5;

// INSERT INTO user (...) VALUES (...) ON CONFLICT(userId) DO UPDATE SET karma=...
user.upsert({"userId"});      // insert, or update the matching row in place
```

Pass the conflict column(s) that identify an existing row. This is ideal for
importing records from an API by a natural key on repeated runs — no duplicates,
and existing rows (and their `id`) are preserved. Requires SQLite 3.24+.

### Transactions and batch writes

Wrap a group of writes in a **transaction** for atomicity and a big speed-up
(SQLite otherwise commits — and fsyncs — after every statement). The RAII
`QiTransaction` guard rolls back automatically if you don't commit:

```c++
#include <qitransaction.h>

QiTransaction transaction;          // BEGIN

for (Order &order : orders)
    order.save();

transaction.commit();               // COMMIT
// (if we returned or threw before commit(), the guard rolls back)
```

There's also the explicit form: `connection.transaction()` / `commit()` /
`rollback()`. For workloads where readers and writers run concurrently (e.g. the
[threaded HTTP importer](#loading-json-over-http-on-a-worker-thread) writing
while your UI thread reads), switch the database to **write-ahead logging** so
readers don't block the writer:

```c++
connection.setJournalMode("WAL");   // persistent property of the database file
```

**Batch writes come for free with `QiList<T>::save()`.** It groups the records by
their column set, writes each group through a single *prepared statement*, and
wraps the whole thing in one transaction — so bulk inserts (and the
[threaded HTTP importer](#loading-json-over-http-on-a-worker-thread)) are
dramatically faster than saving records one at a time:

```c++
QiList<User> users = QiJsonMapper::map<User>(array);
users.save();                       // one prepared statement, one transaction
```

**Transactions nest.** Open another `QiTransaction` inside an active one and it
uses a `SAVEPOINT`, so an inner `rollback()` undoes only its own work while the
outer transaction continues — see the [savepoints example](examples/savepoints).

### Foreign keys

`QiForeignKey<T>` links a row to another model's row. Assign a loaded model to
it, and dereference it with `->` to lazily load and read the referenced row's
fields.

> Qivot turns on SQLite **foreign-key enforcement** (`PRAGMA foreign_keys = ON`)
> when it opens a connection, so a `QiForeignKey` referencing a non-existent row
> is rejected. Toggle it with `connection.setForeignKeysEnforced(false)` if you
> need the legacy (unchecked) behaviour.

**Relations, both ways.** Declare the reverse of a foreign key with
`QI_HAS_MANY(Book, books, "author")` → `author.books()` (a composable query), and
a **many-to-many** with `QI_MANY_TO_MANY(Tag, tags, "photo_tag")` on each side →
`photo.tags().add(tag)` / `.all()` / `.remove()` / `.contains()`, the join table
created for you. Load a whole list's relations without N+1 using
`qiPrefetchHasMany`. See [`relations`](examples/relations), [`manytomany`](examples/manytomany),
and [`prefetch`](examples/prefetch).

```c++
class FriendShip : public QiModel {
    QI_MODEL
public:
    QiForeignKey<User> a;
    QiForeignKey<User> b;
    QiField<QDateTime> creationDate;
};

QI_DECLARE_MODEL(FriendShip,
                 "friendship",
                 QI_FIELD(a, QiNotNull),
                 QI_FIELD(b, QiNotNull),
                 QI_FIELD(creationDate, QiDefault("CURRENT_TIMESTAMP")));
```

```c++
User tester1; tester1.load(User::col().userId == "tester1");
User tester2; tester2.load(User::col().userId == "tester2");

FriendShip fs;
fs.a = tester1;
fs.b = tester2;
fs.save(true);            // force insert a new link

// Query and walk the foreign relation without touching the user table yourself
auto friends = FriendShip::objects()
                               .filter(FriendShip::col().a == tester1.id)
                               .all();

for (int i = 0; i < friends.size(); i++) {
    FriendShip* f = friends.at(i);
    qDebug() << f->b->userId;   // -> loads the referenced User on demand
}
```

`QiForeignKey<T>` lazy loading is convenient for walking a single relation, but
for filtering across tables in one round trip use a [join](#joins).

**Referential actions.** Pass an `ON DELETE` action as the second template
argument to emit it in the schema — e.g. delete a user's rows automatically when
the user is removed:

```c++
class Post : public QiModel {
    QI_MODEL
public:
    QiField<QString>                author;
    QiForeignKey<User, QiFkCascade> owner;   // FOREIGN KEY(owner) REFERENCES user(id) ON DELETE CASCADE
};
```

Available actions: `QiFkCascade`, `QiFkRestrict`, `QiFkSetNull`, `QiFkSetDefault`
(and the default `QiFkNoAction`).

**References to non-`id` keys.** A foreign key automatically references the
target model's actual primary key — so it works against a
[string-keyed model](#custom-primary-keys-string-ids-no-auto-id) too:

```c++
QiForeignKey<Team> team;   // -> FOREIGN KEY(team) REFERENCES team(teamId)
member.team = someTeam;    // stores teamId; member.team->teamName lazy-loads it
```

**Reverse relations (`has many`).** A `QiForeignKey` walks child → parent.
`qiHasMany<Child>(parent, "fkField")` walks the other way and returns a query for
every child that points back — in **one** statement, not one-per-child:

```c++
#include <qirelation.h>   // (also pulled in by <qivot.h>)

QiList<Comment> comments = qiHasMany<Comment>(blog, "blog").all();

// composes like any query
int recent = qiHasMany<Post>(user, "author")
                 .filter( Post::col().remoteId > 100 )
                 .count();
```

To prefetch children for many parents at once, collect their keys and use one
`IN` query: `QiQuery<Post>().filter( QiWhere("author").in(ids) ).all()`, then
group in memory.

### Joins

Qivot supports `INNER`, `LEFT OUTER`, `RIGHT OUTER`, `FULL OUTER` and `CROSS`
joins through `QiJoin<T>` and the chainable `join()` method. A join lets you
filter (and aggregate) across related tables in a single query. The query still
returns records of the *primary* model — the joined table is available to the
`ON` condition and to `filter()`.

Add a join with `join(QiJoin<Model>(on, type))`:

* `on` is a `QiWhere` comparing columns of the two tables. **Qualify column
  names with the table name** (`"user.id"`, `"friendship.a"`) so they aren't
  ambiguous. Compare two columns by putting a `QiWhere` on both sides — it is
  emitted as an identifier, not a bound value. Omit it to derive the condition
  from a foreign key (see *Automatic ON* below).
* `type` is one of `QiBaseJoin::Inner` (default), `Left`, `Right`, `Full`, or
  `Cross`.

Using the `User` / `FriendShip` models from the [Foreign keys](#foreign-keys)
section (`friendship.a` and `friendship.b` both reference `user.id`):

```c++
// INNER JOIN — the users that tester1 has befriended (the "b" side)
// SELECT user.* FROM user
//   INNER JOIN friendship ON friendship.b = user.id
//   WHERE friendship.a = <tester1.id>
auto friends = User::objects()
        .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
        .filter( QiWhere("friendship.a") == tester1.id )
        .all();
```

`orderBy()`, `limit()`, `count()` and `call()` all work with a join — just
qualify the column names:

```c++
// Aggregate across the join: average karma of tester1's friends
double avg = User::objects()
        .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
        .filter( QiWhere("friendship.a") == tester1.id )
        .call("avg", "user.karma")
        .toDouble();

// Count rows produced by the join
int n = User::objects()
        .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
        .filter( QiWhere("friendship.b") == tester3.id )
        .count();
```

Choose the join type with the second argument:

```c++
// LEFT OUTER JOIN — every user, plus the friendships they initiate
auto rows = User::objects()
        .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id"),
                                   QiBaseJoin::Left ) )
        .orderBy("user.userId asc")
        .all();

// CROSS JOIN — no ON condition
User::objects()
        .join( QiJoin<FriendShip>( QiWhere(), QiBaseJoin::Cross ) )
        .all();
```

Chain multiple `join()` calls to join more than two tables. `ON` conditions may
contain literal values as well as column comparisons — they are bound safely:

```c++
query.join( QiJoin<FriendShip>(
        (QiWhere("friendship.b") == QiWhere("user.id")) &&
        (QiWhere("friendship.a") == tester1.id) ) );
```

**Removing duplicates with `distinct()`.** A join can produce the same primary
record several times (once per matching row in the joined table). Add
`distinct()` to collapse them:

```c++
// Users who have initiated at least one friendship, each listed once
auto initiators = User::objects()
        .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) )
        .distinct()
        .all();
```

**Automatic ON from a foreign key.** When the two models are linked by a single
`QiForeignKey`, you can omit the `ON` condition and let Qivot derive it — in
either direction. `join<T>()` is shorthand for `join(QiJoin<T>())`:

```c++
// Config declares QiForeignKey<User> uid, so the ON condition
// "config.uid = user.id" is derived automatically.
auto list = Config::objects()
        .join<User>()
        .filter( QiWhere("user.userId") == "tester1" )
        .all();
```

> If more than one foreign key links the two tables (for example `FriendShip`
> has both `a` and `b` referencing `user`), the relationship is ambiguous —
> Qivot warns and you should pass an explicit `ON` condition instead.

**What a join returns.** A query always returns records of the *primary* model
(`QiList<User>` above) — Qivot does not hydrate the joined table's rows into the
result. Use the join to *filter and aggregate* across tables; to read the linked
row, dereference its `QiForeignKey` (which lazy-loads it) or run a second query.

> A join can return the same primary row several times (once per matching row in
> the joined table) — standard SQL semantics; use `distinct()` to dedup.
> `RIGHT OUTER` and `FULL OUTER` joins require SQLite 3.39 or newer. See
> [`examples/tutorial5`](examples/tutorial5) for a complete, runnable program.

### Indexes

Declare a `QiIndex<T>`, append the columns to cover, and create it through the
connection.

```c++
QiIndex<HealthCheck> byHeight("idx_height");
byHeight << "height";

QiIndex<HealthCheck> byBoth("idx_height_weight");
byBoth << "height" << "weight";     // composite index

connection.createIndex(byHeight);   // CREATE INDEX idx_height on healthcheck (height)
connection.createIndex(byBoth);

connection.dropIndex("idx_height"); // remove it later
```

Make it **`UNIQUE`**, or a **partial** index restricted by a condition:

```c++
QiIndex<User> byEmail("uq_email");
byEmail << "email";
byEmail.setUnique(true);                   // CREATE UNIQUE INDEX
connection.createIndex(byEmail);

QiIndex<Order> activeIdx("idx_active");
activeIdx << "customerId";
activeIdx.setWhere("status = 'open'");     // partial: ... WHERE status = 'open'
connection.createIndex(activeIdx);
```

### Full-text search (FTS5)

Qivot can build a **SQLite FTS5** full-text index over a model's text columns.
Declare a `QiFtsIndex<T>` with the searchable columns and create it through the
connection — Qivot builds the FTS5 virtual table, installs triggers that keep it
in sync with the table, and backfills it from existing rows:

```c++
QiFtsIndex<Article> fts("article_fts");
fts << "title" << "body";               // the searchable columns

connection.createFtsIndex(fts);         // FTS5 table + sync triggers + backfill
```

From then on, ordinary `save()`, `upsert()`, and `remove()` calls keep the index
current automatically. Run a search with `search()`, which joins the index,
filters by the FTS5 `MATCH` query, and orders by relevance (`rank`):

```c++
// SELECT article.* FROM article JOIN article_fts ON article.id = article_fts.rowid
//   WHERE article_fts MATCH 'sqlite AND orm' ORDER BY article_fts.rank
QiList<Article> hits = QiQuery<Article>()
        .search("article_fts", "sqlite AND orm")
        .limit(20)
        .all();
```

The MATCH string is a full [FTS5 query](https://www.sqlite.org/fts5.html#full_text_query_syntax)
— `AND` / `OR` / `NOT`, phrase `"..."`, prefix `orm*`, and column filters
(`title:sqlite`). Drop the index (its triggers and virtual table) with:

```c++
connection.dropFtsIndex("article_fts");
```

> Requires SQLite built with FTS5 (Qt's bundled SQLite includes it). Qivot turns
> on `PRAGMA recursive_triggers` when it opens a connection so that
> `REPLACE`-based `save()` updates keep the index in sync.

### Validation with clean()

Override `clean()` to validate (and optionally mutate) a record before it is
saved. Returning `false` aborts the `save()`; it's also the natural place to
stamp bookkeeping fields.

```c++
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString>   userId;
    QiField<QString>   passwd;
    QiField<QDateTime> lastModifiedTime;

    virtual bool clean();
};

bool User::clean() {
    if (passwd->isNull() || passwd->toString().size() < 8) {
        setError("password must be at least 8 characters");   // surfaces via lastError()
        return false;                                         // save() returns false
    }
    lastModifiedTime = QDateTime::currentDateTime();          // stamp on every save
    return true;
}
```

```c++
User user;
user.userId = "anonymous";
qDebug() << user.save();               // false
qDebug() << user.lastError().text();   // "password must be at least 8 characters"

user.passwd = "123456789";
qDebug() << user.save();               // true
```

### Error handling

Operations return `bool`; when one fails, the reason is a `QiError` from
`lastError()` on the object involved.

```c++
if (!user.save()) {
    QiError e = user.lastError();
    switch (e.type()) {
        case QiError::ValidationError: /* clean() rejected it */ break;
        case QiError::StatementError:  qWarning() << e.text(); break;  // e.g. UNIQUE constraint failed
        default: break;
    }
}

Config config;
if (!config.load(Config::col().key == "missing"))
    qDebug() << config.lastError().type();   // QiError::NotFound

if (!connection.createTables())
    qWarning() << connection.lastError().text();   // connection-level failures
```

`QiError::type()` is one of `NoError`, `ValidationError`, `StatementError`,
`NotFound`, `ConnectionError`, `NotSupported`; `text()` is the message (the SQL
driver's for statement errors, or the one your `clean()` passed to `setError()`).
`QiModel::lastError()` covers `save()` / `load()` / `remove()` / `upsert()`;
`QiConnection::lastError()` covers connection-level operations. (The threaded
`QiJsonRequest` reports errors through its `failed(QString)` signal instead.)

### Debug logging

`QiLog` traces what Qivot does against the database — every statement with its
bound parameters, row count and timing, plus connection open/close and errors.
It's **off by default** (zero overhead); switch it on when you need to debug:

```c++
#include <qilog.h>          // (also pulled in by <qivot.h>)

QiLog::enableAll();         // everything, at Debug level
```

```text
[2026-07-21 12:34:10.885] [SQL ] DEBUG REPLACE INTO user (karma,name,userId) values (:karma,:name,:userId);  | args: [:karma=120, :name=Ada, :userId=ada]  | rows: 1
[2026-07-21 12:34:10.885] [SQL ] DEBUG SELECT ALL * FROM user WHERE karma > :arg0 ORDER BY karma desc ;  | args: [:arg0=100]  | rows: 1  | 0.01ms
[2026-07-21 12:34:10.886] [SQL ] ERROR INSERT INTO ...  | error: UNIQUE constraint failed: user.userId
```

Each line carries a **timestamp**, a **category** tag and a **level**; the
built-in sink colorizes by level. Filter by either:

```c++
QiLog::setEnabled(true);
QiLog::setLevel(QiLog::Warning);                       // only Warning and above
QiLog::setCategories(QiLog::Sql | QiLog::Connection);  // only these categories
QiLog::setTimestamps(false);                           // or drop the timestamp
```

Categories are `Sql`, `Connection`, `Model`, `Json` and `General`. Send the log
somewhere other than `stderr` — a file, your app's logger, a test buffer — with a
handler:

```c++
QiLog::setHandler([](QiLog::Level level, int category, const QString &line) {
    myLogFile << line << '\n';
});
```

Log your own lines in the same format with a `QDebug`-style stream (skipped
entirely when logging is off, so it's cheap to leave in):

```c++
qiLog(QiLog::General, QiLog::Info) << "imported" << n << "posts";
// or: QiLog::info("cache warm"), QiLog::warning(...), QiLog::error(...)
```

### Seeding initial data

Override `initialData()` to return rows that are inserted automatically the
first time `createTables()` builds the table.

```c++
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString> userId;
    virtual QiSharedList initialData() const;
};

QiSharedList User::initialData() const {
    QiList<User> rows;
    QiListWriter writer(&rows);
    writer << "tester1"
           << "tester2"
           << "tester3";
    return rows;
}
```

```c++
connection.addModel<User>();
connection.createTables();                  // table created AND seeded with 3 users
qDebug() << User::objects().all().size();   // 3
```

### Creating and dropping tables

```c++
connection.addModel<User>();
connection.addModel<FriendShip>();

connection.createTables();   // create every registered model's table (if missing)
connection.dropTables();     // drop them all — handy to reset during development
```

**Lightweight migrations.** `createTables()` also *evolves* an existing table:
if you add a `QiField` to a model, the next `createTables()` runs
`ALTER TABLE ... ADD COLUMN` for the new column (SQLite adds it nullable, or with
its `QiDefault` value).

**Rename and drop columns** explicitly (a rename can't be auto-detected):

```c++
connection.renameColumn<User>("karma", "reputation");   // SQLite 3.25+
connection.dropColumn<User>("legacyField");             // SQLite 3.35+
```

### JSON mapping

`QiJsonMapper` converts between JSON and models — a plain object (de)serializer
that has nothing to do with the database. Only the fields declared on the model
are considered — extra JSON keys are ignored, and values are coerced to each
field's declared type (an ISO-8601 string maps into a `QiField<QDateTime>`, a
JSON number into a `QiField<int>`, etc.). It is pure QtCore — no I/O, no threading.

**Pure JSON text ⇄ object** — the one-liners take/return raw bytes:

```c++
#include <qijsonmapper.h>

User      user  = QiJsonMapper::deserialize<User>(bytes);   // JSON text -> object
QByteArray back = QiJsonMapper::serialize(&user);           // object -> JSON text

QiList<User> users = QiJsonMapper::deserializeList<User>(bytes);   // a JSON array
```

Or work at the `QJsonObject` level if you already have one parsed:

```c++
QJsonObject obj  = QJsonDocument::fromJson(bytes).object();
User        user = QiJsonMapper::map<User>(obj);            // object <- QJsonObject
QJsonObject out  = QiJsonMapper::toJson(&user);            // object -> QJsonObject
```

**Mapping is in-memory — persistence is on your terms.** `map()` / `fromJson()`
return plain model objects; nothing touches the database until *you* call
`save()`. So you can deserialize, inspect, mutate, validate, and only then
persist (or never):

```c++
Post p = QiJsonMapper::map<Post>(obj);   // built in memory, NOT saved
p.title = p.title->toString().trimmed(); // tweak it
if (looksGood(p)) p.save();              // persist only when you decide
```

**Complex / nested JSON.** Declare a field as `QiField<QJsonObject>` or
`QiField<QJsonArray>` (also `QVariantMap` / `QVariantList`) to hold an arbitrarily
nested structure. It lives as real JSON in memory — read and mutate it — and is
serialized to a `TEXT` column only on `save()`:

```c++
class Doc : public QiModel {
    QI_MODEL
public:
    QiField<QString>     name;
    QiField<QJsonObject> meta;    // nested object   -> TEXT
    QiField<QJsonArray>  tags;    // nested array     -> TEXT
};
QI_DECLARE_MODEL(Doc, "doc", QI_FIELD(name), QI_FIELD(meta), QI_FIELD(tags));

Doc d = QiJsonMapper::map<Doc>(json);          // nested JSON, in memory
qDebug() << d.meta->toJsonObject()["author"];  // navigate the structure
d.save();                                      // now it's TEXT in SQLite

Doc l; l.load( Doc::col().name == "report" );  // reload -> structure intact
QJsonObject back = QiJsonMapper::toJson(&l);   // nested object/array, not a string
```

**A graph of related models.** The mapper works one model at a time, so a nested
document — an order that embeds its customer and a list of line items — is
composed by mapping each level and wiring the relations by id:

```c++
QJsonObject root = QJsonDocument::fromJson(bytes).object();

Order            order    = QiJsonMapper::map<Order>(root);                          // parent
Customer         customer = QiJsonMapper::map<Customer>(root.value("customer").toObject());
QiList<LineItem> items    = QiJsonMapper::map<LineItem>(root.value("items").toArray());

order.customerRef = customer.customerId;                       // link in memory
for (int i = 0; i < items.size(); i++)
    items.at(i)->orderRef = order.orderId;

customer.save();  order.save();  items.save();                 // persist when ready
```

Serialize the other way by nesting `toJson()` of each part into one `QJsonObject`
(and reload children with [`qiHasMany`](#foreign-keys)). The runnable
[`jsonnested`](examples/jsonnested) example does the full round trip — parse →
in-memory graph → re-serialize → persist → reload → re-nest.

**Type coercion.** Each JSON value is coerced to the field's *declared* type, so
your model holds real `int`s / `QDateTime`s (not raw JSON), and saves cleanly:

| Field type          | Accepted JSON                          | Result |
| ------------------- | -------------------------------------- | ------ |
| `int` / `qreal`     | number **or** numeric string           | `42`, `3.14` |
| `bool`              | `true` / `false` / `0` / `1`           | `true` |
| `QDateTime` / `QDate` | ISO-8601 string (`"2026-01-02T03:04:05"`) | parsed date/time |
| `QByteArray`        | base64 string                          | decoded bytes |
| any                 | `null` / missing                       | left untouched |

```c++
class Event : public QiModel {
    QI_MODEL
public:
    QiField<int>       priority;
    QiField<bool>      active;
    QiField<QDateTime> startsAt;
    QiField<QByteArray> payload;
};

QByteArray bytes = R"({
    "priority": "3",                         // numeric string -> int 3
    "active":   1,                           // 1 -> true
    "startsAt": "2026-06-01T09:30:00",       // ISO string -> QDateTime
    "payload":  "aGVsbG8=",                  // base64 -> "hello"
    "note":     "ignored (no such field)"    // extra keys are dropped
})";
auto event = QiJsonMapper::map<Event>(QJsonDocument::fromJson(bytes).object());
qDebug() << event.priority->toInt()      // 3
         << event.active->toBool()       // true
         << event.startsAt->toDateTime() // QDateTime(2026-06-01 09:30:00)
         << event.payload->toByteArray();// "hello"
```

**Match the API's field names.** Mapping is by *exact* field name (case-sensitive),
and a field's name is also its column name — so to consume a snake_case REST API,
just name your fields to match. Anything not declared is ignored:

```c++
class Article : public QiModel {          // maps GET /articles cleanly
    QI_MODEL
public:
    QiField<int>       remote_id;         // JSON "remote_id"  (column: remote_id)
    QiField<QString>   title;
    QiField<bool>      is_published;      // JSON "is_published"
    QiField<QDateTime> published_at;      // JSON "published_at" (ISO-8601)
};
QI_DECLARE_MODEL(Article, "article",
                 QI_FIELD(remote_id, QiUnique | QiNotNull),
                 QI_FIELD(title), QI_FIELD(is_published), QI_FIELD(published_at));
```

> A JSON key that isn't a valid/desired column name (or `id`, which maps onto the
> auto-increment primary key) — rename it on the `QJsonObject` before mapping.

**Unwrap a wrapped / paginated response, and ingest it.** Real APIs wrap the list
under a key and paginate. Pull the array out, then map + upsert each page inside a
transaction so the whole sync is atomic and fast:

```c++
// { "next": "...", "results": [ {…}, {…} ] }
QiTransaction tx;
for (const QByteArray &page : pages) {                    // however you fetched them
    QJsonArray rows = QJsonDocument::fromJson(page).object().value("results").toArray();
    QiList<Article> batch = QiJsonMapper::map<Article>(rows);
    for (int i = 0; i < batch.size(); i++)
        batch.at(i)->upsert({"remote_id"});               // idempotent on refetch
}
tx.commit();
```

**Merge a partial update into an existing record.** `fromJson(&model, obj)` writes
only the keys present, leaving the rest of a loaded record intact — ideal for a
`PATCH`-style payload:

```c++
Article article;
article.load(Article::col().remote_id == 42);             // full row from the DB

QJsonObject patch = QJsonDocument::fromJson(R"({"title":"Edited"})").object();
QiJsonMapper::fromJson(&article, patch);                  // only `title` changes
article.save();                                           // published_at etc. preserved
```

### Loading JSON over HTTP on a worker thread

`QiJsonRequest` fetches JSON from an HTTP endpoint **on a background thread**,
maps it into models with `QiJsonMapper`, optionally saves them to the database,
and delivers the result on your thread via a Qt signal — so the calling (e.g.
GUI) thread never blocks.

This is an optional add-on that pulls in **QtNetwork**. Include the
network-enabled project file instead of `qivot.pri`:

```pro
QT += core network
include(path/to/qivot/src/qivot-network.pri)
```

```c++
#include <qijsonrequest.h>

QiJsonRequest *request = qiJsonRequest<Post>();           // typed factory
request->setConnection(connection);                       // save the mapped records
request->setRawHeader("Authorization", "Bearer <token>"); // optional headers

// IMPORTANT: pass a context object that lives on the thread you want the slot
// to run on (here `this`, a main-thread QObject). With a context object the
// connection is queued and the slot runs on that thread. A context-free lambda
// would run on the worker thread instead.
connect(request, &QiJsonRequest::loaded, this, [](QiSharedList records) {
    QiList<Post> posts = records;                 // QiSharedList -> QiList<Post>
    qDebug() << "loaded" << posts.size() << "posts";
});
connect(request, &QiJsonRequest::failed, this, [](QString message) {
    qWarning() << "request failed:" << message;
});

request->get(QUrl("https://example.com/api/posts"));   // returns immediately
```

The request object is a one-shot: it deletes itself once finished. The JSON root
may be an array of objects (mapped to many records) or a single object (one
record).

**Insert vs. upsert.** By default each fetched record is written with `save()`
(INSERT OR REPLACE by primary/unique key). To keep a table **in sync with a REST
API keyed by a natural id** — updating changed rows in place and inserting new
ones, without duplicating on repeated fetches — set the upsert key(s):

```c++
request->setUpsertKeys({"remoteId"});   // INSERT ... ON CONFLICT(remoteId) DO UPDATE
request->get(QUrl("https://example.com/api/posts"));
// run it again tomorrow: existing posts update in place, new ones insert
```

`setUpsertKeys()` switches the request to `Upsert` mode; the column(s) you pass
must be `UNIQUE` (or the primary key) on the model. Requires SQLite 3.24+.

**Saving on the worker thread.** When you call `setConnection()`, the mapped
records are saved on the worker using a *private* database connection: the
connection's parameters are captured on your thread and re-opened on the worker
with its own name, so Qivot never touches your primary `QSqlDatabase` from the
wrong thread. This relies on the new thread-safe open overload:

```c++
// open without touching / becoming the shared default connection — for use
// on a worker thread:
workerConnection.open(db, /* asDefault = */ false);
```

To map without saving (populate objects and hand them back for the caller to
persist), call `request->setSaveEnabled(false)` and don't set a connection.

> Saving requires SQLite and an already-open primary connection. A busy-timeout
> is set on the worker connection so concurrent writes wait for the file lock
> rather than failing; for heavy concurrent access enable WAL with
> `connection.setJournalMode("WAL")`. See
> [`examples/jsonhttp`](examples/jsonhttp) for a complete, runnable program (it
> starts a tiny local server so it runs offline).

### Exposing models to QML

Qivot models are plain (non-`QObject`) by design, so they're invisible to Qt's
meta-object system out of the box. To surface them to **QML**, opt a model in
with `Q_GADGET` + `QI_QML_FIELD` (from `<qigadget.h>`) — each field becomes a
`Q_PROPERTY`, while `save()`/queries keep working unchanged:

```c++
#include <qigadget.h>

class Post : public QiModel {
    Q_GADGET                          // literal — qmake's automoc needs the keyword
    QI_MODEL
    QI_QML_FIELD(int,     remoteId)   // = QiField<int> + a Q_PROPERTY
    QI_QML_FIELD(QString, title)
};
QI_DECLARE_MODEL(Post, "post",
                 QI_FIELD(remoteId, QiUnique | QiNotNull),
                 QI_FIELD(title));
```

> The model must live in a header listed in your `.pro`'s `HEADERS` so moc
> processes it. Write `Q_GADGET` **literally** — hiding it in a macro defeats
> qmake's automoc detection.

For a **list** bound to a QML `ListView`, use `QiListModel` — a
`QAbstractListModel` whose roles are the model's field names (no per-model
boilerplate; works for any model, gadget or not). Rather than pushing it in with
`setContextProperty`, wrap it in a small **QML-registered** controller
(`QML_ELEMENT`) that exposes the model, invokable slots, and `NOTIFY`-backed
properties — the modern, declarative way:

```c++
#include <QQmlEngine>          // QML_ELEMENT
#include <qilistmodel.h>

class PostStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *posts READ posts CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    QAbstractItemModel *posts()      { return &m_model; }
    QString status() const           { return m_status; }

    Q_INVOKABLE void reload() {                                   // a slot QML calls
        m_model.setList( Post::objects().orderBy(Post::col().remoteId.desc()).all() );
        m_status = QString("%1 posts").arg(m_model.count());
        emit statusChanged();                                    // drives QML bindings
    }
    Q_INVOKABLE void add(const QString &title, const QString &author) {
        Post p; p.remoteId = /* next id */; p.title = title; p.author = author;
        p.save();                                                // insert via the library
        reload();
    }
    Q_INVOKABLE void remove(int remoteId) {
        Post p;
        if (p.load(Post::col().remoteId == remoteId)) p.remove();  // delete via the library
        reload();
    }
    Q_INVOKABLE void search(const QString &text) {
        m_model.setList( QiQuery<Post>().search("post_fts", text).all() );
        emit statusChanged();
    }
signals:
    void statusChanged();
private:
    QiListModel m_model;
    QString     m_status;
};
```

Enable `QML_ELEMENT` in the `.pro` (`CONFIG += qmltypes`, `QML_IMPORT_NAME = Qivot`,
`QML_IMPORT_MAJOR_VERSION = 1`), then use it declaratively:

```qml
import QtQuick
import QtQuick.Controls
import Qivot

ApplicationWindow {
    PostStore { id: store; Component.onCompleted: reload() }   // create + call a slot

    header: ToolBar { Label { text: store.status } }           // binding to a NOTIFY property

    ListView {
        anchors.fill: parent
        model: store.posts                                     // the QiListModel
        delegate: RowLayout {                                  // roles == field names
            Label { text: "#" + remoteId + "  " + title; Layout.fillWidth: true }
            Button { text: "delete"; onClicked: store.remove(remoteId) }   // slot
        }
    }
    footer: RowLayout {
        TextField { id: t; placeholderText: "new title…"; Layout.fillWidth: true }
        Button { text: "add"; onClicked: { store.add(t.text, "me"); t.clear() } }   // slot
    }
}
```

See [`examples/qmlmodel`](examples/qmlmodel) for a complete, runnable Qt Quick app.

### Reactive queries (live models)

Make a `QiListModel` **live** and it re-runs its query automatically whenever its
table changes — so a bound `ListView` updates itself after any `save()` /
`remove()` / `update()`, from anywhere, with **no manual reload**:

```c++
QiListModel *tasks = new QiListModel(this);
tasks->setLive<Task>(connection, [] {
    return Task::objects().orderBy(Task::col().id.desc()).all();
});
// ...later, from anywhere:
Task t; t.title = "buy milk"; t.save();     // the ListView bound to `tasks` updates itself
```

How it works: every Qivot write (`save`, `remove`, `update`, batch `QiList::save`)
notifies its connection which table changed; live models watching that table
re-run their query, **coalesced** to one refresh per event-loop turn (a burst of
writes = a single update). It runs on the connection's thread — great for the
usual GUI-thread app. You can also observe changes directly:

```c++
int id = connection.addChangeHook([](const QString &table) {
    qDebug() << table << "changed";
});
// connection.removeChangeHook(id);
```

The runnable [`examples/reactive`](examples/reactive) app is a to-do list whose
view never calls reload — tick "auto-add" and watch rows appear on their own.

### Infinite scroll (lazy paging)

`QiLazyListModel` loads records a page at a time via `canFetchMore()` /
`fetchMore()`, so a `ListView` pulls the next `LIMIT`/`OFFSET` page as the user
scrolls — never loading the whole table up front:

```c++
auto *model = new QiLazyListModel(this);
model->setQuery( Item::objects().orderBy(Item::col().id.asc()), 50 );  // 50 rows / page
model->reset();                                                        // load the first page
```

```qml
// QML — the ListView fetches the next page automatically as you approach the end:
ListView {
    model: itemModel                       // a QiLazyListModel
    delegate: ItemDelegate { text: name }
    footer: BusyIndicator { running: !itemModel.atEnd }   // spinner until the last page
}
```

`count` (rows loaded) and `atEnd` are properties you can bind to. Use a stable
`orderBy` so paging stays consistent. The runnable
[`examples/infinitescroll`](examples/infinitescroll) app scrolls a 1,000-row
table 40 at a time.

### Putting it together: an iOS-style Contacts app

[`examples/contacts`](examples/contacts) is the showcase — a native-feeling
Contacts screen built entirely on the pieces above:

- a **reactive** `QiListModel` sorted by last name, so a new contact drops into
  the right place the instant it's saved;
- QML `ListView` **sticky A–Z sections** (`ViewSection.FirstCharacter` on the
  `lastName` role);
- an **A–Z scrubber** on the right that jumps via `store.indexForLetter(...)`
  (drag it like on iOS);
- **live search** that re-queries as you type;
- colored **avatar initials**, an add dialog, and iOS-style styling.

It's ~200 lines of QML over a ~60-line store — the ORM does the rest.

## Compatibility & requirements

- **Qt 5.15 and Qt 6** from a single codebase — no `QT_VERSION` guards. Minimum
  Qt 5.10 (for `QStringView`). Verified building and passing the test suite on Qt 5.15.
- **C++17** (`CONFIG += c++17`, set in `qivot.pri`).
- **SQLite** — a few features want a recent build (all standard in Qt's bundled SQLite):
  `upsert()` needs ≥ 3.24 · `RIGHT`/`FULL OUTER` joins need ≥ 3.39 · full-text search needs the FTS5 module.

<details>
<summary><strong>Qt 6 port &amp; C++17 modernization details</strong></summary>

<br>

Qivot's data-model core originated in DQuest (Qt 5 only). The APIs Qt 6 removed
are replaced with equivalents that build on both major versions:

- `QVariant::Type` → `QMetaType` ids · `QVariant::type()` → `userType()` vs `QMetaType::*` · `QVariant::typeToName()` → `QMetaType(id).name()` · `qVariantFromValue()` → `QVariant::fromValue()` · `QStringRef` → `QStringView` · `QRegExp` → `QRegularExpression`

Modernization:

- `override` on virtuals; `[[nodiscard]]` on `exec()`, `remove()`, `open()`, `createTables()`/`dropTables()`, `createIndex()`/`dropIndex()`.
- `open(db, asDefault)` opens a connection without touching the shared default — used for per-thread connections (see [`QiJsonRequest`](#loading-json-over-http-on-a-worker-thread)).
- `QiQuery<T>` is **typed end-to-end**: `Model::objects()` is a `QiQuery<Model>`, the fluent methods return `QiQuery<T>`, and `all()` returns `QiList<T>` — so results deduce correctly with `auto` and element access stays typed. (Converting a raw `QiSharedList`, e.g. the records from `QiJsonRequest::loaded`, still needs the explicit `QiList<Post> posts = records;`.)

</details>

## Limitations

- SQLite only (FTS5, `WITHOUT ROWID`, savepoints and the SQL dialect assume it).
- The typed query builder doesn't cover every construct (correlated subqueries,
  CTEs, window functions) — use [`qiRawQuery<T>`](examples/dashboard) for those; it
  still returns typed models.
- No `CREATE TRIGGER` in the public API (triggers are used internally for FTS sync).
- Multithreading uses one connection per thread — [`QiConnectionPool`](examples/dashboard)
  (and `QiAsync`) handle this for you; a `:memory:` database can't be shared across
  threads, so async needs a file-based database.
- `save()` uses `REPLACE`, so updating a row that is referenced by a foreign key is
  rejected under FK enforcement — use [`upsert()`](#updating-and-deleting)
  (non-destructive) or `connection.setForeignKeysEnforced(false)`.

## Credits & license

Qivot builds on the wonderful [DQuest](https://github.com/benlau/dquest) project
by Ben Lau ("DQuest is Database Quest") — its elegant data-model design lives on
in Qivot's `Qi` classes. Huge thanks.

© 2026 Austin Kottke. Qivot is licensed under the **MIT** license — use it in
open- or closed-source applications (see [`LICENSE.txt`](LICENSE.txt)). It
includes portions derived from DQuest (© 2010 Ben Lau) under the BSD-3-Clause
license; that notice is retained in [`NOTICE.txt`](NOTICE.txt). If Qivot helps
your project, letting us know encourages further development.
