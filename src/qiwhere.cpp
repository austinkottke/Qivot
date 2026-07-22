#include <QtCore>
#include "qiwhere.h"
#include "qiwhere_p.h"

/* Test cases:

  coretests::where()

  sqlitetests::querySelectWhere()

 */

static int qiWhereTypeId = qMetaTypeId<QiWhere>();

static QString variantToString(QVariant v,bool quoteString);

QiWhereDataPriv::QiWhereDataPriv(){
    m_type = None;
}

QiWhereDataPriv::QiWhereDataPriv(Type type) : m_type(type){
}

QiWhereDataPriv& QiWhereDataPriv::operator<<(QVariant v){
    m_list << v;
    return *this;
}

QList<QVariant> QiWhereDataPriv::list(){
    return m_list;
}

void QiWhereDataPriv::setList(QList<QVariant> list){
    m_list = list;
}

QiWhereDataPriv::Type QiWhereDataPriv::type(){
    return m_type;
}

/// A private datastructure to represent the database field in QiWhere
class QiWhereFieldPriv : public QString {
};

Q_DECLARE_METATYPE(QiWhereFieldPriv)

static int fieldTypeId = qMetaTypeId<QiWhereFieldPriv>();

QiWhere::QiWhere()
{
    m_isNull = true;
}

QiWhere::QiWhere(QString field,QString op, QVariant right)
    : m_op(op),m_right(right){

    m_left = QiWhere(field);

    m_isNull = false;
}

QiWhere::QiWhere(const QiWhere &other){
    m_left = other.m_left;
    m_right = other.m_right;
    m_op = other.m_op;
    m_isNull = other.m_isNull;
}

QiWhere::QiWhere(QString fieldAndOp , QVariant right)  : m_right(right){
    QRegularExpression rx("^\\s*[a-zA-Z0-9]+");
    QRegularExpressionMatch match = rx.match(fieldAndOp);

    if (!match.hasMatch()){
        qWarning() << QString("QiWhere() : can not parse %1").arg(fieldAndOp);
        return;
    }
    int pos = match.capturedStart();
    int len = match.capturedLength();

    QString str = fieldAndOp.mid(pos,len);
//    m_left = str.trimmed();
    m_left = QiWhere(str.trimmed());

    str = fieldAndOp.mid(pos + len);

    m_op = str.trimmed();

    m_isNull = false;
}

QiWhere::QiWhere(QString field){
    QiWhereFieldPriv f;
    f.append(field);
    m_left.setValue(f);
    m_isNull = false;
}


QVariant QiWhere::left(){
    return m_left;
}

QVariant QiWhere::right(){
    return m_right;
}

QString QiWhere::op(){
    return m_op;
}


bool QiWhere::isNull(){
    return m_isNull;
}

bool QiWhere::isField() {
    return m_right.isNull() && m_left.userType() == fieldTypeId;
}

QString QiWhere::toString() {
    if (isField()) {
        return variantToString(m_left,false);
    }

    if (m_left.isNull() || m_right.isNull())
        return "";

    QString op1,op2;

    op1 = variantToString(m_left,false);

    op2 = variantToString(m_right,true);

    return QString("%1 %2 %3").arg(op1).arg(m_op).arg(op2);
}

QString QiWhere::asc() {
    return toString() + QLatin1String(" asc");
}

QString QiWhere::desc() {
    return toString() + QLatin1String(" desc");
}

QiWhere QiWhere::expr(QString op,QVariant right){
    QiWhere w;

    w.m_left.setValue(*this);
    w.m_right = right;
    w.m_op = op;
    w.m_isNull = false;

    return w;
}

/*
QiWhere QiWhere::operator&(const QiWhere other) {
    QiWhere w;

    w.m_left.setValue(*this);
    w.m_right.setValue( other);
    w.m_op = "and";
    w.m_isNull = false;

    return w;
}
*/

QiWhere QiWhere::operator&&(const QiWhere other) {
    QiWhere w;

    w.m_left.setValue(*this);
    w.m_right.setValue( other);
    w.m_op = "and";
    w.m_isNull = false;

    return w;
}

/*
QiWhere QiWhere::operator|(const QiWhere other) {
    QiWhere w;

    w.m_left.setValue(*this);
    w.m_right.setValue( other);
    w.m_op = "or";
    w.m_isNull = false;

    return w;
}
*/

QiWhere QiWhere::operator||(const QiWhere other) {
    QiWhere w;

    w.m_left.setValue(*this);
    w.m_right.setValue( other);
    w.m_op = "or";
    w.m_isNull = false;

    return w;
}

QiWhere QiWhere::operator< (QVariant right){
    return expr("<",right);
}

QiWhere QiWhere::operator<= (QVariant right){
    return expr("<=",right);
}

QiWhere QiWhere::operator> (QVariant right){
    return expr(">",right);
}

QiWhere QiWhere::operator>= (QVariant right){
    return expr(">=",right);
}

QiWhere QiWhere::operator==(QVariant right){
    return expr("=",right);
}

QiWhere QiWhere::operator!=(QVariant right){
    return expr("<>",right);
}

QiWhere QiWhere::operator+(QVariant right){
    return expr("*",right);
}

QiWhere QiWhere::operator-(QVariant right){
    return expr("*",right);
}

QiWhere QiWhere::operator*(QVariant right){
    return expr("*",right);
}

QiWhere QiWhere::operator/(QVariant right){
    return expr("/",right);
}

QiWhere QiWhere::operator%(QVariant right){
    return expr("%",right);
}

QiWhere QiWhere::equal(QVariant right){
    return expr("=",right);
}

QiWhere QiWhere::notEqual(QVariant right){
    return expr("<>",right);
}

QiWhere QiWhere::between(QVariant v1,QVariant v2){
    QiWhereDataPriv data(QiWhereDataPriv::Between);
    data << v1 << v2;
    QVariant v;
    v.setValue(data);

    return expr("between",v);
}

QiWhere QiWhere::in(QList<QVariant> list){
    QiWhereDataPriv data(QiWhereDataPriv::In);
    data.setList(list);
    QVariant v;
    v.setValue(data);

    return expr("in",v);
}

QiWhere QiWhere::notIn (QList<QVariant> list) {
    QiWhereDataPriv data(QiWhereDataPriv::In);
    data.setList(list);
    QVariant v;
    v.setValue(data);

    return expr("not in",v);
}

QiWhere QiWhere::like (QVariant other){
    return expr("like",other);
}

QiWhere QiWhere::glob (QVariant other){
    return expr("glob",other);
}

QiWhere QiWhere::is (QVariant other){
    return expr("is",other);
}

QiWhere QiWhere::isNot(QVariant other){
    return expr("is not",other);
}

QiWhere::operator QVariant() const{
    QVariant v;
    v.setValue(*this);
    return v;
}

QString variantToString(QVariant v,bool quoteString){
    QString res;
    /// @todo Implement QVariant::convert()
//    qDebug() << v.userType() << v;

    if (v.userType() == qiWhereTypeId) {

        QiWhere w = v.value<QiWhere>();
        QString pattern = "( %1 )";
        if (w.isField()) {
            pattern = "%1";
        }
        res = QString(pattern).arg(w.toString() );
    } else if (v.userType() == fieldTypeId) {
        QiWhereFieldPriv f;
        f = v.value<QiWhereFieldPriv>();
        res = f;
    } else if (v.userType() == QMetaType::QString
               && quoteString) {
        res = QString("\"%1\"").arg(v.toString());
    } else {
        res = v.toString();
    }

    return res;
}

