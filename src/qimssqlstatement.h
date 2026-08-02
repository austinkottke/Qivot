#ifndef QIMSSQLSTATEMENT_H
#define QIMSSQLSTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// Microsoft SQL Server SQL statement generator (Qt driver "QODBC").
/**
    SQL Server diverges from the portable base more than MySQL/Postgres do: no
    LIMIT/OFFSET (needs OFFSET...FETCH, which requires an ORDER BY), no
    ON CONFLICT/ON DUPLICATE KEY upsert (needs MERGE, which — unlike every other
    statement shape this library emits — is a syntax error without a trailing
    ";"), and no ANSI RENAME COLUMN (needs sp_rename). Full-text search (FTS) is
    SQLite-only for now.

    @remarks Stateless / thread-safe, like every QiSqlStatement.
 */
class QiMsSqlStatement : public QiSqlStatement
{
public:
    QiMsSqlStatement();

    QString driverName() override;

    QString columnTypeName(int type) override;
    QString columnTypeForField(int type, QiClause clause) override;
    QString primaryKeyClause(const QString &typeName) override;

    QString createTableIfNotExists(QiModelMetaInfo *info) override;

    QString addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field) override;
    QString renameColumn(QiModelMetaInfo *info, const QString &from, const QString &to) override;

    QString upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns) override;
    QString replaceInto(QiModelMetaInfo *info, QStringList fields) override;

    // Un-hides QiSqlStatement::lastInsertIdQuery(QiModelMetaInfo*) — declaring the
    // no-arg overload below in this class's own scope would otherwise hide every base
    // overload of the same name from any caller holding a concrete QiMsSqlStatement
    // (as opposed to a QiSqlStatement base pointer, where virtual dispatch already
    // finds it regardless).
    using QiSqlStatement::lastInsertIdQuery;
    QString lastInsertIdQuery() const override;
    bool keepsStatementTerminator() const override { return true; }

    QString select(QiSharedQuery query) override;

    QStringList createFtsIndex(const QiBaseFtsIndex &index) override;
    QStringList dropFtsIndex(QString name) override;
};

#endif // QIMSSQLSTATEMENT_H
