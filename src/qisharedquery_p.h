#ifndef QiABSTRACTQUERY_P_H
#define QiABSTRACTQUERY_P_H

#include <QSqlQuery>
#include "qiconnection.h"
#include "qimodel.h"
#include "qimodelmetainfo.h"
#include "qiwhere.h"
#include "qiexpression.h"
#include "qijoin.h"

/// QiSharedQuery private data

class QiSharedQueryPriv : public QSharedData {
public:
    inline QiSharedQueryPriv() {
        metaInfo = 0;
        limit = -1; // No limit
        offset = -1; // No offset
        distinct = false;
    }

    QiConnection connection;

    /// The function to be called on result column.
    QString func;

    QiModelMetaInfo *metaInfo;
    int limit;
    int offset;

    /// TRUE if the query should emit SELECT DISTINCT
    bool distinct;

    QSqlQuery query;

    QiExpression expression;

    /// select(fields)
    QStringList fields;

    QStringList orderBy;

    /// GROUP BY terms
    QStringList groupBy;

    /// HAVING clause
    QiExpression having;

    /// The JOIN clauses of the query, in the order they were added
    QList<QiBaseJoin> joins;
};

#endif // QiABSTRACTQUERY_P_H
