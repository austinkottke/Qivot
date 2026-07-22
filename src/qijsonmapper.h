#ifndef QiJSONMAPPER_H
#define QiJSONMAPPER_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QByteArray>
#include <qimodelmetainfo.h>
#include <qiabstractmodel.h>
#include <qisharedlist.h>
#include <qilist.h>

/// Maps between JSON (QJsonObject / QJsonArray) and DQuest models
/**
  QiJsonMapper converts JSON documents into QiModel instances and back. Only the
  fields declared on the model are considered — any extra keys in the JSON are
  ignored, and any model field missing from the JSON is left untouched. Values
  are coerced to each field's declared type (so an ISO-8601 string maps into a
  QiField\<QDateTime\>, a JSON number into a QiField\<int\>, and so on).

  This class is pure QtCore — it performs no I/O and no threading. For loading
  JSON from an HTTP endpoint on a worker thread, see QiJsonRequest.

\code
    QJsonArray array = QJsonDocument::fromJson(bytes).array();
    QiList<Post> posts = QiJsonMapper::map<Post>(array);
\endcode
 */
class QiJsonMapper {
public:
    /// Populate an existing model from a JSON object
    /**
      @param model  The model to populate (its fields are matched by name).
      @param object The source JSON object.
      @return TRUE if at least one field was assigned.
     */
    static bool fromJson(QiAbstractModel *model, const QJsonObject &object);

    /// Create and populate a model of the given meta type from a JSON object
    /**
      @return A newly allocated model. Ownership is passed to the caller.
     */
    static QiAbstractModel *fromJson(QiModelMetaInfo *metaInfo, const QJsonObject &object);

    /// Map a JSON array of objects into a QiSharedList of the given meta type
    /**
      Non-object array elements are skipped. The returned list owns the models.
     */
    static QiSharedList fromJson(QiModelMetaInfo *metaInfo, const QJsonArray &array);

    /// Serialize a model to a JSON object (one member per declared field)
    static QJsonObject toJson(const QiAbstractModel *model);

    /// Typed convenience: map a JSON array into a QiList\<T\>
    template <typename T>
    static QiList<T> map(const QJsonArray &array) {
        return QiList<T>( fromJson(qiMetaInfo<T>(), array) );
    }

    /// Typed convenience: map a JSON object into a T
    template <typename T>
    static T map(const QJsonObject &object) {
        T model;
        fromJson(&model, object);
        return model;
    }

    /// Coerce a JSON value to a QVariant of the field's declared QMetaType id
    static QVariant jsonToVariant(const QJsonValue &value, int targetType);

    // --- pure JSON <-> object (raw bytes/string, no database involved) -------

    /// Deserialize a JSON *object* document into a model of type T.
    /**
      Pure in-memory: parses the bytes and populates a T. Nothing is persisted.
\code
    User u = QiJsonMapper::deserialize<User>(bytes);   // JSON text -> object
\endcode
     */
    template <typename T>
    static T deserialize(const QByteArray &json) {
        return map<T>(QJsonDocument::fromJson(json).object());
    }

    /// Deserialize a JSON *array* document into a QiList<T>.
    template <typename T>
    static QiList<T> deserializeList(const QByteArray &json) {
        return map<T>(QJsonDocument::fromJson(json).array());
    }

    /// Serialize a model to a JSON object document (raw bytes).
    /**
\code
    QByteArray bytes = QiJsonMapper::serialize(&user);   // object -> JSON text
\endcode
     */
    static QByteArray serialize(const QiAbstractModel *model,
                                QJsonDocument::JsonFormat format = QJsonDocument::Indented);

    /// Serialize a list of models to a JSON array document (raw bytes).
    static QByteArray serialize(const QiSharedList &list,
                                QJsonDocument::JsonFormat format = QJsonDocument::Indented);
};

#endif // QiJSONMAPPER_H
