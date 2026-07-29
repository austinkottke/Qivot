#ifndef INTMODEL_H
#define INTMODEL_H

#include <qivot.h>
#include <QJsonObject>

/// A model that exercises the tricky cross-dialect paths:
///   - auto-increment id            (SQLite/MySQL lastInsertId vs Postgres RETURNING)
///   - a long plain string          (MySQL TEXT, not a truncating VARCHAR)
///   - a UNIQUE string key           (MySQL VARCHAR(255), used as the upsert conflict target)
///   - double / bool                 (type round-trip)
///   - a JSON object                 (MySQL JSON, Postgres JSONB, SQLite TEXT)
class IntThing : public QiModel {
    QI_MODEL
public:
    QiField<QString>     name;    // plain text
    QiField<QString>     code;    // unique
    QiField<double>      score;
    QiField<bool>        active;
    QiField<QJsonObject> meta;
};

QI_DECLARE_MODEL(IntThing, "int_thing",
    QI_FIELD(name),
    QI_FIELD(code, QiUnique),
    QI_FIELD(score),
    QI_FIELD(active),
    QI_FIELD(meta));

#endif // INTMODEL_H
