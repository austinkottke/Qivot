# QDUCKDB — a Qt SQL driver for DuckDB

DuckDB is an embedded OLAP database ("SQLite for analytics") with **no Qt-bundled driver**,
so Qivot's DuckDB dialect (`src/qiduckdbstatement.*`) needs a runtime driver to actually
execute. This is a small, self-contained `QSqlDriver`/`QSqlResult` over DuckDB's C API.

It's not built into libqivot (that would force a libduckdb dependency on everyone). Instead
you compile these two files into your app and register the driver at startup:

```cpp
#include "duckdbdriver.h"
QSqlDatabase::registerSqlDriver("QDUCKDB", new QSqlDriverCreator<DuckDbDriver>());

QSqlDatabase db = QSqlDatabase::addDatabase("QDUCKDB");
db.setDatabaseName(":memory:");     // or a file path
db.open();

QiConnection conn;
conn.open(db);                       // Qivot maps driverName "QDUCKDB" -> QiDuckDbStatement
conn.addModel<MyModel>();
conn.createTables();
```

## Building

Get DuckDB's C API bundle (`duckdb.h` + `libduckdb.{dylib,so,dll}`) from the
[DuckDB releases](https://github.com/duckdb/duckdb/releases) (the `libduckdb-*` asset), then:

```pro
INCLUDEPATH += /path/to/libduckdb /path/to/qivot/drivers/duckdb
SOURCES     += /path/to/qivot/drivers/duckdb/duckdbdriver.cpp
LIBS        += -L/path/to/libduckdb -lduckdb
QMAKE_RPATHDIR += /path/to/libduckdb
```

The integration test wires this up already — see `tests/integration/`:

```sh
cd tests/integration
qmake CONFIG+=duckdb DUCKDB_DIR=/path/to/libduckdb
make && ./integration duckdb
```

## What it does / doesn't

- Implements: `open`/`close`, prepared and direct queries, positional binding (Qivot's
  `:name` placeholders are rewritten to `?`), transactions, `record()` (so Qivot's portable
  migration column-read works), and typed results incl. DATE/TIME/TIMESTAMP round-trips.
- Buffers each result fully in memory — fine for OLTP-style access; not tuned for streaming
  huge analytical result sets.
- Verified: the full Qivot integration suite passes against DuckDB **v1.5.5** (auto-id via
  `currval()`, multi-statement `CREATE SEQUENCE; CREATE TABLE`, batch ids, upserts, and
  date/time round-trips all green).
