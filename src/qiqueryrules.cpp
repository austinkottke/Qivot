#include "qiqueryrules.h"
#include <QSharedData>
#include "qisharedquery_p.h"

QiQueryRules::QiQueryRules() : data(new QiSharedQueryPriv)
{
}

QiQueryRules::QiQueryRules(const QiQueryRules &rhs) : data(rhs.data)
{
}

QiQueryRules &QiQueryRules::operator=(const QiQueryRules &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QiQueryRules &QiQueryRules::operator=(const QiSharedQuery &rhs){
    data.operator =(rhs.data);
    return *this;
}

QiQueryRules::~QiQueryRules()
{
}

int QiQueryRules::limit(){
    return data->limit;
}

int QiQueryRules::offset(){
    return data->offset;
}

QStringList QiQueryRules::groupBy(){
    return data->groupBy;
}

QiExpression QiQueryRules::having(){
    return data->having;
}

QiExpression QiQueryRules::expression(){
    return data->expression;
}

QString QiQueryRules::func(){
    return data->func;
}

QiModelMetaInfo *QiQueryRules::metaInfo(){
    return data->metaInfo;
}

QStringList QiQueryRules::fields(){
    return data->fields;
}

QStringList QiQueryRules::orderBy() {
    return data->orderBy;
}

QList<QiBaseJoin> QiQueryRules::joins() {
    return data->joins;
}

bool QiQueryRules::distinct() {
    return data->distinct;
}
