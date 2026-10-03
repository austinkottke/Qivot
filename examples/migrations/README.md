# Tutorial — versioned schema migrations

Evolve a database's schema over time with **`QiMigrator`**. It records what ran
in a `qivot_migrations` table, runs pending migrations in order (each in its own
transaction), and skips ones already applied, so `migrate()` is safe to call on
every startup. Migrations can be code, SQL text, or a folder of `.sql` files, and
the same migrator runs on SQLite, PostgreSQL, MySQL, SQL Server and DuckDB.

> **Run it**
> ```sh
> cd examples/migrations
> qmake && make
> ./migrations
> ```

---

## Step 1 — Register your migrations

Each migration has a version number, a name, and a step `bool(QiConnection&)`.
`add()` order doesn't matter — they always run in ascending version order.

```cpp
QiMigrator migrator(conn);

migrator.add(1, "create note table", [](QiConnection &c) {
    return c.query().exec(
        "CREATE TABLE note (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL)");
});
migrator.add(2, "add body column", [](QiConnection &c) {
    return c.query().exec("ALTER TABLE note ADD COLUMN body TEXT");
});
migrator.add(3, "add pinned flag + index", [](QiConnection &c) {
    return c.query().exec("ALTER TABLE note ADD COLUMN pinned INTEGER NOT NULL DEFAULT 0")
        && c.query().exec("CREATE INDEX idx_note_pinned ON note(pinned)");
});
```

A step can run raw SQL (shown here) or any Qivot API — `c.createTables()`,
`c.renameColumn<T>(...)`, `c.dropColumn<T>(...)`, batched writes, etc.

## Step 2 — Migrate

```cpp
int applied = migrator.migrate();   // number run this call, or -1 on failure
```

On a fresh database this runs all three and records versions 1–3:

```text
Fresh database.
  version: 0  target: 3

migrate() applied 3 migrations -> version 3
  note columns: id, title, body, pinned
```

## Step 3 — It's idempotent

Call it again and nothing happens: every registered migration is already recorded:

```text
migrate() again applied 0 (idempotent)
```

That's why you can call `migrate()` unconditionally on startup.

## Step 4 — Ship a new version later

Register a higher version and migrate again; only the new one runs:

```cpp
migrator.add(4, "backfill a welcome note", [](QiConnection &c) {
    QSqlQuery q = c.query();
    q.prepare("INSERT INTO note (title, body, pinned) VALUES (?, ?, 1)");
    q.addBindValue("Welcome"); q.addBindValue("Your first note.");
    return q.exec();
});
```

```text
Added v4; migrate() applied 1 -> version 4
  note rows: 1
```

## Step 5 — Failures roll back

If a step fails, its transaction is rolled back and nothing is recorded, so the
database never ends up half-migrated:

```text
Broken v5: migrate() returns -1 (-1 = failed)
  version still: 4
  error: migration 5 (intentionally broken) failed: the migration step returned false
```

## Step 6 — Migrations as `.sql` files

Most projects keep migrations as files. Name them `NNNN_name.sql`, or
`NNNN_name.up.sql` with a matching `NNNN_name.down.sql`, and load the folder —
from disk, or from a Qt resource as here ([`migrations.qrc`](migrations.qrc)):

```text
sql/0001_create_tags.sql
sql/0002_seed_tags.up.sql      sql/0002_seed_tags.down.sql
sql/0003_tag_counts.up.sql     sql/0003_tag_counts.down.sql
```

```cpp
QiMigrator files(conn);
files.setTable("tag_migrations");          // a second history beside the first
files.addDirectory(":/migrations");
for (const QiMigrator::Migration &m : files.pending()) …
files.migrate();
```

A file can hold several statements. `0003_tag_counts.up.sql` adds a column and a
trigger whose `BEGIN … END` body has semicolons of its own; it goes to SQLite in
one piece. `status()` then reports when each one ran and how long it took:

```text
From :/migrations: 3 files
  pending v1 create tags
  pending v2 seed tags (has a down step)
  pending v3 tag counts (has a down step)
migrate() applied 3 -> version 3
  tag uses: work=1, home=0, someday=0
  v1 create tags  applied 2026-10-03T21:29:12Z in 0 ms
```

## Step 7 — Edited migrations are caught

Each SQL migration is recorded with a SHA-256 of its text. If a migration that
already ran is changed, `migrate()` stops instead of carrying on with a database
that no longer matches the files:

```text
Edited v1: migrate() returns -1
  error: migration 1 (create tags) was changed after it was applied; undo the edit, or accept it with acceptChecksums()
```

## Step 8 — Roll back

`rollback(n)` runs the down steps of everything newer than *n*, newest first:

```text
rollback(1) undid 2 -> version 1
  tag columns: id, name
```

---

## Files

| File | Role |
|---|---|
| `main.cpp` | Part 1: code migrations 1–5 through every case. Part 2: the `.sql` files, checksums and rollback. |
| `sql/` | The migration files, `NNNN_name.sql` / `.up.sql` / `.down.sql`. |
| `migrations.qrc` | Puts `sql/` into the binary as `:/migrations`. |

## See also

- [`schema`](../schema) — the schema features (keys, constraints, relations) your
  migrations build up.
- [`relations`](../relations) — many-to-many, custom types, hooks, timestamps,
  and soft delete.
