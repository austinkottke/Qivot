#ifndef QiABSTRACTMODELLIST_H
#define QiABSTRACTMODELLIST_H

#include <QSharedDataPointer>
#include <QMetaType>
#include <qiabstractmodel.h>
#include <QExplicitlySharedDataPointer>


class QiSharedListPriv;

/// QiSharedList is the base class of QiList
/**
  QiSharedList is the base class of QiList that implements the storage
  and QiAbstractModel management. It is a explicity shared class which could be
  used to exchange data between different objects.

  Although most of the class in DQuest return QiSharedList instead of QiList.
  You should use QiList in your code. It is binded to specific model and
  provide type checking code.

  Moreover, QiList and QiSharedList are exchangable. That means
  QiSharedList can be converted to QiList, and vice visa.

  If QiSharedList is used alone, it could store any combination
  of QiModel subclass ,which is not restricted to a single type.
  In this condition , the class is no longer exchangable with DSList.

  For the list created by QiList, it is binded to specific model
  (metaInfo() return null), the append function will only accept
  a single type.

  @remarks It is an explicity shared class
  */

class QiSharedList
{
public:
    /// Default constructor
    QiSharedList();

    /// Copy constructor
    QiSharedList(const QiSharedList &);

    /// Assignment operator overloading
    QiSharedList &operator=(const QiSharedList &);

    /// Default destructor
    virtual ~QiSharedList();

    /// Get the size of the list
    int size() const;

    /// Returns the item at index position i in the list. i must be a valid index position in the list (i.e., 0 <= i < size()).

    QiAbstractModel* at(int index) const;

    /// Append a model to the list.
    /**
      @param model The input model. Ownership will be taken.
      @return TRUE if it is appended successfully. Otherwise it is false

      @see metaInfo
     */
    bool append(QiAbstractModel* model);

    /// Removes all items from the list.
    void clear();

    /// Removes the item at index position i. i must be a valid index position in the list (i.e., 0 <= i < size()).
    void removeAt(int index);

    /// Save all the contained item to database
    /**
      @param forceInsert  TRUE if the data should be inserted to the database as a new record regardless of the original id. The id field will be updated after operation.
      @param forceAllField TRUE if all the field should be saved no matter it is null or not. If false, then null field will be skipped.
      @see QiModel::save()

      @return TRUE if all of the item is successfully saved
     */

    bool save(bool forceInsert = false,bool forceAllField = false);

    /// Get the binded model's meta info
    /** If this function non-null value , then this object is binded
      to specific model, it could only be used to store single model type.
      @return The binded model's QiModelMetaInfo object.
     */
    QiModelMetaInfo* metaInfo();

protected:
    /// Set the binded data model by it's meta info
    void setMetaInfo(QiModelMetaInfo* metaInfo);

private:
    QExplicitlySharedDataPointer<QiSharedListPriv> data;
};

Q_DECLARE_METATYPE(QiSharedList)

#endif // QiABSTRACTMODELLIST_H
