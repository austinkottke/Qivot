/**
	@author Ben Lau <xbenlau@gmail.com>
 */

#include <QtCore>
#include "qistream.h"

QiStream::QiStream()
{
    m_model = 0;
    m_current = 0;
}

QiStream::QiStream(QiAbstractModel* model) {
    m_model = 0;
    m_current = 0;

    open(model);
}

void QiStream::open(QiAbstractModel* model){
    m_model = model;
    m_current = 0;

    if (m_model) {
        QiModelMetaInfo *metaInfo = m_model->metaInfo();
        if (metaInfo->size() == 1) {
            qWarning() << "QiStream::setModel() do not support model with single field of id.";
            m_model = 0;
        }
    }
}

QiAbstractModel* QiStream::model(){
    return m_model;
}

int QiStream::currentField() {
    return m_current;
}

void QiStream::write(const QVariant value) {
    Q_ASSERT(m_model);

    QiModelMetaInfo *metaInfo = m_model->metaInfo();

    if (m_current == 0) // ignore id field
        m_current++;

    metaInfo->setValue(m_model,m_current++,value);

    if (m_current >= metaInfo->size() ) {
        m_current = 0 ; // reset the count to zero.
    }
}

void QiStream::read(QVariant &target) {
    Q_ASSERT(m_model);
    Q_ASSERT(m_model);

    QiModelMetaInfo *metaInfo = m_model->metaInfo();

    if (m_current == 0) // ignore id field
        m_current++;

    target = metaInfo->value(m_model,m_current++);

    if (m_current >= metaInfo->size() ) {
        m_current = 0 ; // reset the count to zero.
    }
}

void QiStream::close() {
    m_current = 0;
    m_model = 0;
}

QiStream& QiStream::operator<< (const QVariant value){
    write(value);
    return (*this);
}

QiStream& QiStream::operator>> (QVariant &target){
    read(target);
    return (*this);
}

QiStream& QiStream::operator>> (QString& target){
    QVariant v;
    read(v);
    target = v.toString();
    return (*this);
}

QiStream& QiStream::operator>> (int& target) {
    QVariant v;
    read(v);
    target = v.toInt();
    return (*this);
}

QiStream& QiStream::operator>> (double& target) {
    QVariant v;
    read(v);
    target = v.toDouble();
    return (*this);
}

QiStream& QiStream::operator>> (QDateTime& target) {
    QVariant v;
    read(v);
    target = v.toDateTime();
    return (*this);
}
