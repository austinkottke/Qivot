#ifndef MISC_H
#define MISC_H

#include "qimodel.h"
#include "user.h"
#include <qiforeignkey.h>

class ExamResult : public QiModel {
    QI_MODEL
public:
    QiForeignKey <User> uid;

    QiField<QString> subject;
    QiField<int> mark;

};

QI_DECLARE_MODEL(ExamResult,
                 "examresult",
                 QI_FIELD(uid,QiNotNull),
                 QI_FIELD(subject),
                 QI_FIELD(mark)

                 );

/// An example model for people's height and weight data
class HealthCheck : public QiModel {
    QI_MODEL
public:

    QiField<QString> name;
    QiField<int>     height;
    QiField<double>  weight;
    QiField<QDate>   recordDate;
};

QI_DECLARE_MODEL(HealthCheck,
                 "healthcheck",
                 QI_FIELD(name , QiNotNull),
                 QI_FIELD(height),
                 QI_FIELD(weight),
                 QI_FIELD(recordDate)
                 );

/// A model with all supported field type
class AllType : public QiModel {
    QI_MODEL
public:
    QiField<QString> string;
    QiField<int> integer;
    QiField<double> d;
    QiField<qreal> real;
    QiField<QDateTime> lastModifiedTime;
    QiField<QByteArray> data;
    QiField<bool> b;
    QiField<QStringList> sl;
};

QI_DECLARE_MODEL(AllType,
                 "alltype",
                 QI_FIELD(string),
                 QI_FIELD(integer),
                 QI_FIELD(d),
                 QI_FIELD(real),
                 QI_FIELD(lastModifiedTime),
                 QI_FIELD(data),
                 QI_FIELD(b),
                 QI_FIELD(sl)
                 );


/// A database model with private field
class PrivateFieldModel : public QiModel {
    QI_MODEL
private:
   QiField<int> field1;

};

QI_DECLARE_MODEL(PrivateFieldModel,
                 "privatefieldmodel",
                 QI_FIELD(field1)

                 );

#endif // MISC_H
