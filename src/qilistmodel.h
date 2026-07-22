#ifndef QiLISTMODEL_H
#define QiLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <qisharedlist.h>
#include <qilist.h>
#include <qimodelmetainfo.h>

/// A QAbstractListModel that exposes Qivot records to QML (and Qt item views)
/**
  QiListModel wraps a QiList / query result and presents each declared field as
  a **role**, so a QML `ListView` (or any `QAbstractItemView`) can bind directly
  to the field names — no per-model boilerplate. It works for any QiModel using
  Qivot's field reflection; the model does not need to be a Q_GADGET.

  Fill it from a query, then expose it to QML — typically as a property of a
  QML-registered controller (see examples/qmlmodel), so a ListView binds to the
  field names as roles:

\code
    QiListModel *posts = new QiListModel(this);
    posts->setList( Post::objects().orderBy("published desc").all() );
\endcode

\code
    // QML: the roles are the model's field names
    ListView {
        model: postModel
        delegate: Text { text: title + " (#" + remoteId + ")" }
    }
\endcode
 */
class QiListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit QiListModel(QObject *parent = nullptr);

    /// Replace the exposed records; roles are (re)derived from their fields
    void setList(const QiSharedList &list);

    /// Typed convenience overload
    template <typename T>
    void setList(const QiList<T> &list) {
        setList(static_cast<const QiSharedList &>(list));
    }

    /// The records currently exposed
    QiSharedList list() const;

    /// Number of rows
    int count() const;

    // --- QAbstractItemModel ---
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    void rebuildRoles();

    QiSharedList m_list;
    QiModelMetaInfo *m_metaInfo;
    QHash<int, QByteArray> m_roleNames;   // role id  -> field name (bytes, for QML)
    QHash<int, QString>    m_roleFields;  // role id  -> field name (string, for lookup)
};

#endif // QiLISTMODEL_H
