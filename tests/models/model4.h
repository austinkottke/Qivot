#ifndef MODEL4_H
#define MODEL4_H

#include "qimodel.h"
#include "model1.h"

/// Model 4 - Standard declaration with using Model1 as parent

class Model4 : public Model1
{
    QI_MODEL
public:

    QiField<QString> description;
    QiField<QString> help;
};

QI_DECLARE_MODEL2( Model4,
                  "model4",
                  Model1,
                  QI_FIELD(description),
                  QI_FIELD(help , QiDefault( qiEscape("...") ))
                  );

#endif // MODEL4_H
