#ifndef QiJSONREQUEST_H
#define QiJSONREQUEST_H

#include <QThread>
#include <QUrl>
#include <QList>
#include <QPair>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QAtomicInt>
#include <qimodelmetainfo.h>
#include <qisharedlist.h>
#include <qiconnection.h>

/// Fetch JSON over HTTP on a worker thread and map it into DQuest models
/**
  QiJsonRequest performs an HTTP GET on a background thread, maps the returned
  JSON (an array of objects, or a single object) into models of a given type
  using QiJsonMapper, optionally persists them to the database on a per-thread
  connection, and delivers the result on the caller's thread via the loaded()
  signal (or failed() on error).

  Because the work runs on its own thread, the calling (e.g. GUI) thread is
  never blocked.

\code
    QiJsonRequest *req = qiJsonRequest<Post>();   // typed factory
    req->setConnection(connection);               // where to save (params are cloned per-thread)
    req->setRawHeader("Authorization", "Bearer <token>");

    QObject::connect(req, &QiJsonRequest::loaded, [](QiSharedList records){
        QiList<Post> posts = records;             // QiSharedList -> QiList<Post>
        qDebug() << "loaded" << posts.size() << "posts";
    });
    QObject::connect(req, &QiJsonRequest::failed, [](QString message){
        qWarning() << "request failed:" << message;
    });

    req->get(QUrl("https://example.com/api/posts"));
\endcode

  The request object deletes itself once finished (it is a one-shot). Do not use
  it after get() unless you keep your own reference and disable auto-deletion by
  reparenting it.

  @remarks Saving requires QSQLITE and an already-open primary connection whose
           database parameters are captured by setConnection().
 */
class QiJsonRequest : public QThread {
    Q_OBJECT
public:
    /// How fetched records are written to the database
    enum WriteMode {
        Replace,  ///< INSERT OR REPLACE by primary/unique key (the default)
        Upsert    ///< INSERT ... ON CONFLICT(keys) DO UPDATE — update in place
    };

    /// Construct a request that maps into the model described by @p metaInfo
    explicit QiJsonRequest(QiModelMetaInfo *metaInfo, QObject *parent = nullptr);

    ~QiJsonRequest() override;

    /// Add an HTTP request header (e.g. "Authorization"). Call before get().
    void setRawHeader(const QByteArray &name, const QByteArray &value);

    /// Set the database connection to persist mapped records into
    /**
      The connection's database parameters are captured immediately (on the
      calling thread) and re-opened on the worker thread with a private name, so
      the worker never touches the caller's QSqlDatabase. Setting a connection
      implicitly enables saving.
     */
    void setConnection(QiConnection connection);

    /// Enable or disable saving mapped records to the database
    void setSaveEnabled(bool enabled);

    /// Choose how records are written (Replace by default)
    void setWriteMode(WriteMode mode);

    /// Upsert fetched records on the given natural key(s)
    /**
      Sets the write mode to Upsert and the conflict column(s). On repeated
      fetches a record with a matching key is updated in place instead of
      being duplicated or replaced — ideal for mirroring a REST API keyed by,
      say, its own id column. Requires SQLite 3.24+.

\code
    req->setUpsertKeys({"remoteId"});   // update-or-insert by remoteId
\endcode
     */
    void setUpsertKeys(const QStringList &keys);

    /// Start the request on a worker thread
    /**
      Emits loaded() with the mapped records on success, or failed() with a
      message on error.
     */
    void get(const QUrl &url);

signals:
    /// Emitted on the caller's thread with the mapped (and optionally saved) records
    void loaded(QiSharedList records);

    /// Emitted on the caller's thread when the request fails
    void failed(QString message);

protected:
    void run() override;   // executes on the worker thread

private:
    QiModelMetaInfo *m_metaInfo;
    QUrl m_url;
    QList<QPair<QByteArray, QByteArray> > m_headers;
    bool m_save;
    WriteMode m_writeMode;
    QStringList m_upsertKeys;
    QString m_dbDriver;
    QString m_dbName;

    static QAtomicInt s_counter;
};

/// Typed factory: create a QiJsonRequest that maps into model T
template <typename T>
inline QiJsonRequest *qiJsonRequest(QObject *parent = nullptr) {
    return new QiJsonRequest(qiMetaInfo<T>(), parent);
}

#endif // QiJSONREQUEST_H
