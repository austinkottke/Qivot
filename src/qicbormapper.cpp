#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QCborStreamReader>
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

    // The common case, a value already of the field's type, is read straight
    // from the CBOR value; anything else goes through QVariant's conversions.
    switch (targetType) {
    case QMetaType::Int:
        if (value.isInteger()) return int(value.toInteger());
        return value.toVariant().toInt();
    case QMetaType::UInt:
        if (value.isInteger()) return uint(value.toInteger());
        return value.toVariant().toUInt();
    case QMetaType::LongLong:
        if (value.isInteger()) return qlonglong(value.toInteger());
        return value.toVariant().toLongLong();
    case QMetaType::ULongLong:
        if (value.isInteger()) return qulonglong(value.toInteger());
        return value.toVariant().toULongLong();
    case QMetaType::Double:
        if (value.isDouble() || value.isInteger()) return value.toDouble(double(value.toInteger()));
        return value.toVariant().toDouble();
    case QMetaType::Float:
        if (value.isDouble() || value.isInteger()) return float(value.toDouble(double(value.toInteger())));
        return value.toVariant().toFloat();
    case QMetaType::Bool:
        if (value.isBool()) return value.toBool();
        return value.toVariant().toBool();
    case QMetaType::QString:
        if (value.isString()) return value.toString();
        return value.toVariant().toString();
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
        return value.isArray() ? QVariant(value.toVariant().toStringList()) : QVariant();
    default:
        break;
    }

    return value.toVariant();
}

/// Field name -> index, built once per call so each CBOR map is walked a single
/// time. (QCborMap looks keys up by scanning, so asking it for every field in
/// turn costs fields x keys string comparisons per row.)
static QHash<QString, int> qiFieldIndex(const QiModelMetaInfo *metaInfo) {
    QHash<QString, int> index;
    const int n = metaInfo->size();
    index.reserve(n);
    for (int i = 0 ; i < n ; i++)
        index.insert(metaInfo->at(i)->name, i);
    return index;
}

static bool qiFromCbor(QiModelMetaInfo *metaInfo, const QHash<QString, int> &index,
                       QiAbstractModel *model, const QCborMap &map) {
    bool assigned = false;
    for (auto it = map.constBegin() ; it != map.constEnd() ; ++it) {
        const QCborValue key = it.key();
        if (!key.isString())
            continue;
        const auto found = index.constFind(key.toString());
        if (found == index.constEnd())
            continue;                                   // not a declared field: ignored
        const int i = found.value();
        metaInfo->setValue(model, i, QiCborMapper::cborToVariant(it.value(), metaInfo->at(i)->type));
        assigned = true;
    }
    return assigned;
}

