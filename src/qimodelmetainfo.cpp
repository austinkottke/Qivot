#include <QMap>
#include <QCoreApplication>
#include "qimodelmetainfo.h"
#include "qimodel.h"

#define MEMBER_PTR(model, offset)   \
    ((void*) ((quint8*) (model) + (qint64) (offset)))
#define QI_MODEL_GET_FIELD(model, offset) \
            ( (QiBaseField*) MEMBER_PTR(model,offset) )

static QMap<QString , QiModelMetaInfo* > metaTypeList;

QiModelMetaInfo* qiFindMetaInfo(QString name) {
    QiModelMetaInfo * res = 0;
    if (metaTypeList.contains(name))
        res = metaTypeList[name];
    return res;
}

void qiRegisterMetaInfo(QString name, QiModelMetaInfo *metaType){
    metaTypeList[name] = metaType;
}

QiModelMetaInfo::QiModelMetaInfo() : QObject() {
    QCoreApplication *app = QCoreApplication::instance();
    setParent(app); // Then it will be destroyed in program termination. Make valgrind happy.
}

void QiModelMetaInfo::registerField(QiModelMetaInfoField field){
    // The final registerField() call

    if (field.clause.testFlag(QiClause::FOREIGN_KEY)) {
        m_foreignKeyList << field;
    }

    m_fields[field.name] = field;
    m_fieldList << field;
}

void QiModelMetaInfo::registerFields(QList<QiModelMetaInfoField> fields){
    foreach (QiModelMetaInfoField field,fields) {
        registerField(field);
    }
}

QStringList QiModelMetaInfo::fieldNameList(){
    QStringList result;
    QMapIterator<QString, QiModelMetaInfoField> iter(m_fields);
    while (iter.hasNext()) {
         iter.next();
         result << iter.key();
    }
    return result;
}

QStringList QiModelMetaInfo::primaryKeyFields() const {
    QStringList res;
    foreach (QiModelMetaInfoField field, m_fieldList) {
        QiClause clause = field.clause;   // copy: testFlag() is non-const
        if (clause.testFlag(QiClause::PRIMARY_KEY))
            res << field.name;
    }
    return res;
}

bool QiModelMetaInfo::isWithoutRowid() const {
    return m_withoutRowid;
}

void QiModelMetaInfo::setWithoutRowid(bool on) {
    m_withoutRowid = on;
}

QString QiModelMetaInfo::primaryKeyName() const {
    QStringList pk = primaryKeyFields();
    if (pk.size() == 1)
        return pk.first();
    return QString();   // none, or composite (no single reference column)
}

QList<QiModelMetaInfoField> QiModelMetaInfo::foreignKeyList(){
    return m_foreignKeyList;
}

QStringList QiModelMetaInfo::foreignKeyNameList(){
    QStringList result;
    foreach (QiModelMetaInfoField field , m_foreignKeyList){
        result << field.name;
    }

    return result;

}

int QiModelMetaInfo::size() const{
    return m_fields.size();
}

const QiModelMetaInfoField* QiModelMetaInfo::at(int idx) const{
    return &m_fieldList.at(idx);
}

bool QiModelMetaInfo::setValue(QiAbstractModel *model,QString field, const QVariant& val){
    if (!m_fields.contains(field))
        return false;
    int offset = m_fields[field].offset;

    QiBaseField* f = QI_MODEL_GET_FIELD(model,offset);
    f->set(val);
    return true;
}

bool QiModelMetaInfo::setValue(QiAbstractModel *model,int index, const QVariant& val){
    if (index< 0 || index > size() ) {
        return false;
    }

    int offset = m_fieldList[index].offset;

    QiBaseField* f = QI_MODEL_GET_FIELD(model,offset);
    f->set(val);
    return true;
}

QVariant QiModelMetaInfo::value(const QiAbstractModel *model,QString field,bool convert) const{
    QVariant v;
    if (!m_fields.contains(field))
        return v;
    int offset = m_fields[field].offset;

    QiBaseField* f = QI_MODEL_GET_FIELD(model,offset);

    return f->get(convert);
}

QVariant QiModelMetaInfo::value(const QiAbstractModel *model,int index ,bool convert) const{
    if (index< 0 || index > size() ) {
        return QVariant();
    }

    int offset = m_fieldList[index].offset;

    QiBaseField* f = QI_MODEL_GET_FIELD(model,offset);
    return f->get(convert);
}

QString QiModelMetaInfo::name() const{
    return m_name;
}

void QiModelMetaInfo::setName(QString val){
    m_name = val;
}

void QiModelMetaInfo::setClassName(QString val){
    m_className = val;
}

QString QiModelMetaInfo::className() const {
    return m_className;
}

QiAbstractModel* QiModelMetaInfo::create(){
    return createFunc();
}

QiSharedList QiModelMetaInfo::initialData(){
    return initialDataFunc();
}
