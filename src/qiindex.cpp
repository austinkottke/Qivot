#include "qiindex.h"

QiBaseIndex::QiBaseIndex(QiModelMetaInfo* metaInfo, QString name) :
    m_metaInfo(metaInfo),
    m_name(name),
    m_unique(false)
{
}

const QiModelMetaInfo* QiBaseIndex::metaInfo() const{
    return m_metaInfo;
}

QString QiBaseIndex::name() const {
    return m_name;
}

QStringList QiBaseIndex::columnDefList() const {
    return m_columnDefList;
}

void QiBaseIndex::setColumnDefList(QStringList fields) {
    m_columnDefList = fields;
}

QiBaseIndex& QiBaseIndex::operator<<(QString columnDef){
    m_columnDefList << columnDef;
    return *this;
}

QiBaseIndex& QiBaseIndex::setUnique(bool unique){
    m_unique = unique;
    return *this;
}

bool QiBaseIndex::isUnique() const {
    return m_unique;
}

QiBaseIndex& QiBaseIndex::setWhere(QString condition){
    m_where = condition;
    return *this;
}

QString QiBaseIndex::where() const {
    return m_where;
}
