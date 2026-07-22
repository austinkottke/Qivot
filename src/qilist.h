#ifndef QiLIST_H
#define QiLIST_H

#include <QSharedDataPointer>
#include <QExplicitlySharedDataPointer>
#include <qiabstractmodel.h>
#include <qimodelmetainfo.h>
#include <qisharedlist.h>

/// Storage of a list of model item instance
/**

  @remarks It is an explicity shared class
 */

template <class T>
class QiList : public QiSharedList
{
public:

    /// Construct an empty QiList instance
    QiList() : QiSharedList() {
        setMetaInfo(qiMetaInfo<T>());
    }

    /// Construct a QiList which is the reference to other QiList instance
    QiList(const QiList &rhs) : QiSharedList(rhs) {
    }

    /// Construct a QiList which is a refernece to a QiSharedList
    QiList(const QiSharedList& rhs ) : QiSharedList(rhs) {
    }

    /// Make a reference to other QiList
    /** It will drop the reference of current data and refer to other QiList
     */
    QiList &operator=(const QiList &rhs){
        QiSharedList::operator=( rhs);
        return *this;
    }

    /// Returns the item at index position i in the list. i must be a valid index position in the list (i.e., 0 <= i < size()).
    T* at(int i) const {
        QiAbstractModel* m = QiSharedList::at(i);
        if (m->metaInfo() != qiMetaInfo<T>() ) {
            qWarning() << QString("QiList::at() - Can not convert %1 to %2")
                          .arg(m->metaInfo()->className()).arg(qiMetaInfo<T>()->className());
            m = 0;
        }
        return (T*) m;
    }

    /// Append a model to the list
    /**
      It is a overloaded function for append().
     */
    QiList& operator<<(const T& model){
        if (!append(model)){
            qWarning() << "QiList::operator<<(model) - Failed to append";
        }
        return *this;
    }


    /// Append a model to the list.
    /**
      @param model The input model. A copy of instance of the model will be stored to the list.
     */
    bool append(const T& model) {
        T* t = new T(model);
        bool res = QiSharedList::append(t);
        if (!res) {
            delete t;
        }
        return res;
    }

    /// Append a model to the list.
    /**
      @param model The input model. Ownership will be taken.
     */

    bool append(T* model) {
        return QiSharedList::append(model);
    }

    /// Cast it to QiSharedList
    operator QiSharedList() {
        QiSharedList res (*this);
        return res;
    }

};

template <class T>
inline QDebug operator<< (QDebug d, const QiList<T>& rhs ){
    QStringList record;
    int n = rhs.size();
    QiModelMetaInfo *metaInfo;
    for (int i = 0 ; i < n;i++) {
        QiAbstractModel *model = rhs.at(i);
        metaInfo = model->metaInfo();
        QStringList fields = metaInfo->fieldNameList();
        QStringList col;
        foreach (QString field,fields){
            QVariant value = metaInfo->value(model,field);
            if (value.isNull())
                continue;
            col << QString("%1=%2").arg(field).arg(value.toString());
        }

        QString res = QString("(%2)")
                      .arg(col.join(","));
        record << res;
    }

    metaInfo = qiMetaInfo<T>();

    d.nospace() << QString("%1[%2]")
                    .arg(metaInfo->className())
                    .arg(record.join(","));

    return d.space();
}


#endif // QiLIST_H
