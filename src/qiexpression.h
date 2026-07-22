#ifndef QiEXPRESSION_H
#define QiEXPRESSION_H

#include <qiwhere.h>
#include <QMap>
#include <QSharedDataPointer>

class QiExpressionPriv;

/// Construct an expression based on QiWhere clause
/**
   @remarks It is a private class for implementation use. User should not use this class.
 */
class QiExpression
{
public:
    QiExpression();
    QiExpression(const QiExpression& rhs);
    QiExpression(QiWhere where);
    QiExpression &operator=(const QiExpression &rhs);

    ~QiExpression();

    /// Get the expression in string
    QString string();

    /// A map of values to find with QSqlQuery
    QMap<QString,QVariant> bindValues();

    bool isNull();

private:

    QSharedDataPointer<QiExpressionPriv> d;
};

#endif // QiEXPRESSION_H
