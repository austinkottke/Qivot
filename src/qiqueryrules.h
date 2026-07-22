#ifndef QiQUERYRULES_H
#define QiQUERYRULES_H

#include <QSharedDataPointer>
#include <qisharedquery.h>
#include <qiexpression.h>
#include <qijoin.h>

/// QiQueryRules represent the rules/clauses set for QiSharedQuery.

/** QiSharedQuery/QiQuery do not provide any interface to get the rules / clause set for query.
  Instead it should use QiQueryRules to retrieve the information. Normally user do not need to
  retreive the rules set for a query, it is useful for implement custom select sql or unit tests
 */

class QiQueryRules
{
public:
    QiQueryRules();
    QiQueryRules(const QiQueryRules &);
    QiQueryRules &operator=(const QiQueryRules &);
    QiQueryRules &operator=(const QiSharedQuery &);
    ~QiQueryRules();

    /// Get the limit of query
    int limit();

    /// Get the offset of query
    int offset();

    QiExpression expression();

    /// Get the GROUP BY terms
    QStringList groupBy();

    /// Get the HAVING expression
    QiExpression having();

    /// Get the func that should be applied on result column
    QString func();

    /// Get the QiModelMetaInfo instance of the query model
    QiModelMetaInfo *metaInfo();

    /// Get the field for result column
    QStringList fields();

    /// Get the field for orderBy
    QStringList orderBy();

    /// Get the JOIN clauses of the query
    QList<QiBaseJoin> joins();

    /// TRUE if the query should emit SELECT DISTINCT
    bool distinct();

private:
    QSharedDataPointer<QiSharedQueryPriv> data;
};

#endif // QiQUERYRULES_H
