/**
 * @author Ben Lau
 */

#include <QtCore>
#include "qiclause.h"


QiClause::QiClause(){

}

QiClause::QiClause(Type type) {
    m_flags[type] = true;
}

QiClause::QiClause(Type type, QVariant value) {
    m_flags[type] = value;
}

bool QiClause::testFlag(Type type){
    return m_flags.contains(type);
}

QVariant QiClause::flag(Type type){
    if (testFlag(type)){
        return m_flags[type];
    }
    return QVariant();
}

void QiClause::setFlag(Type type,QVariant val){
    m_flags[type] = val;
}

QiClause::QiClause(const QiRule &rule) {
    m_flags[RULES] = QVariant::fromValue(QiRuleList() << rule);
}

QiRuleList QiClause::rules() const {
    return m_flags.contains(RULES) ? m_flags.value(RULES).value<QiRuleList>() : QiRuleList();
}

QiClause QiClause::operator|(const QiClause& other) {
    QiClause clause;

    QMap<Type, QVariant >::const_iterator iter = m_flags.constBegin();
    while (iter != m_flags.constEnd() ) {
         clause.setFlag(iter.key(),iter.value());
         iter++;
    }

    iter = other.m_flags.constBegin();
    while (iter != other.m_flags.constEnd() ) {
         clause.setFlag(iter.key(), iter.value());
         iter++;
    }

    // Rules add up rather than the right-hand ones replacing the left.
    if (m_flags.contains(RULES) && other.m_flags.contains(RULES))
        clause.setFlag(RULES, QVariant::fromValue(rules() + other.rules()));

    return clause;
}

QString qiEscape(QString val,bool trimStrings) {
    QString res;

    if (trimStrings)
        res = val.trimmed();
    else
        res = val;

    res.replace(QLatin1Char('\''), QLatin1String("''"));
    res = QString("'%1'").arg(res);

    return res;
}
