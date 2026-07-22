#include <QtCore>
#include <QSharedData>
#include "qiexpression.h"
#include "qiwhere_p.h"

class QiExpressionPriv : public QSharedData {
public:
    /// The string expression
    QString m_string;

    QMap<QString,QVariant> m_values;

    int m_num;

    bool m_null;

    void process(QiWhere& where);

    /// A recusrive verison of process()
    QString _process(QiWhere& where);

    /// A recursive function to prcess the operand in QiWhere
    QString _process(QVariant v);

    QString _process(QiWhereDataPriv& data);

    /// Bind the values , and return its argument name
    QString bind(QVariant v);
};


static int qiExprTypeId = qMetaTypeId<QiWhere>();

static int dataPrivTypeId = qMetaTypeId<QiWhereDataPriv>();

QiExpression::QiExpression(){
    d = new QiExpressionPriv();

    d->m_null = true;
}

QiExpression::QiExpression(QiWhere where)
{
    d = new QiExpressionPriv();

    d->process(where);
    d->m_null = false;
}

QiExpression::QiExpression(const QiExpression& rhs) : d(rhs.d){
}

QiExpression & QiExpression::operator=(const QiExpression &rhs){
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

QiExpression::~QiExpression(){
}

bool QiExpression::isNull(){
    return d->m_null;
}


QString QiExpression::string(){
    return d->m_string;
}

QMap<QString,QVariant> QiExpression::bindValues(){
    return d->m_values;
}

void QiExpressionPriv::process(QiWhere& where){
    m_string.clear();
    m_values.clear();

    m_num = 0;

    if (!where.isNull())
        m_string = _process(where);
}

QString QiExpressionPriv::_process(QiWhere& where) {
    if (where.isField())
        return where.toString();

    QString leftString,rightString;

    leftString = _process(where.left());
    rightString = _process(where.right());

    return QString("%1 %2 %3").arg(leftString).arg(where.op()).arg(rightString);

}

QString QiExpressionPriv::_process(QVariant v) {
    QString res;
    if (v.userType() == qiExprTypeId) {
        QiWhere w = v.value<QiWhere>();

        if (w.isField()) {
            res = w.toString();
        } else {
            res = QString("(%1)").arg(_process(w));
        }
    } else if (v.userType() == dataPrivTypeId) {
        QiWhereDataPriv data = v.value<QiWhereDataPriv>();
        res = _process(data);
    } else {
        res = bind(v);
//        QString arg = QString(":arg%1").arg(m_num++);
//        m_values[arg] = v;
//        res = arg;
    }

    return res;
}

QString QiExpressionPriv::_process(QiWhereDataPriv& data){
    QString res;
    QString arg,arg1,arg2;
    QStringList args;
    QList<QVariant> list;
    list = data.list();

    QVariant v;

    switch (data.type() ) {
    case QiWhereDataPriv::Between:
        Q_ASSERT(list.size() == 2);
        arg1 = bind(list.at(0));
        arg2 = bind(list.at(1));
        res = QString("%1 and %2").arg(arg1).arg(arg2);
        break;

    case QiWhereDataPriv::In:
        foreach (v,list) {
            arg = bind(v);
            args << arg;
        }
        res = QString("(%1)").arg(args.join(","));
        break;

    default:
        qWarning() << "QiWhereDataPriv - Unsupported type";
        break;
    }

    return res;
}

QString QiExpressionPriv::bind(QVariant v){
    QString arg = QString(":arg%1").arg(m_num++);
    m_values[arg] = v;
    return arg;
}
