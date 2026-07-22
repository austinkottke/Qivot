#ifndef MODEL5_H
#define MODEL5_H

#include <qimodel.h>

/// Model5 - Model with unsupported data field

class Model5 : public QiModel{
    QI_MODEL
public:
    QiField<QTime> accessTime;
};

QI_DECLARE_MODEL(Model5,
                 "model5",
                 QI_FIELD(accessTime)
                 );

#endif // MODEL5_H
