#include "duckdbdriver.h"
#include <QRegularExpression>
#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QVariant>

// ---- type mapping ----------------------------------------------------------

static QVariant::Type metaFor(duckdb_type t) {
    switch (t) {
    case DUCKDB_TYPE_BOOLEAN: return QVariant::Bool;
    case DUCKDB_TYPE_TINYINT: case DUCKDB_TYPE_SMALLINT:
    case DUCKDB_TYPE_INTEGER: return QVariant::Int;
    case DUCKDB_TYPE_BIGINT:  case DUCKDB_TYPE_HUGEINT: return QVariant::LongLong;
    case DUCKDB_TYPE_FLOAT: case DUCKDB_TYPE_DOUBLE: case DUCKDB_TYPE_DECIMAL: return QVariant::Double;
    case DUCKDB_TYPE_DATE: return QVariant::Date;
    case DUCKDB_TYPE_TIME: return QVariant::Time;
    case DUCKDB_TYPE_TIMESTAMP: case DUCKDB_TYPE_TIMESTAMP_S:
    case DUCKDB_TYPE_TIMESTAMP_MS: case DUCKDB_TYPE_TIMESTAMP_NS: return QVariant::DateTime;
    case DUCKDB_TYPE_BLOB: return QVariant::ByteArray;
    default: return QVariant::String;
    }
}

static QVariant cellValue(duckdb_result *res, idx_t col, idx_t row, duckdb_type t) {
    if (duckdb_value_is_null(res, col, row))
        return QVariant(metaFor(t));   // typed NULL
    switch (t) {
    case DUCKDB_TYPE_BOOLEAN: return duckdb_value_boolean(res, col, row);
    case DUCKDB_TYPE_TINYINT: case DUCKDB_TYPE_SMALLINT: case DUCKDB_TYPE_INTEGER:
    case DUCKDB_TYPE_BIGINT:  case DUCKDB_TYPE_HUGEINT:
        return qlonglong(duckdb_value_int64(res, col, row));
    case DUCKDB_TYPE_FLOAT: case DUCKDB_TYPE_DOUBLE: case DUCKDB_TYPE_DECIMAL:
        return duckdb_value_double(res, col, row);
    default: break;
    }
    // everything else via its text form, then parse dates/times back to typed QVariants
    char *c = duckdb_value_varchar(res, col, row);
    const QString s = c ? QString::fromUtf8(c) : QString();
    if (c) duckdb_free(c);
    switch (t) {
    case DUCKDB_TYPE_DATE: return QDate::fromString(s, "yyyy-MM-dd");
    case DUCKDB_TYPE_TIME: return QTime::fromString(s.left(8), "HH:mm:ss");
    case DUCKDB_TYPE_TIMESTAMP: case DUCKDB_TYPE_TIMESTAMP_S:
    case DUCKDB_TYPE_TIMESTAMP_MS: case DUCKDB_TYPE_TIMESTAMP_NS: {
        QDateTime dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss");
        if (!dt.isValid()) dt = QDateTime::fromString(s.left(19), "yyyy-MM-dd HH:mm:ss");
        return dt;
    }
    default: return s;
    }
}

static void bindParam(duckdb_prepared_statement stmt, idx_t idx, const QVariant &v) {
    if (v.isNull() || !v.isValid()) { duckdb_bind_null(stmt, idx); return; }
    switch (v.type()) {
    case QVariant::Bool:     duckdb_bind_boolean(stmt, idx, v.toBool()); return;
    case QVariant::Int: case QVariant::UInt:
    case QVariant::LongLong: case QVariant::ULongLong:
        duckdb_bind_int64(stmt, idx, v.toLongLong()); return;
    case QVariant::Double:   duckdb_bind_double(stmt, idx, v.toDouble()); return;
    case QVariant::Date: {
        const QByteArray s = v.toDate().toString("yyyy-MM-dd").toUtf8();
        duckdb_bind_varchar(stmt, idx, s.constData()); return;
    }
    case QVariant::Time: {
        const QByteArray s = v.toTime().toString("HH:mm:ss").toUtf8();
        duckdb_bind_varchar(stmt, idx, s.constData()); return;
    }
    case QVariant::DateTime: {
        const QByteArray s = v.toDateTime().toString("yyyy-MM-dd HH:mm:ss").toUtf8();
        duckdb_bind_varchar(stmt, idx, s.constData()); return;
    }
    case QVariant::ByteArray: {
        const QByteArray b = v.toByteArray();
        duckdb_bind_blob(stmt, idx, b.constData(), b.size()); return;
    }
    default: {
        const QByteArray s = v.toString().toUtf8();
        duckdb_bind_varchar(stmt, idx, s.constData()); return;
    }
    }
}

// ---- result ----------------------------------------------------------------

