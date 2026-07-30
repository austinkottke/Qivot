# multidb — one codebase, three databases

This example proves the headline of Qivot's multi-database support: **you write the
model and the queries once, and the same binary runs on SQLite, MySQL/MariaDB, or
PostgreSQL.** The only thing that changes is the connection.

It defines a `Product` model, creates the table, inserts rows, runs a filtered query,
and does an upsert on a unique key — exercising the paths that differ per backend
(auto-increment ids, `TEXT` vs `VARCHAR`, `BOOLEAN` vs `TINYINT(1)`, JSON/JSONB, and
`ON CONFLICT` vs `ON DUPLICATE KEY UPDATE`).

## Run it

**SQLite** works out of the box (no server needed):

```sh
cd examples/multidb
qmake && make
./multidb
```

**MySQL / MariaDB** and **PostgreSQL** need a running server. The easiest way is the
Docker setup in [`tests/integration`](../../tests/integration/):

```sh
docker compose -f ../../tests/integration/docker-compose.yml up -d

QIVOT_DB=postgres ./multidb
QIVOT_DB=mysql    QIVOT_PORT=3306 ./multidb
```

(If host port 3306 is taken, start MariaDB on another port and pass `QIVOT_PORT=3307`.)

## Configuration (environment variables)

| Var | Default | Meaning |
|-----|---------|---------|
| `QIVOT_DB` | `sqlite` | `sqlite` · `mysql` / `mariadb` · `postgres` / `pg` |
| `QIVOT_HOST` | `127.0.0.1` | server host |
| `QIVOT_PORT` | 3306 / 5432 | server port |
| `QIVOT_NAME` | `qivot_test` (`multidb.db` for SQLite) | database name |
| `QIVOT_USER` | `root` / `postgres` | user |
| `QIVOT_PASS` | `qivot` | password |

## The only backend-specific code

Everything is identical across databases except building the connection:

```cpp
QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");   // or QMYSQL / QSQLITE
db.setHostName(...); db.setDatabaseName(...); db.setUserName(...); db.setPassword(...);
db.open();

QiConnection connection;
connection.open(db);            // Qivot picks the right SQL dialect from the driver
```

From here on — `createTables()`, `save()`, `QiQuery<Product>().filter(...)`, `upsert()` —
your code doesn't know or care which database it's talking to.

## Needing the Qt SQL driver plugins

To connect to MySQL/Postgres, your Qt must have the `QMYSQL` / `QPSQL` **driver plugins**
(and their client libraries reachable at runtime). If `QSqlDatabase::drivers()` doesn't list
them, install the matching Qt SQL plugin packages. See
[`tests/integration/README.md`](../../tests/integration/README.md) for the details and a
driver-free way to validate the generated SQL. Full multi-database notes live in
[`docs/Databases.md`](../../docs/Databases.md).
