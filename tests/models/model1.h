/**
 * @author Ben Lau
 */

#ifndef MODEL1_H
#define MODEL1_H

#include "qimodel.h"

/// Example Model1 - Declare model info without using helper macro
/**
  Model1 demonstrate how to write a QiModel without using any helper macro. It is used as a control
  experiment and compare the result with Model2 , which is declared with helepr macro.

  The design of Model1 is a simulation of "Configuration" database.
 */

class Model1 : public QiModel
{
public:
    enum { QiModelDefined = 1 };

    virtual inline QString tableName();
    static inline QString TableName();
    virtual inline QiModelMetaInfo *metaInfo() const;

    QiField<QString> key;
    QiField<QString> value;
};

template<>
class QiModelMetaInfoHelper<Model1> {
public:
    typedef Model1 Table;

    enum {QiModelDefined = 1 };

    static inline QString className() {
        return "Model1";
    }

    static inline QList<QiModelMetaInfoField> fields() {
        QList<QiModelMetaInfoField> result;
        Model1 m;
        result << QiModelMetaInfoHelper<QiModel>::fields() ;
        QiModelMetaInfoField* list[] = {
                new QiModelMetaInfoField("key",offsetof(Table,key),m.key.type(),m.key.clause(),QiNotNull),
                new QiModelMetaInfoField("value",offsetof(Table,value),m.value.type(),m.value.clause()),
                0 };

        result << _qiMetaInfoCreateFields(list);
        return result;
    }
};
inline QiModelMetaInfo *Model1::metaInfo() const {
    static QiModelMetaInfo *meta = 0;
    if (!meta){
        meta = qiMetaInfo<Model1>();
    }
    return meta;
}

inline QString Model1::tableName() {
    return "model1";
}

inline QString Model1::TableName(){
    return "model1";
}




#endif // MODEL1_H
