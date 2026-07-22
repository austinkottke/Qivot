#ifndef MODEL3_H
#define MODEL3_H

#include "qimodel.h"

/// Model3 - A model without QI_MODEL / QI_DECLAREMODEL

class Model3 : public QiModel
{
public:
    QiField<QString> key;
    QiField<QString> value;

};

#endif // MODEL3_H
