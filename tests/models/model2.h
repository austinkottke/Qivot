/**
 * @author Ben Lau
 */

#ifndef MODEL2_H
#define MODEL2_H

#include "qimodel.h"
#include <qilist.h>

/// Model2 - Contruct a model same as model1 but using QI_DECLARE_MODEL

class Model2 : public QiModel
{
    QI_MODEL

public:
    QiField<QString> key;
    QiField<QString> value;

    inline QiSharedList initialData() const {
        QiList<Model2> res;

        Model2 item;
        for (int i = 0 ; i < 5;i++) {
            item.key = QString("initial%1").arg(i);
            item.value = QString("value%1").arg(i);
            res.append(item);
        }

        return res;
    }

signals:

public slots:

};

QI_DECLARE_MODEL(Model2,
                 "model2",
                 QI_FIELD(key,QiNotNull),
                 QI_FIELD(value)
                 );


#endif // MODEL2_H
