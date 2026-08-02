#ifndef QIDUCKDBSTATEMENT_H
#define QIDUCKDBSTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// DuckDB SQL statement generator (embedded OLAP database; Qt driver "QDUCKDB").
/**
    DuckDB is embedded like SQLite but speaks a Postgres-flavoured SQL, so most of this
    dialect is inherited from the base QiSqlStatement — the `ON CONFLICT ... DO UPDATE`
    upsert and the `information_schema` table check come for free (exactly as Postgres
    uses them). What's DuckDB-specific:

      - No AUTO_INCREMENT/SERIAL/IDENTITY keyword: an auto id column DEFAULTs to
        `nextval()` of a per-table companion sequence ("<table>_id_seq"), created next
        to the table (see createTableIfNotExists()) — the same shape as the Oracle
        dialect, but with `CREATE SEQUENCE IF NOT EXISTS` instead of a PL/SQL block.
      - The new id is read back with `currval('<table>_id_seq')`, which is connection-
        scoped once nextval has run in the session (like Oracle's CURRVAL / Postgres's
        lastval()), so the table-aware lastInsertIdQuery(info) names that sequence.
      - No `REPLACE INTO`: translated to an upsert on the primary key (like Postgres).

    The companion `CREATE SEQUENCE` is emitted in the same ";"-separated statement string
    as the `CREATE TABLE`; DuckDB executes multiple statements per query natively, which
    the QDUCKDB driver relies on.

    Full-text search (FTS) is SQLite-only for now.

    @remarks Stateless / thread-safe, like every QiSqlStatement.
 */
class QiDuckDbStatement : public QiSqlStatement
{
public:
    QiDuckDbStatement();

    QString driverName() override;

    QString columnTypeName(int type) override;
    QString primaryKeyClause(const QString &typeName) override;

    QString createTableIfNotExists(QiModelMetaInfo *info) override;

    QString replaceInto(QiModelMetaInfo *info, QStringList fields) override;

    // Un-hides QiSqlStatement::lastInsertIdQuery() — see the identical comment in
    // qimssqlstatement.h for why this is needed.
    using QiSqlStatement::lastInsertIdQuery;
    QString lastInsertIdQuery(QiModelMetaInfo *info) const override;

    QStringList createFtsIndex(const QiBaseFtsIndex &index) override;
    QStringList dropFtsIndex(QString name) override;

private:
    static QString sequenceName(QiModelMetaInfo *info);
};

#endif // QIDUCKDBSTATEMENT_H
