# Tutorial — versioned schema migrations

Evolve a database's schema over time with **`QiMigrator`**. It tracks the schema
version in SQLite's `PRAGMA user_version`, runs pending migrations in order (each
in its own transaction), and skips ones already applied — so `migrate()` is safe
to call on every startup.

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

On a fresh database this runs all three, advancing `user_version` from 0 to 3:

```text
Fresh database.
  version: 0  target: 3

migrate() applied 3 migrations -> version 3
  note columns: id, title, body, pinned
```

## Step 3 — It's idempotent

Call it again and nothing happens — every registered migration is already at or
below `user_version`:

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

If a step fails, its transaction is rolled back and `user_version` is left
untouched, so the database never ends up half-migrated:

```text
Broken v5: migrate() returns -1 (-1 = failed)
  version still: 4
  error: migration 5 (intentionally broken) failed: the migration step returned false
```

---

## Files

| File | Role |
|---|---|
| `main.cpp` | Registers versions 1–5 and drives migrate() through every case. |

## See also

- [`schema`](../schema) — the schema features (keys, constraints, relations) your
  migrations build up.
- [`relations`](../relations) — many-to-many, custom types, hooks, timestamps,
  and soft delete.
