QT       += core sql
QT       -= gui

TARGET   = integration
CONFIG   += c++17 console
CONFIG   -= app_bundle

TEMPLATE = app

HEADERS += intmodel.h
SOURCES += main.cpp

# Optional DuckDB backend. DuckDB ships no Qt driver, so link the bundled QDUCKDB driver
# (drivers/duckdb) against libduckdb's C API:
#   qmake CONFIG+=duckdb DUCKDB_DIR=/path/to/libduckdb    # dir with duckdb.h + libduckdb.*
#   make && ./integration duckdb
duckdb {
    isEmpty(DUCKDB_DIR): error("CONFIG+=duckdb needs DUCKDB_DIR=<dir with duckdb.h + libduckdb.*>")
    DEFINES     += QIVOT_DUCKDB
    INCLUDEPATH += $$DUCKDB_DIR $$PWD/../../drivers/duckdb
    HEADERS     += $$PWD/../../drivers/duckdb/duckdbdriver.h
    SOURCES     += $$PWD/../../drivers/duckdb/duckdbdriver.cpp
    LIBS        += -L$$DUCKDB_DIR -lduckdb
    QMAKE_RPATHDIR += $$DUCKDB_DIR
}

include(../../src/qivot.pri)