bool QiCborMapper::fromCbor(QiAbstractModel *model, const QCborMap &map) {
    if (!model)
        return false;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    if (!metaInfo)
        return false;

    return qiFromCbor(metaInfo, qiFieldIndex(metaInfo), model, map);
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

    const QHash<QString, int> index = qiFieldIndex(metaInfo);
    for (const QCborValue &value : array) {
        if (!value.isMap())
            continue;
        QiAbstractModel *model = metaInfo->create();
        qiFromCbor(metaInfo, index, model, value.toMap());
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

// --- streaming decode: bytes -> models without an intermediate QCborValue tree.
// Qt's tree parser (QCborValue::fromCbor) is several times slower than reading
// the same bytes with QCborStreamReader, so the byte-level calls stream.

static QString qiReadString(QCborStreamReader &reader) {
    QString out;
    auto chunk = reader.readString();
    while (chunk.status == QCborStreamReader::Ok) {
        out += chunk.data;
        chunk = reader.readString();
    }
    return out;
}

static QByteArray qiReadByteArray(QCborStreamReader &reader) {
    QByteArray out;
    auto chunk = reader.readByteArray();
    while (chunk.status == QCborStreamReader::Ok) {
        out += chunk.data;
        chunk = reader.readByteArray();
    }
    return out;
}

/// Reads the value at the reader's position as a field of `targetType`.
/**
  Scalars are read straight off the stream. Text and bytes go directly into a
  field of the same type; only when they need converting (an ISO string into a
  QDateTime, say) are they wrapped in a QCborValue, as are tags and containers.
 */
static QVariant qiStreamValue(QCborStreamReader &reader, int targetType) {
    QCborValue value;
    switch (reader.type()) {
    case QCborStreamReader::UnsignedInteger:
    case QCborStreamReader::NegativeInteger:
        value = QCborValue(reader.toInteger());
        reader.next();
        break;
    case QCborStreamReader::Float16:
    case QCborStreamReader::Float:
    case QCborStreamReader::Double:
        value = QCborValue(reader.isDouble() ? reader.toDouble()
                           : reader.isFloat() ? double(reader.toFloat())
                           : double(reader.toFloat16()));
        reader.next();
        break;
    case QCborStreamReader::SimpleType:
        if (reader.isBool()) {
            value = QCborValue(reader.toBool());
            reader.next();
        } else if (reader.isNull() || reader.isUndefined()) {
            reader.next();
            return QVariant();
        } else {
            value = QCborValue::fromCbor(reader);
        }
        break;
    case QCborStreamReader::String: {
        const QString text = qiReadString(reader);
        if (targetType == QMetaType::QString)
            return text;
        value = QCborValue(text);
        break;
    }
    case QCborStreamReader::ByteArray: {
        const QByteArray bytes = qiReadByteArray(reader);
        if (targetType == QMetaType::QByteArray)
            return bytes;
        value = QCborValue(bytes);
        break;
    }
    default:                                       // tags, arrays, maps
        value = QCborValue::fromCbor(reader);
        break;
    }
    return QiCborMapper::cborToVariant(value, targetType);
}

/// Reads one map at the reader's position into `model`. The reader must be on a map.
static bool qiStreamMap(QCborStreamReader &reader, QiModelMetaInfo *metaInfo,
                        const QHash<QString, int> &index, QiAbstractModel *model) {
    bool assigned = false;
    reader.enterContainer();
    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        if (!reader.isString()) {                  // a non-text key: skip key and value
            reader.next();
            reader.next();
            continue;
        }
        const auto found = index.constFind(qiReadString(reader));
        if (found == index.constEnd()) {
            reader.next();                         // not a declared field: skip its value
            continue;
        }
        const int i = found.value();
        metaInfo->setValue(model, i, qiStreamValue(reader, metaInfo->at(i)->type));
        assigned = true;
    }
    if (reader.lastError() == QCborError::NoError)
        reader.leaveContainer();
    return assigned;
}

static void qiReportError(QCborStreamReader &reader, QCborParserError *error) {   // lastError() isn't const on Qt 5
    if (!error)
        return;
    error->error = reader.lastError();
    error->offset = reader.currentOffset();
}

bool QiCborMapper::decodeInto(QiAbstractModel *model, const QByteArray &cbor, QCborParserError *error) {
    if (error)
        *error = QCborParserError();
    if (!model || !model->metaInfo())
        return false;

    QiModelMetaInfo *metaInfo = model->metaInfo();
    QCborStreamReader reader(cbor);
    bool assigned = false;
    if (reader.isMap())
        assigned = qiStreamMap(reader, metaInfo, qiFieldIndex(metaInfo), model);
    else if (reader.lastError() == QCborError::NoError && reader.isValid())
        reader.next();                             // valid CBOR but not a map: nothing to read

    if (reader.lastError() != QCborError::NoError) {
        qiReportError(reader, error);
        return false;
    }
    return assigned;
}

QiSharedList QiCborMapper::decodeList(QiModelMetaInfo *metaInfo, const QByteArray &cbor,
                                      QCborParserError *error) {
    if (error)
        *error = QCborParserError();
    QiSharedList list;
    if (!metaInfo)
        return list;

    QCborStreamReader reader(cbor);
    if (!reader.isArray()) {
        if (reader.lastError() != QCborError::NoError || !reader.isValid())
            qiReportError(reader, error);
        return list;                               // not an array: empty, like QiJsonMapper
    }

    const QHash<QString, int> index = qiFieldIndex(metaInfo);
    reader.enterContainer();
    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        if (!reader.isMap()) {
            reader.next();                         // not a map: skipped
            continue;
        }
        QiAbstractModel *model = metaInfo->create();
        qiStreamMap(reader, metaInfo, index, model);
        list.append(model);
    }

    if (reader.lastError() != QCborError::NoError) {
        qiReportError(reader, error);
        return QiSharedList();                     // bad bytes: nothing half-read
    }
    reader.leaveContainer();
    return list;
}
