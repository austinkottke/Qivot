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

    QString columnTypeName(int type);
    QString columnConstraint(QiClause clause, const QString &typeName, bool emitPrimaryKey = true);

    QString driverName() override;

    /// Check is a table exist
    static QString exists(QiModelMetaInfo *info);

protected:

    QString _createTableIfNotExists(QiModelMetaInfo *info) override;

public:
    QString addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field) override;

private:

};

#endif // QiSQLITESTATEMENT_H
