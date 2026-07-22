#include <QDateTime>
#include <QDate>
#include <QByteArray>
#include <QJsonDocument>
#include "qijsonmapper.h"

QVariant QiJsonMapper::jsonToVariant(const QJsonValue &value, int targetType) {
    if (value.isNull() || value.isUndefined())
        return QVariant();

    QVariant v = value.toVariant();

    // Coerce to the field's declared type so the in-memory model holds the
    // value it was designed for (and saves cleanly to SQL).
    switch (targetType) {
    case QMetaType::Int:
        return v.toInt();
    case QMetaType::UInt:
        return v.toUInt();
    case QMetaType::LongLong:
        return v.toLongLong();
    case QMetaType::ULongLong:
        return v.toULongLong();
    case QMetaType::Double:
        return v.toDouble();
    case QMetaType::Bool:
        return v.toBool();
    case QMetaType::QString:
        return v.toString();
    case QMetaType::QDateTime:
        if (v.userType() == QMetaType::QString)
            return QDateTime::fromString(v.toString(), Qt::ISODate);
        break;
    case QMetaType::QDate:
        if (v.userType() == QMetaType::QString)
            return QDate::fromString(v.toString(), Qt::ISODate);
        break;
    case QMetaType::QByteArray:
        if (v.userType() == QMetaType::QString)
            return QByteArray::fromBase64(v.toString().toUtf8());
        break;
    // Structured fields keep the nested JSON verbatim.
    case QMetaType::QJsonObject:
        return value.isObject() ? QVariant(value.toObject()) : QVariant();
    case QMetaType::QJsonArray:
        return value.isArray() ? QVariant(value.toArray()) : QVariant();
    case QMetaType::QVariantMap:
        return value.toObject().toVariantMap();
    case QMetaType::QVariantList:
        return value.toArray().toVariantList();
    default:
        break;
    }

    return v;
}

/// TRUE for field types that carry nested JSON (embedded whole, not stringified).
static bool qiIsStructuredJson(int type) {
    return type == QMetaType::QJsonObject || type == QMetaType::QJsonArray
        || type == QMetaType::QVariantMap || type == QMetaType::QVariantList;
}

bool QiJsonMapper::fromJson(QiAbstractModel *model, const QJsonObject &object) {
    if (!model)
        return false;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    if (!metaInfo)
        return false;

    bool assigned = false;
    int n = metaInfo->size();

    for (int i = 0 ; i < n ; i++) {
        const QiModelMetaInfoField *field = metaInfo->at(i);
        if (!object.contains(field->name))
            continue;

        QVariant value = jsonToVariant(object.value(field->name), field->type);
        metaInfo->setValue(model, field->name, value);
        assigned = true;
    }

    return assigned;
}

QiAbstractModel *QiJsonMapper::fromJson(QiModelMetaInfo *metaInfo, const QJsonObject &object) {
    if (!metaInfo)
        return nullptr;

    QiAbstractModel *model = metaInfo->create();
    fromJson(model, object);
    return model;
}

QiSharedList QiJsonMapper::fromJson(QiModelMetaInfo *metaInfo, const QJsonArray &array) {
    QiSharedList list;
    if (!metaInfo)
        return list;

    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;
        QiAbstractModel *model = fromJson(metaInfo, value.toObject());
        if (model)
            list.append(model);
    }

    return list;
}

QJsonObject QiJsonMapper::toJson(const QiAbstractModel *model) {
    QJsonObject object;
    if (!model)
        return object;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    if (!metaInfo)
        return object;

    int n = metaInfo->size();
    for (int i = 0 ; i < n ; i++) {
        const QiModelMetaInfoField *field = metaInfo->at(i);
        // Structured fields are embedded as nested JSON (convert = false keeps the
        // QJsonObject/QJsonArray); scalar fields use the serialisation-friendly
        // representation (convert = true).
        const bool structured = qiIsStructuredJson(field->type);
        QVariant value = metaInfo->value(model, field->name, !structured);
        object.insert(field->name, QJsonValue::fromVariant(value));
    }

    return object;
}

QByteArray QiJsonMapper::serialize(const QiAbstractModel *model, QJsonDocument::JsonFormat format) {
    return QJsonDocument(toJson(model)).toJson(format);
}

QByteArray QiJsonMapper::serialize(const QiSharedList &list, QJsonDocument::JsonFormat format) {
    QJsonArray array;
    for (int i = 0 ; i < list.size() ; i++)
        array.append(toJson(list.at(i)));
    return QJsonDocument(array).toJson(format);
}
