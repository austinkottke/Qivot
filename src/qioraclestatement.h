#ifndef QIORACLESTATEMENT_H
#define QIORACLESTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// Oracle SQL statement generator (Qt driver "QOCI").
/**
    Oracle has no ON CONFLICT/ON DUPLICATE KEY (needs MERGE, like SQL Server, though
    Oracle's doesn't require a trailing ";"), no information_schema (needs
    user_tables), and — the one genuinely new problem versus every other dialect
    here — no session-scoped "id of the row I just inserted": Postgres's lastval()
    and SQL Server's SCOPE_IDENTITY() both work because *any* insert in the session
    counts, but Oracle only offers that via a sequence's CURRVAL, and CURRVAL needs
    to know *which* sequence. So every auto-increment table gets its own
    deterministically-named companion sequence ("<table>_id_seq"), created
    alongside the table (see createTableIfNotExists()), and lastInsertIdQuery(info)
    — the table-aware overload — asks that specific sequence for its CURRVAL.

    Full-text search (FTS) is SQLite-only for now.

    @remarks Stateless / thread-safe, like every QiSqlStatement.
 */
class QiOracleStatement : public QiSqlStatement
{
public:
    QiOracleStatement();

    QString driverName() override;

    QString columnTypeName(int type) override;
    QString columnTypeForField(int type, QiClause clause) override;
    QString primaryKeyClause(const QString &typeName) override;

    QString createTableIfNotExists(QiModelMetaInfo *info) override;
    QString exists(QiModelMetaInfo *info) override;

    QString upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns) override;
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

#endif // QIORACLESTATEMENT_H
