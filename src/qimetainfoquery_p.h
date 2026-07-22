#ifndef QiMETAINFOQUERY_P_H
#define QiMETAINFOQUERY_P_H

#include "qisharedquery.h"

/// Query class for internal use

class _QiMetaInfoQuery : public QiSharedQuery {
public:
    inline _QiMetaInfoQuery(QiModelMetaInfo *metaInfo,QiConnection connection) : QiSharedQuery(connection) , m_metaInfo(metaInfo){
        setMetaInfo(metaInfo);
    }

    _QiMetaInfoQuery& operator=(const QiSharedQuery &rhs ) {
        QiSharedQuery::operator =(rhs);
        return *this;
    }

    bool recordTo(QiModel *model) {
        return QiSharedQuery::recordTo(model);
    }

    QiModelMetaInfo *m_metaInfo;
};

#endif // QiMETAINFOQUERY_P_H