class DuckDbResult : public QSqlResult {
public:
    explicit DuckDbResult(const DuckDbDriver *drv) : QSqlResult(drv), m_drv(drv) {}

protected:
    QVariant data(int i) override {
        if (m_at < 0 || m_at >= m_rows.size() || i < 0 || i >= m_rows[m_at].size()) return QVariant();
        return m_rows[m_at][i];
    }
    bool isNull(int i) override { const QVariant v = data(i); return v.isNull(); }

    bool reset(const QString &query) override {          // non-prepared exec (DDL, etc.)
        clear();
        duckdb_result res;
        if (duckdb_query(m_drv->conn(), query.toUtf8().constData(), &res) != DuckDBSuccess) {
            setLastError(QSqlError("query failed", QString::fromUtf8(duckdb_result_error(&res)), QSqlError::StatementError));
            duckdb_destroy_result(&res); return false;
        }
        store(res); duckdb_destroy_result(&res);
        setActive(true);
        return true;
    }

    bool prepare(const QString &query) override { m_query = query; return true; }

    bool exec() override {
        clear();
        // Qivot binds by name (:field); DuckDB wants positional '?'. boundValues() is in
        // placeholder-appearance order, so replacing each :name with ? left-to-right lines up.
        static const QRegularExpression re(":[A-Za-z_][A-Za-z0-9_]*");
        QString sql = m_query;
        sql.replace(re, "?");

        duckdb_prepared_statement stmt;
        if (duckdb_prepare(m_drv->conn(), sql.toUtf8().constData(), &stmt) != DuckDBSuccess) {
            setLastError(QSqlError("prepare failed", QString::fromUtf8(duckdb_prepare_error(stmt)), QSqlError::StatementError));
            duckdb_destroy_prepare(&stmt); return false;
        }
        const QVector<QVariant> vals = boundValues();
        for (int i = 0; i < vals.size(); ++i)
            bindParam(stmt, idx_t(i + 1), vals[i]);

        duckdb_result res;
        const bool ok = duckdb_execute_prepared(stmt, &res) == DuckDBSuccess;
        duckdb_destroy_prepare(&stmt);
        if (!ok) {
            setLastError(QSqlError("execute failed", QString::fromUtf8(duckdb_result_error(&res)), QSqlError::StatementError));
            duckdb_destroy_result(&res); return false;
        }
        store(res); duckdb_destroy_result(&res);
        setActive(true);
        return true;
    }

    bool fetch(int i) override { if (i < 0 || i >= m_rows.size()) return false; m_at = i; setAt(i); return true; }
    bool fetchFirst() override { return fetch(0); }
    bool fetchLast() override  { return fetch(m_rows.size() - 1); }
    bool fetchNext() override  { return fetch(m_at + 1); }
    int  size() override { return m_rows.size(); }
    int  numRowsAffected() override { return -1; }
    QSqlRecord record() const override { return m_record; }
    QVariant lastInsertId() const override { return QVariant(); }   // Qivot uses currval() for DuckDB

private:
    void clear() { m_rows.clear(); m_record.clear(); m_at = -1; setAt(QSql::BeforeFirstRow); setActive(false); }
    void store(duckdb_result &res) {
        const idx_t cols = duckdb_column_count(&res), rows = duckdb_row_count(&res);
        QVector<duckdb_type> types(cols);
        for (idx_t c = 0; c < cols; ++c) {
            types[c] = duckdb_column_type(&res, c);
            m_record.append(QSqlField(QString::fromUtf8(duckdb_column_name(&res, c)), metaFor(types[c])));
        }
        for (idx_t r = 0; r < rows; ++r) {
            QVariantList row; row.reserve(cols);
            for (idx_t c = 0; c < cols; ++c) row.append(cellValue(&res, c, r, types[c]));
            m_rows.append(row);
        }
        setSelect(cols > 0);
    }

    const DuckDbDriver  *m_drv;
    QString              m_query;
    QVector<QVariantList> m_rows;
    QSqlRecord           m_record;
    int                  m_at = -1;
};

// ---- driver methods needing the result type --------------------------------

QSqlResult *DuckDbDriver::createResult() const { return new DuckDbResult(this); }

QSqlRecord DuckDbDriver::record(const QString &tableName) const {
    QSqlRecord rec;
    duckdb_result res;
    const QByteArray q = ("SELECT * FROM \"" + tableName + "\" LIMIT 0").toUtf8();
    if (duckdb_query(m_con, q.constData(), &res) == DuckDBSuccess) {
        const idx_t cols = duckdb_column_count(&res);
        for (idx_t c = 0; c < cols; ++c)
            rec.append(QSqlField(QString::fromUtf8(duckdb_column_name(&res, c)), metaFor(duckdb_column_type(&res, c))));
    }
    duckdb_destroy_result(&res);
    return rec;
}
