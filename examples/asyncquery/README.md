# Tutorial — asynchronous queries off the UI thread

Run an expensive query on a background thread and get the result back as a
`QFuture`, so the main (event-loop / UI) thread never blocks. This is
**`QiAsync`**.

> **Run it**
> ```sh
> cd examples/asyncquery
> qmake && make
> ./asyncquery
> ```

---

## Why

Every Qivot query runs synchronously on the calling thread. Fetch 200k rows on
your UI thread and the UI freezes for the duration. `QiAsync` moves that work to
Qt's global thread pool. Because SQLite requires **one connection per thread**,
it opens (and reuses) a separate connection per worker thread to the same
database file, and hands your callable a `QiConnection` for that thread.

## Step 1 — Configure and go

```cpp
#include <qiasync.h>                 // NOT part of <qivot.h> — pulls in QtConcurrent

QiAsync::configure("QSQLITE", "app.db");        // once, at startup

QFuture<int> f = QiAsync::run([](QiConnection &c) -> int {
    return Event::objects(c).all().size();      // runs on a worker thread
});
```

Two requirements:

- `QT += concurrent` in your `.pro` (see [asyncquery.pro](asyncquery.pro)).
- A **file-based** database — a `:memory:` database is private to each
  connection, so a worker couldn't see the main thread's data.

## Step 2 — Get the result without blocking

Use a `QFutureWatcher` to be notified on the main thread when it's done:

```cpp
auto *watcher = new QFutureWatcher<int>(&app);
QObject::connect(watcher, &QFutureWatcherBase::finished, &app, [&]{
    qInfo() << "result:" << watcher->result();
    app.quit();
});
watcher->setFuture(f);
```

## Step 3 — Prove the main thread stays live

The example runs a `QTimer` printing "tick" every 15 ms while the 200k-row fetch
runs on a worker. The ticks keep coming — the event loop is never blocked:

```text
Seeding 200000 rows…
main thread: 0x1ede4bac0
  . main-thread tick 1 (not blocked)
  . main-thread tick 2 (not blocked)
  . main-thread tick 3 (not blocked)
  . main-thread tick 4 (not blocked)
  . main-thread tick 5 (not blocked)
  . main-thread tick 6 (not blocked)
  [worker 0x16d58f000 ] fetched 200000 rows in 92 ms
  <- result: 200000 rows, wall time 93 ms
```

Note the **worker thread id differs from the main thread id** — the fetch really
did run elsewhere, and finished in ~the same wall time the main thread spent
ticking freely.

## Notes

- **Concurrency & WAL.** The example sets `conn.setJournalMode("WAL")` so the
  worker's read connection doesn't block (and isn't blocked by) the main
  connection. Fire several `QiAsync::run` calls at once and they run in parallel
  across pool threads.
- **A benign Qt notice.** While *building* a query object on a worker thread,
  Qivot briefly resolves the default connection's handle, so Qt may print
  `requested database does not belong to the calling thread`. The query itself
  executes on the worker's own connection (`exec()` → `connection.query()`), so
  the result is correct; this example installs a small message handler that
  filters just that one line.

## Files

| File | Role |
|---|---|
| `main.cpp` | Seeds a big table, runs the fetch via `QiAsync`, and ticks the main thread to prove it stays responsive. |
| `asyncquery.pro` | Adds `QT += concurrent`. |

## See also

- [`keyset`](../keyset) — page huge tables efficiently; pairs naturally with async.
- [`jsonhttp`](../jsonhttp) — the other threaded workload: fetch JSON over HTTP off-thread.
