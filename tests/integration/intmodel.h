#ifndef INTMODEL_H
#define INTMODEL_H

#include <qivot.h>
#include <QJsonObject>
#include <QDate>
#include <QDateTime>

/// Exercises the cross-dialect paths: auto-increment id, long text (MySQL TEXT vs
/// truncating VARCHAR), a UNIQUE string key, double / bool, JSON, and date/time types.
class IntThing : public QiModel {
    QI_MODEL
public:
    QiField<QString>     name;
    QiField<QString>     code;      // unique
    QiField<double>      score;
    QiField<bool>        active;
    QiField<QJsonObject> meta;
    QiField<QDate>       day;
    QiField<QDateTime>   ts;
};
QI_DECLARE_MODEL(IntThing, "int_thing",
    QI_FIELD(name),
    QI_FIELD(code, QiUnique),
    QI_FIELD(score),
    QI_FIELD(active),
    QI_FIELD(meta),
    QI_FIELD(day),
    QI_FIELD(ts));

/// A model with a STRING primary key and no auto-increment id column. On MySQL this
/// exercises the clause-aware VARCHAR(255) (a keyed string can't be TEXT there); on
/// Postgres it exercises save()'s REPLACE→upsert translation on a non-"id" key.
class IntTag : public QiModel {
    QI_MODEL
public:
    QiField<QString> slug;
    QiField<QString> label;
};
QI_DECLARE_MODEL_NOID(IntTag, "int_tag",
    QI_FIELD(slug, QiPrimary | QiNotNull),
    QI_FIELD(label));

#endif // INTMODEL_H
