/**
    @author Ben Lau
 */

#include "qilistwriter.h"

QiListWriter::QiListWriter()
{
    m_list = 0;
    m_connection = QiConnection::defaultConnection();
}

QiListWriter::QiListWriter(QiSharedList *list){
    open(list);
    m_connection = QiConnection::defaultConnection();
}

QiListWriter::QiListWriter(QiSharedList *list,QiConnection connection){
    open(list);
    setConnection(connection);
}

bool QiListWriter::open(QiSharedList *list){
    bool res = false;
    if (list->metaInfo()) {
        res = true;
        m_list = list;
        m_stream.close();
    }
    return res;
}

QiSharedList *QiListWriter::list(){
    return m_list;
}

void QiListWriter::append(QVariant v) {
    if (v.userType() == qMetaTypeId<Next>()) {
        m_stream.close();
        return;
    }

    Q_ASSERT(m_list);
    QiModelMetaInfo* metaInfo = m_list->metaInfo();

    if (m_stream.currentField() == 0 ) { // it should create a new model
        QiAbstractModel* model = metaInfo->create();
        QiModel *m = static_cast<QiModel*>(model);
        m->setConnection(m_connection);
        m_list->append(model);

        m_stream.close();
        m_stream.open(model);
    }

    m_stream.write(v);
}

void QiListWriter::close(){
    m_stream.close();
    m_list = 0;
}

QVariant QiListWriter::next(){
    QVariant v;
    v.setValue<QiListWriter::Next> (Next());
    return v;
}

void QiListWriter::setConnection(QiConnection val){
    m_connection = val;
}

QiConnection QiListWriter::connection(){
    return m_connection;
}

QiListWriter& QiListWriter::operator<< (const QVariant value){
    append(value);
    return *this;
}

