#ifndef QiFOREIGNKEY_H
#define QiFOREIGNKEY_H

#include <qifield.h>
#include <qiquery.h>
#include <qimodelmetainfo.h>

/// Foreign key field
/** QiForeignKey is a special kind of QiField which can declare
  a foreign key in SQL scheme.

  The foreign key constraints in SQL are used to enforce "exists"
  relationships between tables. It is a link to other record in same
  or other database table.

  Consider the following data model:

\code
class User : public QiModel
{
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<QString> name;

    QiField<QString> passwd;
}
\endcode

  It is a data model to store user information. Each user may link up other
  with a "friendship" relation:

\code
/// A table to store the friendship status between two user
class FriendShip : public QiModel {
    QI_MODEL
public:

    /// Declara two foreign key to table User
    QiForeignKey<User> a;
    QiForeignKey<User> b;

    QiField<QDateTime> creationDate;

};
\endcode

In this example , a and b is a foreign key to user table. The
field store a integer which is the primary key of the user.

<b> Auto loading </b>

Moreover, QiForeignKey support an auto loading mechanism.
You may access the record "linked" by the QiForeignKey
through its extended interface.

By default, QiForeignKey only store the foreign key value.
Whatever the user request to access the "linked" record
field, it will be loaded automatically.

\code
   FriendShip friendship;
   friendship.load(QiWhere("id") == 1 );

   qDebug() << friendship.a.isLoaded(); // It is false. The foreign key only contains the primary key
   qDebug() << friendship.a->userId; // It will retrieve the user information "linked" by the field "a"
   qDebug() << friendship.a.isLoaded(); // It is true. The foreign key has loaded the content from database
\endcode

<b>Assigment operator</b>

To link with other record, it is just need to use the "=" operator
\code
    User user;
    user.load(QiWhere("id") == 1 );

    friendship.b = user; // friendship.b will store the primary key of "user"
\endcode

 */
template <typename T, int OnDelete = QiFkNoAction>
class QiForeignKey : public QiField<int> {
public:
    /// Construct a foreign key field
    QiForeignKey() : model(0){
    }

    /// Destruct the foreign key field
    ~QiForeignKey() {
        if (model)
            delete model;
    }

    /// Copy from other QiForeignKey object.
    /** It will copy the contained model from other
      QiForeignKey object. The original model
      will destroyed.
     */
    QiForeignKey& operator=(T& rhs) {
        QiModelMetaInfo *info = qiMetaInfo<T>();
        QString pk = info->primaryKeyName();
        if (pk.isEmpty() || pk == QLatin1String("id"))
            set(rhs.id());                       // integer id target (common case)
        else
            set(info->value(&rhs, pk));          // custom string / other primary key
        if (model){
            delete model;
            model = 0;
        }
        model = new T(rhs);

        return *this;
    }

    /// Set the primary key of the "linked" model
    QiForeignKey& operator= (QVariant val) {
        set(val);

        return *this;
    }

    /// Access the data field of the "linked" model
    T* operator->() {
        if (!model)
            model = new T();
        if ( !get().isNull() &&  !isLoaded()  ) {
            load();
        }
        return model;
    }

    /// Return an instance of the "linked" model
    T& operator() () {
        if (!model)
            model = new T();
        if ( !get().isNull() &&  !isLoaded() ) {
            load();
        }
        return *model;
    }

    static QiClause clause() {
        QVariant v = QVariant::fromValue( (void*) qiMetaInfo<T>());
        QiClause c(QiClause::FOREIGN_KEY , v );
        if (OnDelete != QiFkNoAction)
            c = c | QiClause(QiClause::FK_ON_DELETE, OnDelete);
        return c;
    }

    /// TRUE if the model is already loaded.
    inline bool isLoaded() {
        if (!model)
            return false;
        QiModelMetaInfo *info = qiMetaInfo<T>();
        QString pk = info->primaryKeyName();
        QVariant modelKey = (pk.isEmpty() || pk == QLatin1String("id"))
                            ? model->id() : info->value(model, pk);
        return !get().isNull() && !modelKey.isNull() && get() == modelKey;
    }

private:
    bool load();
    T *model;

};

template<typename T, int OnDelete>
bool QiForeignKey<T, OnDelete>::load() {
    bool res = false;
    QiModelMetaInfo *info = qiMetaInfo<T>();
    QString pk = info->primaryKeyName();
    if (pk.isEmpty())
        pk = QStringLiteral("id");
    QiQuery<T> query = QiQuery<T>().filter(QiWhere(pk, "=", get()) ).limit(1);
    if ( query.exec() ){
        if (query.next() ) {
            query.recordTo(*model);
            res = true;
        }
    }

    return res;
}


#endif // QiFOREIGNKEY_H
