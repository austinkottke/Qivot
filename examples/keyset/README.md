# Tutorial — keyset (cursor) pagination

Page through a large table with **`QiKeyset`** — the fast, scalable alternative
to `LIMIT/OFFSET`. Instead of skipping rows (which SQLite re-scans, getting
slower the deeper you go), keyset paging remembers the last row's key and seeks
*past* it: `WHERE id > :cursor ORDER BY id LIMIT n`. Every page is one index
seek, so page 1 and page 10,000 cost the same.

> **Run it**
> ```sh
> cd examples/keyset
> qmake && make
> ./keyset
> ```

---

## Step 1 — A model with a unique key

Keyset paging seeks on a unique column that matches the sort order — usually the
primary key.

```cpp
class Event : public QiModel {
    QI_MODEL
public:
    QiField<QString> title;
};
QI_DECLARE_MODEL(Event, "event", QI_FIELD(title));
```

## Step 2 — Page forward

Construct a pager with the key column and page size, then loop until `atEnd()`.

```cpp
QiKeyset<Event> pager("id", 100);        // key column, rows per page
while (!pager.atEnd()) {
    QiList<Event> page = pager.next();
    // ... process page ...
}
```

```text
Paging 250 events, 100 per page:
  page 1 : 100 rows, id 1 -> 100
  page 2 : 100 rows, id 101 -> 200
  page 3 : 50 rows, id 201 -> 250
```

Each `next()` runs `… WHERE id > :lastId ORDER BY id ASC LIMIT 100` — no `OFFSET`,
so it never re-scans the rows you've already passed.

## Step 3 — Save a cursor, resume later

`cursor()` returns the last key seen — a compact, stable token you can persist
(in a URL, a session, a "load more" button). A fresh pager `seek()`s straight
back to it:

```cpp
QiKeyset<Event> p1("id", 100);
p1.next();                          // page 1 (ids 1..100)
QVariant cursor = p1.cursor();      // == 100; store it anywhere

QiKeyset<Event> p2("id", 100);
p2.seek(cursor);                    // brand-new pager, resumed
p2.next();                          // continues at id 101
```

```text
Resume from a saved cursor:
  saved cursor (last id of page 1): 100
  page 2 : 100 rows, id 101 -> 200
```

## Step 4 — Descending, and filtered subsets

Page newest-first, and pass a base filter that's ANDed into every page:

```cpp
QiKeyset<Event> filtered("id", 50, /*ascending=*/false,
                         Event::col().id <= 120);
```

```text
Descending, filtered (id <= 120), 50 per page:
  page 1 : 50 rows, id 120 -> 71
  page 2 : 50 rows, id 70 -> 21
  page 3 : 20 rows, id 20 -> 1
```

---

## Keyset vs. OFFSET

| | `LIMIT/OFFSET` | `QiKeyset` |
|---|---|---|
| Deep page cost | O(offset) — re-scans skipped rows | O(log n) — one index seek |
| Stable under inserts | no (rows shift) | yes (seeks by key) |
| Resumable token | page number (fragile) | a key cursor (stable) |
| Random page jump | yes | no (sequential) |

Use `QiKeyset` for feeds, "load more", exports, and any deep scrolling; keep
`OFFSET` (or [`QiWindowedListModel`](../contacts)) when you need to jump to an
arbitrary page.

## Files

| File | Role |
|---|---|
| `main.cpp` | The `Event` model and a tour of forward paging, cursors, and filtered/descending paging. |

## See also

- [`asyncquery`](../asyncquery) — run these queries off the UI thread.
- [`infinitescroll`](../infinitescroll) / [`contacts`](../contacts) — paged QML list models.
