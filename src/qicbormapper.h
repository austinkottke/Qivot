#ifndef QiCBORMAPPER_H
#define QiCBORMAPPER_H

#include <QByteArray>
#include <QCborArray>
#include <QCborMap>
#include <QCborValue>
#include <qimodelmetainfo.h>
#include <qiabstractmodel.h>
#include <qisharedlist.h>
#include <qilist.h>

/// Maps between CBOR (QCborMap / QCborArray) and Qivot models
/**
  QiCborMapper is the binary twin of QiJsonMapper. CBOR (RFC 8949) is a compact
  binary format with the same shape as JSON — maps, arrays, strings, numbers —
  plus a few types JSON lacks. It is built into Qt Core, so this adds no
  dependency. A model becomes a CBOR map keyed by field name.

  Where it differs from JSON:
  - QByteArray fields are written as raw bytes, not base64 text.
  - QDateTime fields are written as CBOR date-times (tag 0), QUrl and QUuid as
    their CBOR tags; QDate and QTime as ISO-8601 text.
  - 64-bit integers are kept exact instead of passing through a double.
  - Nested QJsonObject / QJsonArray / QVariantMap / QVariantList / QStringList
    fields are embedded as nested CBOR maps and arrays.
  - A NULL field is written as CBOR null.

  Reading is as forgiving as QiJsonMapper: only declared fields are read, extra
  keys are ignored, missing keys leave the field untouched, and values are
  coerced to the field's type (an ISO string still reads into a QDateTime, a
  base64 string into a QByteArray). Pure QtCore: no I/O, no threads.

\code
    QByteArray bytes = QiCborMapper::serialize(&user);        // model -> CBOR
    User copy = QiCborMapper::deserialize<User>(bytes);       // CBOR -> model

    QByteArray many = QiCborMapper::serialize(User::objects().all());
    QiList<User> users = QiCborMapper::deserializeList<User>(many);
\endcode
 */
class QiCborMapper {
public:
    /// Populate an existing model from a CBOR map
    /**
      @param model The model to populate (its fields are matched by name).
      @param map   The source CBOR map; its keys are field names.
      @return TRUE if at least one field was assigned.
     */
    static bool fromCbor(QiAbstractModel *model, const QCborMap &map);

    /// Create and populate a model of the given meta type from a CBOR map
    /**
      @return A newly allocated model. Ownership is passed to the caller.
     */
    static QiAbstractModel *fromCbor(QiModelMetaInfo *metaInfo, const QCborMap &map);

    /// Map a CBOR array of maps into a QiSharedList of the given meta type
    /**
      Elements that are not maps are skipped. The returned list owns the models.
     */
    static QiSharedList fromCbor(QiModelMetaInfo *metaInfo, const QCborArray &array);

    /// Serialize a model to a CBOR map (one entry per declared field)
    static QCborMap toCbor(const QiAbstractModel *model);

    /// Typed convenience: map a CBOR array into a QiList\<T\>
    template <typename T>
    static QiList<T> map(const QCborArray &array) {
        return QiList<T>( fromCbor(qiMetaInfo<T>(), array) );
    }

    /// Typed convenience: map a CBOR map into a T
    template <typename T>
    static T map(const QCborMap &map) {
        T model;
        fromCbor(&model, map);
        return model;
    }

    /// Coerce a CBOR value to a QVariant of the field's declared QMetaType id
    static QVariant cborToVariant(const QCborValue &value, int targetType);

    /// Convert a field value to CBOR, keeping its native type where CBOR has one
    static QCborValue variantToCbor(const QVariant &value);

    // --- raw bytes <-> object (no database involved) -------------------------

    /// Deserialize an encoded CBOR *map* into a model of type T.
    /**
      Pure in-memory: decodes the bytes and populates a T. Nothing is persisted.
      If the bytes are not valid CBOR, or not a map, a default T is returned and
      `error` (when given) says why.
     */
    template <typename T>
    static T deserialize(const QByteArray &cbor, QCborParserError *error = nullptr) {
        return map<T>(decode(cbor, error).toMap());
    }

    /// Deserialize an encoded CBOR *array* of maps into a QiList<T>.
    template <typename T>
    static QiList<T> deserializeList(const QByteArray &cbor, QCborParserError *error = nullptr) {
        return map<T>(decode(cbor, error).toArray());
    }

    /// Serialize a model to an encoded CBOR map.
    static QByteArray serialize(const QiAbstractModel *model);

    /// Serialize a list of models to an encoded CBOR array of maps.
    static QByteArray serialize(const QiSharedList &list);

private:
    static QCborValue decode(const QByteArray &cbor, QCborParserError *error);
};

#endif // QiCBORMAPPER_H
