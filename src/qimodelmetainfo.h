#ifndef QiMODELMETAINFO_H
#define QiMODELMETAINFO_H

#include <QString>
#include <QMap>
#include <QVariant>
#include <QtCore>
#include <qiclause.h>
#include <QObject>
#include <qiabstractmodel.h>
#include <qisharedlist.h>

template <typename T>
QiModelMetaInfo* qiMetaInfo();

/// The field of meta info

class QiModelMetaInfoField {
public:
    inline QiModelMetaInfoField(){
        type = QMetaType::UnknownType;
    }

    inline QiModelMetaInfoField(QString name,
                                int offset,
                                int type,
                                QiClause defaultClause,
                                QiClause c = QiClause()) :
        name(name),
        offset(offset),
        type(type) {
        clause = defaultClause | c;
    }

    /// The name of field
    QString name;

    /// Offset of the field
    int offset;

    /// The QMetaType id of the field (e.g. QMetaType::QString)
    int type;

    /// The clause of the field
    QiClause clause;

};

typedef QiAbstractModel* (*_qiAbstractModelCreateFunc)();
/// A wrapper template for QiAbstractModel creation
template <class T>
QiAbstractModel* _qiAbstractModelCreate() {
    return new T();
}

typedef QiSharedList (*_qiMetaInfoInitalDataFunc)();

template <class T>
QiSharedList _qiMetaInfoInitalData() {
    T t;
    return t.initialData();
}

/// The meta info of a database model
/**
  Each of the derived class of QiModel must be associated
  with a QiModelMetaInfo. Otherwise it is not usable.

  However, user do not need to generate by themself.
  User just need to use QI_MODEL/QI_DECLARE_MODEL macro pair to
  declare the database field of a QiModel, it will
  generate the class automatically.

  When it is created , it will set its parent to QCoreApplication,
  so that it will be destroyed automatically.
 */

class QiModelMetaInfo : private QObject {

public:

    /// Return the list of field name
    QStringList fieldNameList();

    /// List of foreign key name
    QStringList foreignKeyNameList();

    /// List of foreign key
    QList<QiModelMetaInfoField> foreignKeyList();

    /// The columns declared as PRIMARY KEY (one, several for a composite key, or
    /// "id" for a default model).
    QStringList primaryKeyFields() const;

    /// The primary-key column name: "id" for a default model, the single
    /// QiPrimary field for a custom key, or empty if there is none / it is
    /// composite. Used for foreign-key references and lazy loading.
    QString primaryKeyName() const;

    /// No. of field
    int size() const;

    /// Get the field data at index
    const QiModelMetaInfoField* at(int idx) const;

    /// Set value of a field on a model
    bool setValue(QiAbstractModel *model,QString field, const QVariant& val);

    /// Set the value of a field at index
    bool setValue(QiAbstractModel *model,int index, const QVariant& val);

    /// Get value of a field from a model
    /**
      @param model The reading model
      @param field The field name
      @param convert True if the QVariant return should be converted to a type which is suitable for saving.

      @see QiBaseField::get()
     */
    QVariant value(const QiAbstractModel *model,QString field,bool convert = false) const;

    /// Get value of a field from a model at index
    /**
      @param model The reading model
      @param index The index of the field. Which is equal to the registration order
      @param convert True if the QVariant return should be converted to a type which is suitable for saving.
     */
    QVariant value(const QiAbstractModel *model,int index ,bool convert = false) const;

    /// The table name
    QString name() const;

    /// The class name
    QString className() const;

    /// Get the initial data for the model
    QiSharedList initialData();

    /// Create an instance of the associated model type
    QiAbstractModel* create();

    /// TRUE if the table should be created WITHOUT ROWID (requires a PRIMARY KEY)
    bool isWithoutRowid() const;

    /// Create the table WITHOUT ROWID. Call before QiConnection::createTables().
    void setWithoutRowid(bool on = true);

protected:
    /// Default constructor
    QiModelMetaInfo();

    /// Set the table name
    void setName(QString val);

    /// Set the class name
    void setClassName(QString val);

    /// Register a field
    void registerField(QiModelMetaInfoField field);

