# Integration tests — live MySQL, PostgreSQL & SQL Server

The unit tests in [`../unittests`](../unittests) cover **SQL generation** for every backend
(including Oracle) and need no database. To exercise MySQL/MariaDB, PostgreSQL, and SQL
Server *end to end* (real `CREATE TABLE`, insert, query, upsert, id-return), you need
running servers. (Oracle isn't wired into this live suite yet — see the note in
[main.cpp](main.cpp) and the "Oracle" section below.)

## 1. Start the databases

```bash
docker compose -f tests/integration/docker-compose.yml up -d
```

This starts MariaDB on `127.0.0.1:3306`, PostgreSQL on `127.0.0.1:5432` (both with a
`qivot_test` database, password `qivot`), and SQL Server on `127.0.0.1:1433` (SA password
`Qivot_Test1` — SQL Server enforces a password-complexity policy the other two don't, so it
can't reuse the plain `qivot` password).

> **Port note:** if host port 3306 is already taken (a local MySQL), start MariaDB on another port,
> e.g. `docker run -d --name qivot-mysql-3307 -e MARIADB_ROOT_PASSWORD=qivot -e MARIADB_DATABASE=qivot_test -p 3307:3306 mariadb:11`,
> and use `3307` below.

Unlike MariaDB/Postgres, the SQL Server image has no "create this database for me" env var —
create `qivot_test` by hand once the container is healthy:

```bash
docker exec integration-mssql-1 /opt/mssql-tools18/bin/sqlcmd \
    -S localhost -U sa -P "Qivot_Test1" -C \
    -Q "IF DB_ID('qivot_test') IS NULL CREATE DATABASE qivot_test;"
```

## 2. The integration test program

`integration.pro` builds a small program that runs a full round-trip against a live server and
checks the cross-dialect paths the unit tests can't:

```bash
cd tests/integration && qmake && make
./integration sqlite        # -> in-memory SQLite, no server needed
./integration postgres      # -> connects to 127.0.0.1:5432
./integration mysql         # -> connects to 127.0.0.1:3307 (or 3306)
./integration sqlserver     # -> connects to 127.0.0.1:1433 (see the macOS caveat below)
```

The **exact same suite** runs on every backend, so `./integration sqlite` validates all the ORM
logic locally with zero setup before you ever touch a server. It checks:

- the returned auto-increment id (Postgres `SELECT lastval()`, SQL Server
  `SELECT SCOPE_IDENTITY()`, vs MySQL/SQLite `lastInsertId`)
- a 600-char string surviving intact (MySQL `TEXT`/SQL Server `NVARCHAR(MAX)`, not a
  truncating bounded type)
- `double` / `bool` / JSON / `QDate` / `QDateTime` round-trips
- **batch** insert (`QiList::save`) assigning a distinct id back to every row
- **transactions**: a rollback discards the row, a commit persists it
- a **string primary key** model (no auto id): save, load, and re-save updating in place
  (on Postgres/SQL Server this exercises `save()`'s `REPLACE`→upsert translation, `ON CONFLICT`
  or `MERGE` respectively, on a non-`id` key)
- the **migration** path: a second `createTables()` is a safe no-op (portable column reading)
- **upsert** on a unique key updating in place instead of duplicating

If the driver plugin is missing or no server answers, it **skips** (exit 0), so it's safe to run
anywhere — unless `QIVOT_REQUIRE_DB=1`, which turns those into hard failures.

Connection details come from environment variables (per-backend defaults in parentheses):

| Var | MySQL default | Postgres default | SQL Server default |
|-----|---------------|-------------------|---------------------|
| `QIVOT_HOST` | `127.0.0.1` | `127.0.0.1` | `127.0.0.1` |
| `QIVOT_PORT` | `3306` | `5432` | `1433` |
| `QIVOT_NAME` | `qivot_test` | `qivot_test` | `qivot_test` |
| `QIVOT_USER` | `root` | `postgres` | `sa` |
| `QIVOT_PASS` | `qivot` | `qivot` | `Qivot_Test1` |
| `QIVOT_ODBC_DRIVER` | — | — | `ODBC Driver 18 for SQL Server` (the installed driver name) |
| `QIVOT_REQUIRE_DB` | (unset = skip on failure) | (set = fail on failure) | (unset = skip on failure) |

Unlike the other two, SQL Server's `QODBC` driver connects via a full ODBC connection string
(built from the vars above) rather than discrete host/port/user/pass fields — see
`main.cpp`'s `isOdbc` branch if you need to add connection-string options.

**CI runs MySQL, Postgres, and SQL Server automatically.** The `db-integration` job in
`.github/workflows/ci.yml` boots all three as service containers, installs the distro Qt SQL
driver plugins (`libqt6sql6-mysql` / `libqt6sql6-psql` / `libqt6sql6-odbc` + Microsoft's own
`msodbcsql18` package from Microsoft's apt repo), and runs all three backends with
`QIVOT_REQUIRE_DB=1`. Oracle is **not** in CI yet (see below).

### macOS caveat — the Qt SQL driver plugins (MySQL/Postgres)

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

### macOS caveat — SQL Server (`QODBC`) needs more than a library path, and may not work at all

This one is heavier than the MySQL/Postgres case above, and — on a stock macOS + Homebrew Qt
setup — **may not be resolvable locally at all**, for a reason specific to macOS.

**Setup** (this part works): install unixODBC and Microsoft's driver via Homebrew, then verify
the driver actually registered:

```bash
brew tap microsoft/mssql-release
ACCEPT_EULA=Y brew install msodbcsql18 mssql-tools18
odbcinst -q -d   # should list "[ODBC Driver 18 for SQL Server]"
```

**The actual problem**: macOS has *two* separate, incompatible ODBC driver managers —
**iODBC** (Apple's old bundled default, at `/usr/lib/libiodbc*`) and **unixODBC** (the
Homebrew-installed one above, and the standard on Linux). Prebuilt Qt for macOS links its
`QODBC` plugin against **iODBC**, but `brew install msodbcsql18` registers the driver with
**unixODBC**'s config — two different driver registries that don't share data. Pointing iODBC
at unixODBC's config file (`ODBCINSTINI=/usr/local/etc/odbcinst.ini ODBCSYSINI=/usr/local/etc
./integration sqlserver`) gets iODBC to *find* the driver by name (past the initial "driver
could not be loaded" error), but the actual connection then **hangs indefinitely** — tested
directly against a live container, confirmed to be iODBC-specific: `isql` (unixODBC's own test
client) connects to the *same* container in under a second with the *same* credentials, so the
server, network, and TLS/cert trust settings are all fine. The hang is a documented class of
real-world friction between iODBC and Microsoft's driver — Microsoft's own ODBC documentation
for macOS/Linux only supports unixODBC, not iODBC, so this isn't really a "fixable via more
config" problem with the prebuilt Qt binaries most people have installed.

If you hit this: the reliable options are (a) a Qt build whose `QODBC` plugin is linked against
unixODBC instead (rebuilding Qt's sqldrivers plugin from source against unixODBC, or a Qt
distribution that already does this), or (b) just trust the unit tests + CI for SQL Server
correctness and use `isql`/`sqlcmd` locally if you need to poke at the live database directly.
**This is a local macOS-only issue** — Debian/Ubuntu's `libqt6sql6-odbc` package (what CI
installs) links against unixODBC by default, since that's the standard driver manager on
Linux, so CI is expected not to hit this at all.

### Validate the generated SQL without the Qt driver

Even when the Qt plugin won't load, you can prove the **generated SQL** is valid on a real server —
the test can print it, and you feed it straight to `psql` / `mariadb` / `sqlcmd`:

```bash
./integration print-postgres   | docker exec -i integration-postgres-1 psql -U postgres -d qivot_test
./integration print-mysql      | docker exec -i qivot-mysql-3307 mariadb -uroot -pqivot qivot_test
./integration print-sqlserver  # prints CREATE/INSERT/MERGE — feed to sqlcmd the same way
```

This is how the Postgres identity / `lastval()` / `ON CONFLICT`, MySQL `AUTO_INCREMENT` / `TEXT` /
`ON DUPLICATE KEY UPDATE`, and SQL Server `IDENTITY` / `SCOPE_IDENTITY()` / `MERGE` paths were
verified against Postgres 16, MariaDB 11, and SQL Server 2022 (the last one verified this way
specifically *because* of the macOS `QODBC` caveat above — the generated SQL was hand-fed to
`sqlcmd` inside the container and confirmed valid, even though the Qt-driver round-trip could
not be run locally).

### Oracle

Oracle isn't wired into this live-server suite. Qt's `QOCI` plugin isn't packaged for common
Linux distros the way `QMYSQL`/`QPSQL`/`QODBC` are — it may need to be built from Qt's own
source against Oracle Instant Client, which is a bigger CI investment than the other three
backends needed. `src/qioraclestatement.h`/`.cpp` are covered by the SQL-generation unit tests
in `../unittests` instead, which need no live database.

## 3. Tear down

```bash
docker compose -f tests/integration/docker-compose.yml down
docker rm -f qivot-mysql-3307 2>/dev/null   # if you started MariaDB on the alt port
```

## Notes

- **FTS** (`QiQuery::search`) is SQLite-only; skip those assertions on MySQL/Postgres/SQL Server.
- On MySQL a plain `QString` column is `TEXT` (no truncation); a keyed/unique one is `VARCHAR(255)`
  so it can be indexed. SQL Server splits the same way (`NVARCHAR(MAX)` / `NVARCHAR(450)`). JSON is
  stored as text on every backend (portable, parameter-safe).
- These integration tests are intentionally kept out of the default unit-test build so `make`
  stays database-free.
