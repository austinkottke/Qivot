#ifndef QiFIELD_H
#define QiFIELD_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QStringList>
#include <qibasefield.h>

/// Database field
/**
    QiField store the value of a field in database model. The format is QVariant.
    Therefore you may assign a QVariant to QiField direclty, and you may access
    the stored QVariant by using operator-> or get() function.

    @see QiModel
    @see QiForeignKey
 */

template <typename T>
class QiField : public QiBaseField
{
public:
    /// Default constructor
    QiField(){
    }

    /// Return the QMetaType id of the field
    static int type(){
        return qMetaTypeId<T>();
    }

    /// Copy the value from a QVariant object
    inline QVariant operator=(const QVariant &val){
        set(val);
        return val;
    }

    /// Compare with other QiField
    inline bool operator==(const QiField& rhs) const {
        return get() == rhs.get();
    }

    /// Compare with QVariant type
    inline bool operator==(const QVariant &rhs) const {
        return get() == rhs;
    }

    /// Compare with QVariant type
    inline bool operator!=(const QVariant &rhs) const {
        return get() != rhs;
    }

    /// Compare with its template type
    inline bool operator==(const T& t) const {
        return get() == t;
    }

    /// Compare with its template type
    inline bool operator!=(const T& t) const {
        return get() != t;
    }

    /// Compare with string type
    inline bool operator==(const char *string) const {
        return get() == QString(string);
    }

    /// Compare with string type
    inline bool operator!=(const char *string) const {
        return get() != QString(string);
    }

    /// Get the value of the field
    inline QVariant get(bool convert = false) const {
        return QiBaseField::get(convert);
    }

    /// Set the value of the field
    inline bool set(QVariant value) {
        return QiBaseField::set(value);
    }

    /// Cast it to the template type
    inline operator T() const {
        QVariant v = get();
        return v.value<T>();
    }

};

// The built-in structured-field specializations below are defined `inline` in
// the header (not out-of-line in a .cpp) on purpose. get()/set() are virtual,
// and MSVC does not reliably emit an out-of-line explicit specialization of a
// virtual member unless the enclosing class template is instantiated in that
// same translation unit — which it isn't. Defining them inline gives every TU
// (and every compiler) the definition, matching the QI_DECLARE_CONVERTER pattern.

namespace QiFieldDetail {

/// Escape & and " so list items survive being joined with the separator.
inline QString escapeStringListItem(QString value) {
    QString result;
    QMap<char, QString> map;
    map['&'] = "&amp;";
    map['"'] = "&quot;";

    result.reserve(value.size() * 1.1);
    int n = value.size();
    for (int i = 0; i < n; i++) {
        QChar c = value.at(i);
        char l = c.toLatin1();
        if (map.contains(l)) {
            result += map[l];
        } else {
            result += c;
        }
    }
    result.squeeze();
    return result;
}

inline QString unescapeStringListItem(QString value) {
    QMap<QString, char> map;
    map["&amp;"] = '&';
    map["&quot;"] = '"';

    QString result;
    result.reserve(value.size());

    int n = value.size();
    for (int i = 0; i < n; i++) {
        QChar q = value.at(i);

        if (q == '&') {
            bool found = false;
            QMap<QString, char>::const_iterator iter = map.constBegin();
            while (iter != map.end()) {
                QString key = iter.key();
                int len = key.size();

                QStringView ref = QStringView(value).mid(i, len);
                if (ref == QStringView(key)) {
                    q = iter.value();
                    i += len - 1;
                    found = true;
                    break;
                }
                iter++;
            }

            if (!found) {
                qWarning() << QString("Invalid escaped string : %1").arg(value);
            }
        }

        result += q;
    }

    result.squeeze();
    return result;
}

/// Separator for QStringList storage.
inline const char *stringListSeparator() { return " & "; }

} // namespace QiFieldDetail

template <>
inline bool QiField<QStringList>::set(QVariant value) {
    if (value.userType() == QMetaType::QString) {
        QString str = value.toString();
        QStringList list = str.split(QiFieldDetail::stringListSeparator());
        QStringList result;
        foreach (str, list) {
            result << QiFieldDetail::unescapeStringListItem(str);
        }
        value = result;
    }
    return QiBaseField::set(value);
}

