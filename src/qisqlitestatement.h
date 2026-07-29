#ifndef QiSQLITESTATEMENT_H
#define QiSQLITESTATEMENT_H

#include <QVariant>
#include <qisqlstatement.h>

/// Sqlite SQL Statement generator

/**
    @remarks It is thread-safe
    @remarks All the derived class should not hold any member attribute.
 */
class QiSqliteStatement : public QiSqlStatement
{
public:
    QiSqliteStatement();

    QString columnTypeName(int type) override;
    QString columnConstraint(QiClause clause, const QString &typeName, bool emitPrimaryKey = true) override;

    QString driverName() override;

    /// Check is a table exist
    QString exists(QiModelMetaInfo *info) override;

protected:

    QString _createTableIfNotExists(QiModelMetaInfo *info) override;

public:
    QString addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field) override;

private:

};

#endif // QiSQLITESTATEMENT_H
