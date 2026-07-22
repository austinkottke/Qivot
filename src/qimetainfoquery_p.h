#ifndef QiMETAINFOQUERY_P_H
#define QiMETAINFOQUERY_P_H

#include "qisharedquery.h"

/// Query class for internal use

class _DQMetaInfoQuery : public QiSharedQuery {
public:
    inline _DQMetaInfoQuery(QiModelMetaInfo *metaInfo,QiConnection connection) : QiSharedQuery(connection) , m_metaInfo(metaInfo){
        setMetaInfo(metaInfo);
    }

    _DQMetaInfoQuery& operator=(const QiSharedQuery &rhs ) {
        QiSharedQuery::operator =(rhs);
        return *this;
    }

    bool recordTo(QiModel *model) {
        return QiSharedQuery::recordTo(model);
    }

    QiModelMetaInfo *m_metaInfo;
};

#endif // QiMETAINFOQUERY_P_H
