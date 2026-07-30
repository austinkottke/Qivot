#ifndef QIPGSTATEMENT_H
#define QIPGSTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// PostgreSQL SQL statement generator (Qt driver "QPSQL").
/**
    Postgres is close to the portable base: its upsert is the same "INSERT ...
    ON CONFLICT DO UPDATE" as SQLite, so that is inherited. What differs: real
    column types, identity primary keys, and the new-row id — Postgres reports it
    via "RETURNING id" rather than lastInsertId(), so insertInto appends RETURNING
    and returnsIdOnInsert() tells QiSql to read it back. FTS is SQLite-only for now.

    @remarks Stateless / thread-safe.
 */
class QiPgStatement : public QiSqlStatement
{
public:
    QiPgStatement();

    QString driverName() override;

    QString columnTypeName(int type) override;
    QString primaryKeyClause(const QString &typeName) override;

    /// Postgres has no REPLACE INTO — emulate it as an upsert on the primary key.
    QString replaceInto(QiModelMetaInfo *info, QStringList fields) override;

    QString lastInsertIdQuery() const override;

    QStringList createFtsIndex(const QiBaseFtsIndex &index) override;
    QStringList dropFtsIndex(QString name) override;
};

#endif // QIPGSTATEMENT_H
