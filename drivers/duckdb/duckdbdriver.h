#ifndef DUCKDBDRIVER_H
#define DUCKDBDRIVER_H

#include <QSqlDriver>
#include <QSqlResult>
#include <QSqlField>
#include <QSqlRecord>
#include <QSqlError>
#include "duckdb.h"

// A minimal Qt SQL driver over DuckDB's C API. Embedded (no server), like SQLite.
// Registered at runtime under "QDUCKDB"; Qivot's forDriver() maps that to QiDuckDbStatement.
class DuckDbDriver : public QSqlDriver {
    Q_OBJECT
public:
    explicit DuckDbDriver(QObject *parent = nullptr) : QSqlDriver(parent) {}
    ~DuckDbDriver() override { close(); }

    bool hasFeature(DriverFeature f) const override {
        switch (f) {
        case Transactions: case PreparedQueries: case PositionalPlaceholders:
        case QuerySize: case BLOB: case Unicode: case LastInsertId: return true;
        default: return false;
        }
    }

    bool open(const QString &db, const QString &, const QString &,
              const QString &, int, const QString &) override {
        close();
        const QByteArray path = db.isEmpty() ? QByteArray(":memory:") : db.toUtf8();
        if (duckdb_open(path.constData(), &m_db) != DuckDBSuccess) {
            setLastError(QSqlError("duckdb_open failed", QString(), QSqlError::ConnectionError));
            return false;
        }
        if (duckdb_connect(m_db, &m_con) != DuckDBSuccess) {
            duckdb_close(&m_db); m_db = nullptr;
            setLastError(QSqlError("duckdb_connect failed", QString(), QSqlError::ConnectionError));
            return false;
        }
        setOpen(true); setOpenError(false);
        return true;
    }
    void close() override {
        if (m_con) { duckdb_disconnect(&m_con); m_con = nullptr; }
        if (m_db)  { duckdb_close(&m_db); m_db = nullptr; }
        setOpen(false);
    }

    QSqlResult *createResult() const override;

    bool beginTransaction() override    { return simpleExec("BEGIN TRANSACTION"); }
    bool commitTransaction() override   { return simpleExec("COMMIT"); }
    bool rollbackTransaction() override { return simpleExec("ROLLBACK"); }

    QStringList tables(QSql::TableType) const override {
        QStringList out;
        duckdb_result r;
        if (duckdb_query(m_con, "SELECT table_name FROM information_schema.tables WHERE table_schema='main'", &r) == DuckDBSuccess) {
            for (idx_t i = 0; i < duckdb_row_count(&r); ++i) {
                char *v = duckdb_value_varchar(&r, 0, i);
                if (v) { out << QString::fromUtf8(v); duckdb_free(v); }
            }
        }
        duckdb_destroy_result(&r);
        return out;
    }

    // Qivot's portable columnNames() calls QSqlDatabase::record() — so migrations depend on this.
    QSqlRecord record(const QString &tableName) const override;

    QVariant handle() const override { return QVariant(); }

    duckdb_connection conn() const { return m_con; }

private:
    bool simpleExec(const char *sql) {
        duckdb_result r;
        const bool ok = duckdb_query(m_con, sql, &r) == DuckDBSuccess;
        duckdb_destroy_result(&r);
        return ok;
    }
    duckdb_database   m_db = nullptr;
    duckdb_connection m_con = nullptr;
};

#endif // DUCKDBDRIVER_H