    /// Register a list of fields
    void registerFields(QList<QiModelMetaInfoField> fields);

private:
    /// Field data
    QMap<QString, QiModelMetaInfoField> m_fields;

    /// Field in registration order
    QList<QiModelMetaInfoField> m_fieldList;

    QList<QiModelMetaInfoField> m_foreignKeyList;

    /// The table name
    QString m_name;
    QString m_className;
    bool m_withoutRowid = false;

    _qiAbstractModelCreateFunc createFunc;
    _qiMetaInfoInitalDataFunc initialDataFunc;

    template <typename T>
    friend QiModelMetaInfo* qiMetaInfo();

};

/// Find a meta info instance from database
/**
  @return The instance of the QiModelMetaInfo or NULL if it is not found.
 */
QiModelMetaInfo* qiFindMetaInfo(QString name);

/// Register a meta info
/**
  @remarks User should not use this function for any purpose
 */
void qiRegisterMetaInfo(QString name, QiModelMetaInfo *metaType);

/// Helper class for QiModelMetaInfo instance generation
template <typename T>
class QiModelMetaInfoHelper
{
public:
    enum {Defined = 0};
    static inline QString className() {
        return QString();
    }

    /// Get the list of available fields.
    static inline QList<QiModelMetaInfoField> fields() {
        return QList<QiModelMetaInfoField>();
    }

};

/// Create fields from a list of QiModelMetaInfoField*
static inline QList<QiModelMetaInfoField> _qiMetaInfoCreateFields(QiModelMetaInfoField*  list[]) {
    /* Didn't use variadic argument on Mac. The no. of "new" in a line is limited. */
    QList<QiModelMetaInfoField> res;

    int i = 0;
    while (list[i] != 0){
        res << *list[i];
        delete list[i];
        i++;
    }

    return res;
}


/// Find the meta info of QiModel class. If it is not existed, it will create a one automatically
template <typename T>
inline QiModelMetaInfo* qiMetaInfo() {
    static QiModelMetaInfo* metaInfo = 0;
    if (metaInfo)
        return metaInfo;

    QString name = T::TableName();

    if (T::QiModelDefined == 0){
        qWarning() << "qiMetaInfo: You should declare database model class by QI_MODEL / QI_DECLARE_MODEL pair";
        return 0;
    }

    metaInfo = (QiModelMetaInfo*) qiFindMetaInfo(name);
    if (metaInfo) {
        qWarning() << QString("Table with same name is detected! : %1 ").arg(name);
    } else {
        metaInfo = new QiModelMetaInfo();
        metaInfo->setName(name);
        metaInfo->setClassName(QiModelMetaInfoHelper<T>::className());
        metaInfo->createFunc = _qiAbstractModelCreate<T>;
        metaInfo->initialDataFunc =_qiMetaInfoInitalData<T>;

        QList<QiModelMetaInfoField> fields = QiModelMetaInfoHelper<T>::fields();
        metaInfo->registerFields(fields);
        qiRegisterMetaInfo(name,metaInfo);
    }

    return metaInfo;
}

/// Declare a model's table WITHOUT ROWID (call once before createTables()).
/** Only valid for a model that has a PRIMARY KEY (a custom string / composite
  key, see QI_DECLARE_MODEL_NOID). Saves SQLite's shadow rowid. */
template <typename T>
inline void qiWithoutRowid(bool on = true) {
    qiMetaInfo<T>()->setWithoutRowid(on);
}

/// Get the table name of the model
template <typename T>
inline QString qiModelTableName(){
    if (QiModelMetaInfoHelper<T>::Defined == 0){
        qWarning() << QString("QI_DECLARE_MODEL is required!");
    }
    return T::TableName();
}

template <typename T>
inline QString _qiModelTableName(QString model){
    if (QiModelMetaInfoHelper<T>::Defined == 0){
        qWarning() << QString("QI_DECLARE_MODEL is required to declare database model : %1!").arg(model);
    }
    return T::TableName();
}

#define QI_MODEL_NAME(X) _qiModelTableName<X>(#X)

#endif // QiMODELMETATYPE_H