template <>
inline QVariant QiField<QStringList>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);

    if (convert && val.userType() == QMetaType::QStringList) {
        QStringList list = val.toStringList();
        QStringList result;
        QString str;
        foreach (str, list) {
            result << QiFieldDetail::escapeStringListItem(str);
        }
        val = result.join(QiFieldDetail::stringListSeparator());
    }

    return val;
}

// Structured JSON fields: hold a nested QJsonObject / QJsonArray in memory,
// serialize to a JSON string only when persisted (convert = true).
template <>
inline bool QiField<QJsonObject>::set(QVariant value) {
    if (value.userType() == QMetaType::QString)
        value = QJsonDocument::fromJson(value.toString().toUtf8()).object();
    return QiBaseField::set(value);
}

template <>
inline QVariant QiField<QJsonObject>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);
    if (convert && val.userType() == QMetaType::QJsonObject)
        return QString::fromUtf8(QJsonDocument(val.toJsonObject()).toJson(QJsonDocument::Compact));
    return val;
}

template <>
inline bool QiField<QJsonArray>::set(QVariant value) {
    if (value.userType() == QMetaType::QString)
        value = QJsonDocument::fromJson(value.toString().toUtf8()).array();
    return QiBaseField::set(value);
}

template <>
inline QVariant QiField<QJsonArray>::get(bool convert) const {
    QVariant val = QiBaseField::get(convert);
    if (convert && val.userType() == QMetaType::QJsonArray)
        return QString::fromUtf8(QJsonDocument(val.toJsonArray()).toJson(QJsonDocument::Compact));
    return val;
}

/// Primary key field

class QiPrimaryKey : public QiField<int> {
public:
    QiPrimaryKey();
    static QiClause clause();

    inline QVariant operator=(const QVariant &val){
        set(val);
        return val;
    }
};

/// Store an arbitrary C++ value type in a column via a pair of conversion functions.
/**
  Teaches QiField how to persist a custom type `TYPE`. Provide two free/static
  functions (by name — not inline lambdas, so the macro stays comma-safe):

    - `TO_STORAGE_FN(const TYPE&) -> QVariant`   a value SQLite can store
                                                 (int / double / QString / ...)
    - `FROM_STORAGE_FN(const QVariant&) -> TYPE` rebuild the value when loading

  `TYPE` must be registered with `Q_DECLARE_METATYPE(TYPE)`, and the column's SQL
  type should be set with `QI_FIELD_AS(field, "TEXT")` (or INTEGER/REAL/…).

\code
    struct GeoPoint { double lat, lng; };
    Q_DECLARE_METATYPE(GeoPoint)

    static QVariant geoToStorage(const GeoPoint &p) {
        return QStringLiteral("%1,%2").arg(p.lat).arg(p.lng);
    }
    static GeoPoint geoFromStorage(const QVariant &v) {
        const QStringList s = v.toString().split(',');
        return GeoPoint{ s.value(0).toDouble(), s.value(1).toDouble() };
    }
    QI_DECLARE_CONVERTER(GeoPoint, geoToStorage, geoFromStorage)

    // in the model:  QI_FIELD_AS(location, "TEXT")
\endcode

  Place the macro (and the two functions) before any model that uses
  `QiField<TYPE>`.
 */
#define QI_DECLARE_CONVERTER(TYPE, TO_STORAGE_FN, FROM_STORAGE_FN)          \
    template <> inline bool QiField<TYPE>::set(QVariant value) {            \
        if (value.isValid() && !value.isNull()                             \
                && value.userType() != qMetaTypeId<TYPE>())                \
            value = QVariant::fromValue<TYPE>( FROM_STORAGE_FN(value) );    \
        return QiBaseField::set(value);                                     \
    }                                                                       \
    template <> inline QVariant QiField<TYPE>::get(bool convert) const {    \
        QVariant _v = QiBaseField::get(convert);                           \
        if (convert && _v.userType() == qMetaTypeId<TYPE>())               \
            return QVariant( TO_STORAGE_FN( _v.value<TYPE>() ) );          \
        return _v;                                                          \
    }

#endif // QiFIELD_H
