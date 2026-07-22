#ifndef POSTSTORE_H
#define POSTSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>          // QML_ELEMENT
#include <qilistmodel.h>

/// A QML-registered controller that exposes Qivot posts to QML.
/**
  Instead of pushing objects into QML with setContextProperty, this type is
  registered declaratively (QML_ELEMENT) so QML can `import Qivot` and create it:

\code
    import Qivot
    PostStore { id: store; Component.onCompleted: reload() }
\endcode

  It exposes a `posts` model (a QiListModel, for a ListView), a `status`
  property that QML binds to (updated via a NOTIFY signal), and invokable
  slots — `reload()`, `add()`, `search()` — that QML calls.
 */
class PostStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *posts READ posts CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit PostStore(QObject *parent = nullptr);

    QAbstractItemModel *posts();
    QString status() const;

    /// Load all posts, newest first
    Q_INVOKABLE void reload();

    /// Insert a new post (remoteId auto-assigned), then refresh
    Q_INVOKABLE void add(const QString &title, const QString &author);

    /// Delete the post with the given remoteId, then refresh
    Q_INVOKABLE void remove(int remoteId);

    /// Full-text search; empty text shows everything again
    Q_INVOKABLE void search(const QString &text);

signals:
    void statusChanged();

private:
    void setStatus(const QString &status);

    QiListModel m_model;
    QString     m_status;
};

#endif // POSTSTORE_H
