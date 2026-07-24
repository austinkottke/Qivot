# Qivot examples

Runnable programs that each focus on one part of the library, from a two-model
"hello world" up to a 10,000-row iOS-style Contacts app. Every example is a
self-contained qmake project; console demos also build under CMake.

## Building

All examples at once (qmake):

```sh
cd examples
qmake examples.pro && make        # binaries land in each subdirectory
```

A single example:

```sh
cd examples/contacts
qmake && make
./contacts                        # GUI examples open a window
```

Console examples print to stdout and exit. GUI examples (`qmlmodel`,
`reactive`, `infinitescroll`, `contacts`) open a Qt Quick window. Most GUI
examples honor `QIVOT_SELFTEST=1`, which drives a scripted interaction and then
quits — handy for headless/offscreen CI (`QT_QPA_PLATFORM=offscreen`).

## The examples

### Fundamentals

| Example | What it shows |
|---|---|
| [`tutorial1`](tutorial1) | The smallest program: define a `QiModel`, create the table, `save()` a record. |
| [`tutorial2`](tutorial2) | Queries — `QiQuery`, filtering with `QiWhere`, loading results. |
| [`tutorial3`](tutorial3) | Foreign keys and loading a related model. |
| [`tutorial4`](tutorial4) | Updating and deleting records. |
| [`tutorial5`](tutorial5) | Ordering, limits, and aggregate queries. |
| [`index`](index) | Creating indexes with `QiIndex`. |

### Schema & data

| Example | What it shows |
|---|---|
| [`schema`](schema) | 📖 **Tutorial** — the expressive schema layer in one program: string primary keys (no auto id), `WITHOUT ROWID`, composite keys, `CHECK` constraints, FK referential actions, enum fields, and rename/drop-column migrations. → [walkthrough](schema/README.md) |
| [`migrations`](migrations) | 📖 **Tutorial** — versioned schema migrations with `QiMigrator`, tracked by `PRAGMA user_version`: ordered, transactional, idempotent, with rollback on failure. → [walkthrough](migrations/README.md) |
| [`relations`](relations) | 📖 **Tutorial** — many-to-many join tables (`qiAttach`/`qiManyToMany`), custom type converters (`QI_DECLARE_CONVERTER`), lifecycle hooks (`clean()`/`afterSave()`), auto `createdAt`/`updatedAt`, and soft delete (`softRemove` + `qiAlive`/`qiTrashed`). → [walkthrough](relations/README.md) |
| [`keyset`](keyset) | 📖 **Tutorial** — keyset / cursor pagination with `QiKeyset`: seek past the last key (`WHERE id > :cursor`) instead of `OFFSET`, so deep paging stays O(log n). Resumable cursors, descending + filtered paging. → [walkthrough](keyset/README.md) |
| [`asyncquery`](asyncquery) | 📖 **Tutorial** — run queries off the UI thread with `QiAsync`, returning a `QFuture` (per-thread connection, `QT += concurrent`). The main thread keeps ticking while a 200k-row fetch runs on a worker. → [walkthrough](asyncquery/README.md) |
| [`jsonhttp`](jsonhttp) | `QiJsonMapper` (synchronous JSON ↔ model, QtCore only) plus `QiJsonRequest` (fetch JSON over HTTP on a worker thread and map it). Runs offline against a tiny embedded server. |
| [`jsonnested`](jsonnested) | 📖 **Tutorial** — deserializing a nested JSON document (an order embedding its customer and line items) into a graph of related models by composing the mapper. → [walkthrough](jsonnested/README.md) |

### Qt Quick / reactive UI

| Example | What it shows |
|---|---|
| [`qmlmodel`](qmlmodel) | 📖 **Tutorial** — modern QML integration: a `Q_GADGET` model, a `QML_ELEMENT` controller, and a `QiListModel` bound to a `ListView`, with add / remove / full-text search. → [walkthrough](qmlmodel/README.md) |
| [`reactive`](reactive) | 📖 **Tutorial** — *Reactive Qivot*: a to-do list bound to a *live* `QiListModel`. Every `save()`/`remove()` refreshes the view automatically; nothing calls "reload". → [walkthrough](reactive/README.md) |
| [`manytomany`](manytomany) | 📖 **Tutorial** — a music-library app showing **both relations**: **one-to-many** (`Artist --< Song` via `QI_HAS_MANY`, an Artists tab) and **bidirectional many-to-many** (`song.playlists()` ↔ `playlist.songs()` via `QI_MANY_TO_MANY`, join table auto-created). Toggle links from either side; the UI updates live. → [walkthrough](manytomany/README.md) |
| [`dashboard`](dashboard) | 📖 **Tutorial** — a **Query Playground**: 8 numbered, step-by-step cards, each one small query shown with its code and live result — `count` → `sum`/`avg` → filter → order+limit → a one-to-many → a **window function** (`qiRawQuery`) → running a query **off the UI thread** (`QiAsync`). → [walkthrough](dashboard/README.md) |
| [`prefetch`](prefetch) | 📖 **Tutorial** — **eager loading**: load a one-to-many for a whole list in **2 queries** (`qiPrefetchHasMany`) instead of N+1, with a live counter of the *actual* SQL executed. → [walkthrough](prefetch/README.md) |
| [`savepoints`](savepoints) | 📖 **Tutorial** — **nested transactions**: open nested scopes (`BEGIN` → `SAVEPOINT`), edit rows, and roll a scope back to undo just its edits while outer scopes keep theirs. → [walkthrough](savepoints/README.md) |
| [`infinitescroll`](infinitescroll) | 📖 **Tutorial** — a gradient discovery feed. A `QiLazyListModel` streams 5,000 rows 24 at a time via `canFetchMore()`/`fetchMore()` (append-on-scroll paging), with a footer that names the exact rows it's fetching. → [walkthrough](infinitescroll/README.md) |
| [`contacts`](contacts) | 📖 **Tutorial (the showcase)** — an iOS-style Contacts app over 10,000 records on a `QiWindowedListModel`: it counts up front but *fetches only the pages you scroll to* (`LIMIT/OFFSET`), with sticky A–Z sections, a drag-to-jump index (offset via `count(*)`), live search, and a scroll-position HUD. → [walkthrough](contacts/README.md) |
| [`fluxo`](fluxo) | 📖 **Tutorial (the write showcase)** — beginner-friendly, step-by-step: a particle recorder that batch-writes **every particle, every frame** to SQLite on WAL (~100k row-writes/sec), bounds the table with retention pruning, and scrubs a timeline to **replay any past frame from the database**. → [walkthrough](fluxo/README.md) |
