#ifndef QIMYSQLSTATEMENT_H
#define QIMYSQLSTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// MySQL / MariaDB SQL statement generator (Qt drivers "QMYSQL" / "QMARIADB").
/**
    Reuses the portable base generator and overrides only what differs from
    ANSI SQL: real column types, AUTO_INCREMENT primary keys, InnoDB/utf8mb4
    table options, and MySQL's "ON DUPLICATE KEY UPDATE" upsert. Full-text
    search (FTS) is SQLite-only for now.

    @remarks Stateless / thread-safe, like every QiSqlStatement.
 */
class QiMysqlStatement : public QiSqlStatement
{
public:
    QiMysqlStatement();

    QString driverName() override;

    QString columnTypeName(int type) override;
    QString columnTypeForField(int type, QiClause clause) override;
    QString primaryKeyClause(const QString &typeName) override;
    QString tableSuffix(QiModelMetaInfo *info) override;

    QString upsertInto(QiModelMetaInfo *info, QStringList fields, QStringList conflictColumns) override;

    QStringList createFtsIndex(const QiBaseFtsIndex &index) override;
    QStringList dropFtsIndex(QString name) override;
};

#endif // QIMYSQLSTATEMENT_H
