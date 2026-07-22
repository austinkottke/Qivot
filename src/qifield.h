#ifndef QiFIELD_H
#define QiFIELD_H

#include <QJsonObject>
#include <QJsonArray>
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

template <>
bool QiField<QStringList>::set(QVariant value);

template <>
QVariant QiField<QStringList>::get(bool convert) const;

// Structured JSON fields: hold a nested QJsonObject / QJsonArray in memory,
// serialize to a JSON string only when persisted (convert = true).
template <>
bool QiField<QJsonObject>::set(QVariant value);
template <>
QVariant QiField<QJsonObject>::get(bool convert) const;

template <>
bool QiField<QJsonArray>::set(QVariant value);
template <>
QVariant QiField<QJsonArray>::get(bool convert) const;

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

#endif // QiFIELD_H
