# Integration tests — live MySQL & PostgreSQL

The unit tests in [`../unittests`](../unittests) cover **SQL generation** for every backend and
need no database. To exercise the MySQL/MariaDB and PostgreSQL paths *end to end* (real
`CREATE TABLE`, insert, query, upsert, id-return), you need running servers.

## 1. Start the databases

```bash
docker compose -f tests/integration/docker-compose.yml up -d
```

This starts MariaDB on `127.0.0.1:3306` and PostgreSQL on `127.0.0.1:5432`, each with a
`qivot_test` database (password `qivot`).

> **Port note:** if host port 3306 is already taken (a local MySQL), start MariaDB on another port,
> e.g. `docker run -d --name qivot-mysql-3307 -e MARIADB_ROOT_PASSWORD=qivot -e MARIADB_DATABASE=qivot_test -p 3307:3306 mariadb:11`,
> and use `3307` below.

## 2. The integration test program

`integration.pro` builds a small program that runs a full round-trip against a live server and
checks the cross-dialect paths the unit tests can't:

```bash
cd tests/integration && qmake && make
./integration sqlite        # -> in-memory SQLite, no server needed
./integration postgres      # -> connects to 127.0.0.1:5432
./integration mysql         # -> connects to 127.0.0.1:3307 (or 3306)
```

The **exact same suite** runs on every backend, so `./integration sqlite` validates all the ORM
logic locally with zero setup before you ever touch a server. It checks:

- the returned auto-increment id (Postgres `SELECT lastval()` vs MySQL/SQLite `lastInsertId`)
- a 600-char string surviving intact (MySQL `TEXT`, not a truncating `VARCHAR`)
- `double` / `bool` / JSON / `QDate` / `QDateTime` round-trips
- **batch** insert (`QiList::save`) assigning a distinct id back to every row
- **transactions**: a rollback discards the row, a commit persists it
- a **string primary key** model (no auto id): save, load, and re-save updating in place
  (on Postgres this exercises `save()`'s `REPLACE`→`ON CONFLICT` translation on a non-`id` key)
- the **migration** path: a second `createTables()` is a safe no-op (portable column reading)
- **upsert** on a unique key updating in place instead of duplicating

If the driver plugin is missing or no server answers, it **skips** (exit 0), so it's safe to run
anywhere — unless `QIVOT_REQUIRE_DB=1`, which turns those into hard failures.

Connection details come from environment variables (per-backend defaults in parentheses):

| Var | MySQL default | Postgres default |
|-----|---------------|------------------|
| `QIVOT_HOST` | `127.0.0.1` | `127.0.0.1` |
| `QIVOT_PORT` | `3306` | `5432` |
| `QIVOT_NAME` | `qivot_test` | `qivot_test` |
| `QIVOT_USER` | `root` | `postgres` |
| `QIVOT_PASS` | `qivot` | `qivot` |
| `QIVOT_REQUIRE_DB` | (unset = skip on failure) | (set = fail on failure) |

**CI runs this automatically.** The `db-integration` job in `.github/workflows/ci.yml` boots MariaDB
and PostgreSQL as service containers, installs the distro Qt SQL driver plugins
(`libqt6sql6-mysql` / `libqt6sql6-psql`), and runs both backends with `QIVOT_REQUIRE_DB=1`.

### macOS caveat — the Qt SQL driver plugins

The test needs Qt's **QMYSQL** and **QPSQL** plugins to actually *load*, which means their client
libraries (`libpq`, `libmysqlclient`) must be resolvable at runtime. Prebuilt Qt plugins often
hard-code stale paths (old Postgres.app / OpenSSL / MacPorts locations). If you see
`QPSQL driver not loaded`, point the loader at your real client libs:

```bash
DYLD_FALLBACK_LIBRARY_PATH=/usr/local/lib ./integration postgres
```

If the plugin was linked against a client lib you don't have (e.g. `libssl.1.0.0`), you'll need a Qt
whose SQL plugins match your installed client libraries. On Linux/CI this is usually just installing
`libqt*sql-psql` / `libqt*sql-mysql` (or the distro Qt), which is why CI is the natural home for the
live run.

### Validate the generated SQL without the Qt driver

Even when the Qt plugin won't load, you can prove the **generated SQL** is valid on a real server —
the test can print it, and you feed it straight to `psql` / `mariadb`:

```bash
./integration print-postgres | docker exec -i integration-postgres-1 psql -U postgres -d qivot_test
./integration print-mysql    | docker exec -i qivot-mysql-3307 mariadb -uroot -pqivot qivot_test
```

This is how the Postgres identity / `lastval()` / `ON CONFLICT` and MySQL `AUTO_INCREMENT` / `TEXT` /
`ON DUPLICATE KEY UPDATE` paths were verified against Postgres 16 and MariaDB 11.

## 3. Tear down

```bash
docker compose -f tests/integration/docker-compose.yml down
docker rm -f qivot-mysql-3307 2>/dev/null   # if you started MariaDB on the alt port
```

## Notes

- **FTS** (`QiQuery::search`) is SQLite-only; skip those assertions on MySQL/Postgres.
- On MySQL a plain `QString` column is `TEXT` (no truncation); a keyed/unique one is `VARCHAR(255)`
  so it can be indexed. JSON is stored as `TEXT` on every backend (portable, parameter-safe).
- These integration tests are intentionally kept out of the default unit-test build so `make`
  stays database-free. Wire them into CI once a MySQL/Postgres service is available in the pipeline.
