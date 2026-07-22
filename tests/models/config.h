#ifndef CONFIG_H
#define CONFIG_H

#include "qimodel.h"
#include "user.h"
#include <qiforeignkey.h>

/// Config - Configuration tabel with foreign key

class Config : public QiModel
{
    QI_MODEL
public:
    QiField<QString> key;
    QiField<QString> value;

    QiForeignKey <User> uid;

};

QI_DECLARE_MODEL( Config,
                  "config",
                  QI_FIELD(key),
                  QI_FIELD(value),
                  QI_FIELD(uid , QiNotNull )
                  );


#endif // CONFIG_H
