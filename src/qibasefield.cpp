#include "qibasefield.h"
#include <QSharedData>
#include <QString>
#include <QtCore>

QiBaseField::QiBaseField()
{
}

QiBaseField::~QiBaseField()
{
}

bool QiBaseField::set(QVariant val){
    m_value = val;
    return true;
}

QVariant QiBaseField::get(bool convert) const {
    Q_UNUSED(convert);
    return m_value;
}

QiClause QiBaseField::clause(){
    return QiClause();
}

QVariant QiBaseField::operator=(const QVariant &val){
    m_value = val;
    return val;
}

QVariant* QiBaseField::operator->(){
    return &m_value;
}

QVariant QiBaseField::operator() () const {
    return m_value;
}

 QiBaseField::operator QVariant(){
    return m_value;
}

 void QiBaseField::clear(){
    m_value.clear();
 }

 QDebug operator<<(QDebug dbg, const QiBaseField &field){
     dbg.nospace() << field.get();

     return dbg.space();
 }
