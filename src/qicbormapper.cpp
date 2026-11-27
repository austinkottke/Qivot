#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaType>
#include <QStringList>
#include <QTime>
#include <QUrl>
#include <QUuid>
#include <limits>
#include "qicbormapper.h"

/// TRUE if the QMetaType id names an enum (an enum field is stored as its integer).
static bool qiIsEnumType(int type) {
    return type != QMetaType::UnknownType
        && (QMetaType(type).flags() & QMetaType::IsEnumeration);
}

QCborValue QiCborMapper::variantToCbor(const QVariant &value) {
    if (!value.isValid() || value.isNull())
        return QCborValue(QCborValue::Null);

    const int type = value.userType();
    switch (type) {
    case QMetaType::Bool:
        return QCborValue(value.toBool());
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Long:
    case QMetaType::Short:
    case QMetaType::UShort:
    case QMetaType::LongLong:
        return QCborValue(value.toLongLong());
    case QMetaType::ULong:
    case QMetaType::ULongLong: {
        // CBOR integers in Qt are signed 64-bit; beyond that, fall back to a double.
        const qulonglong u = value.toULongLong();
        if (u <= qulonglong(std::numeric_limits<qint64>::max()))
            return QCborValue(qint64(u));
        return QCborValue(double(u));
    }
    case QMetaType::Float:
    case QMetaType::Double:
        return QCborValue(value.toDouble());
    case QMetaType::QString:
        return QCborValue(value.toString());
    case QMetaType::QByteArray:
        return QCborValue(value.toByteArray());          // raw bytes, no base64
    case QMetaType::QDateTime: {
        const QDateTime dt = value.toDateTime();
        return dt.isValid() ? QCborValue(dt) : QCborValue(QCborValue::Null);
    }
    case QMetaType::QDate: {
        const QDate d = value.toDate();
        return d.isValid() ? QCborValue(d.toString(Qt::ISODate)) : QCborValue(QCborValue::Null);
    }
    case QMetaType::QTime: {
        const QTime t = value.toTime();
        return t.isValid() ? QCborValue(t.toString(Qt::ISODateWithMs)) : QCborValue(QCborValue::Null);
    }
    case QMetaType::QUrl:
        return QCborValue(value.toUrl());
    case QMetaType::QUuid:
        return QCborValue(value.toUuid());
    // Structured fields are embedded as nested CBOR, not stringified.
    case QMetaType::QJsonObject:
        return QCborValue::fromJsonValue(QJsonValue(value.toJsonObject()));
    case QMetaType::QJsonArray:
        return QCborValue::fromJsonValue(QJsonValue(value.toJsonArray()));
    case QMetaType::QJsonValue:
        return QCborValue::fromJsonValue(value.toJsonValue());
    default:
        break;
    }

    if (qiIsEnumType(type))
        return QCborValue(value.toLongLong());

    // QVariantMap, QVariantList, QStringList and anything else Qt knows how to map.
    return QCborValue::fromVariant(value);
}

QVariant QiCborMapper::cborToVariant(const QCborValue &value, int targetType) {
    if (value.isNull() || value.isUndefined() || value.isInvalid())
        return QVariant();

    const QVariant v = value.toVariant();

    // Coerce to the field's declared type, as QiJsonMapper does, so the model
    // holds what it was designed for and saves cleanly to SQL.
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
    case QMetaType::Float:
        return v.toFloat();
    case QMetaType::Bool:
        return v.toBool();
    case QMetaType::QString:
        return v.toString();
    case QMetaType::QDateTime:
        if (value.isDateTime())
            return value.toDateTime();
        if (value.isString())
            return QDateTime::fromString(value.toString(), Qt::ISODate);
        break;
    case QMetaType::QDate:
        if (value.isDateTime())
            return value.toDateTime().date();
        if (value.isString())
            return QDate::fromString(value.toString(), Qt::ISODate);
        break;
    case QMetaType::QTime:
        if (value.isString())
            return QTime::fromString(value.toString(), Qt::ISODateWithMs);
        break;
    case QMetaType::QByteArray:
        if (value.isByteArray())
            return value.toByteArray();
        if (value.isString())                             // base64 text, as JSON carries it
            return QByteArray::fromBase64(value.toString().toUtf8());
        break;
    case QMetaType::QUrl:
        if (value.isString())
            return QUrl(value.toString());
        break;
    case QMetaType::QUuid:
        if (value.isString())
            return QUuid(value.toString());
        if (value.isByteArray() && value.toByteArray().size() == 16)
            return QUuid::fromRfc4122(value.toByteArray());
        break;
    case QMetaType::QJsonObject:
        return value.isMap() ? QVariant(value.toMap().toJsonObject()) : QVariant();
    case QMetaType::QJsonArray:
        return value.isArray() ? QVariant(value.toArray().toJsonArray()) : QVariant();
    case QMetaType::QVariantMap:
        return value.isMap() ? QVariant(value.toMap().toVariantMap()) : QVariant();
    case QMetaType::QVariantList:
        return value.isArray() ? QVariant(value.toArray().toVariantList()) : QVariant();
    case QMetaType::QStringList:
        return value.isArray() ? QVariant(v.toStringList()) : QVariant();
    default:
        break;
    }

    return v;
}

bool QiCborMapper::fromCbor(QiAbstractModel *model, const QCborMap &map) {
    if (!model)
        return false;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    if (!metaInfo)
        return false;

    bool assigned = false;
    const int n = metaInfo->size();

    for (int i = 0 ; i < n ; i++) {
        const QiModelMetaInfoField *field = metaInfo->at(i);
        if (!map.contains(field->name))
            continue;

        QVariant value = cborToVariant(map.value(field->name), field->type);
        metaInfo->setValue(model, field->name, value);
        assigned = true;
    }

    return assigned;
}

QiAbstractModel *QiCborMapper::fromCbor(QiModelMetaInfo *metaInfo, const QCborMap &map) {
    if (!metaInfo)
        return nullptr;

    QiAbstractModel *model = metaInfo->create();
    fromCbor(model, map);
    return model;
}

QiSharedList QiCborMapper::fromCbor(QiModelMetaInfo *metaInfo, const QCborArray &array) {
    QiSharedList list;
    if (!metaInfo)
        return list;

    for (const QCborValue &value : array) {
        if (!value.isMap())
            continue;
        QiAbstractModel *model = fromCbor(metaInfo, value.toMap());
        if (model)
            list.append(model);
    }

    return list;
}

QCborMap QiCborMapper::toCbor(const QiAbstractModel *model) {
    QCborMap map;
    if (!model)
        return map;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    if (!metaInfo)
        return map;

    const int n = metaInfo->size();
    for (int i = 0 ; i < n ; i++) {
        const QiModelMetaInfoField *field = metaInfo->at(i);
        // convert = false: the field's own type (a QStringList stays a list, a
        // QJsonObject stays an object) rather than its SQL representation.
        QVariant value = metaInfo->value(model, field->name, false);
        map.insert(field->name, variantToCbor(value));
    }

    return map;
}

QByteArray QiCborMapper::serialize(const QiAbstractModel *model) {
    return QCborValue(toCbor(model)).toCbor();
}

QByteArray QiCborMapper::serialize(const QiSharedList &list) {
    QCborArray array;
    for (int i = 0 ; i < list.size() ; i++)
        array.append(toCbor(list.at(i)));
    return QCborValue(array).toCbor();
}

QCborValue QiCborMapper::decode(const QByteArray &cbor, QCborParserError *error) {
    QCborParserError local;
    QCborValue value = QCborValue::fromCbor(cbor, &local);
    if (error)
        *error = local;
    return value;
}
