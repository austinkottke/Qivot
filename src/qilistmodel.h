#ifndef QiLISTMODEL_H
#define QiLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QStringList>
#include <QVector>
#include <QMutex>
#include <QSet>
#include <functional>
#include <qisharedlist.h>
#include <qilist.h>
#include <qiquery.h>
#include <qimodelmetainfo.h>
#include <qiconnection.h>

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
    ~QiListModel() override;

    /// Replace the exposed records; roles are (re)derived from their fields
    void setList(const QiSharedList &list);

    /// Typed convenience overload
    template <typename T>
    void setList(const QiList<T> &list) {
        setList(static_cast<const QiSharedList &>(list));
    }

    // --- Reactive: auto-refresh when the data changes -----------------------

    /// Make the model **live**: run `query` now, and re-run it (coalesced to the
    /// event loop) whenever any of `tables` changes on `connection` — so a bound
    /// ListView updates by itself after any save / remove / update, from
    /// anywhere. Pass empty `tables` to react to every table.
    /** The new result is compared with the old one by id, and only the rows
        that differ are inserted, removed, moved or changed — the view keeps its
        delegates and scroll position. That still re-runs the whole query; for
        thousands of rows use QiLiveListModel, which doesn't. */
    void setLive(QiConnection connection, const QStringList &tables,
                 std::function<QiSharedList()> query);

    /// Typed convenience: watch T's table and refresh from `query`.
    /**
\code
    model->setLive<Task>(connection, []{ return Task::objects().orderBy("id desc").all(); });
\endcode
     */
    template <typename T>
    void setLive(QiConnection connection, std::function<QiList<T>()> query) {
        setLive(connection, QStringList{ T::TableName() },
                [query]() -> QiSharedList { return query(); });
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
    void scheduleRefresh();
    void refreshNow();
    bool applyDifferences(const QiSharedList &list);

    QiSharedList m_list;
    QVector<QiAbstractModel *> m_rows;    // what the view sees, row by row
    QiModelMetaInfo *m_metaInfo;
    QHash<int, QByteArray> m_roleNames;   // role id  -> field name (bytes, for QML)
    QHash<int, QString>    m_roleFields;  // role id  -> field name (string, for lookup)

    // live mode
    QiConnection m_liveConn;
    int          m_hookId = -1;
    QStringList  m_watch;
    std::function<QiSharedList()> m_query;
    bool         m_refreshPending = false;
};


/// A lazily-paged list model for DB-backed **infinite scroll**.
/**
  Loads records a page at a time through `canFetchMore()` / `fetchMore()`, so a
  QML `ListView` (or any item view) pulls the next `LIMIT`/`OFFSET` page as the
  user scrolls — instead of loading the whole table up front.

\code
    auto *model = new QiLazyListModel(this);
    model->setQuery( Item::objects().orderBy("id asc"), 50 );   // 50 rows / page
    model->reset();                                             // load the first page
\endcode

\code
    // QML — more pages arrive automatically as you scroll:
    ListView { model: itemModel; delegate: Text { text: name } }
\endcode

  Use a stable `orderBy` so paging is consistent.
 */
class QiLazyListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int  count READ count NOTIFY countChanged)
    Q_PROPERTY(bool atEnd READ atEnd NOTIFY atEndChanged)
public:
    explicit QiLazyListModel(QObject *parent = nullptr);

    /// Low-level: fetch(limit, offset) returns one page of records.
    void setFetcher(int pageSize, std::function<QiSharedList(int limit, int offset)> fetch);

    /// Typed convenience: page through an ordered query, `pageSize` rows at a time.
    template <typename T>
    void setQuery(QiQuery<T> query, int pageSize = 50) {
        setFetcher(pageSize, [query](int limit, int offset) -> QiSharedList {
            return QiQuery<T>(query).limit(limit).offset(offset).all();
        });
    }

    /// Clear and load the first page. Call after setQuery() / setFetcher().
    Q_INVOKABLE void reset();

    int  count() const;   ///< rows loaded so far
    bool atEnd() const;   ///< true once a short/empty page proved there's no more

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

signals:
    void countChanged();
    void atEndChanged();

private:
    void ensureRoles(QiModelMetaInfo *metaInfo);

    std::function<QiSharedList(int, int)> m_fetch;
    QVector<QiSharedList> m_pages;     // each page keeps its own records alive
    int  m_total    = 0;
    int  m_pageSize = 50;
    int  m_offset   = 0;
    bool m_atEnd    = false;
    QiModelMetaInfo *m_metaInfo = nullptr;
    QHash<int, QByteArray> m_roleNames;
    QHash<int, QString>    m_roleFields;
};


/// A **windowed** list model: presents the full row count, but only ever holds
/// the pages you're actually looking at.
/**
  Unlike QiLazyListModel (which appends pages and grows), QiWindowedListModel
  knows the total up front (one `SELECT count(*)`) and fetches each `LIMIT`/
  `OFFSET` page **on demand** as the view asks for rows — caching a bounded
  number of recent pages and evicting the rest. So `rowCount()` is the true
  total (scrollbars, section indices and jump-to-row all work), yet a 100k-row
  table never lives in memory: only a handful of pages do.

\code
    auto *model = new QiWindowedListModel(this);
    model->setQuery( Contact::objects().orderBy("lastName asc"), 60 );
    model->refresh();                 // counts, loads the first page
    // jump anywhere — the target page is fetched when the view scrolls to it:
    view->positionViewAtIndex(8123, ...);
\endcode
 */
class QiWindowedListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit QiWindowedListModel(QObject *parent = nullptr);

    /// Low-level: a total-count function and a page fetch(limit, offset).
    void setSource(int pageSize,
                   std::function<int()> countFn,
                   std::function<QiSharedList(int limit, int offset)> fetchFn);

    /// Typed convenience: window over an ordered (optionally filtered) query.
    template <typename T>
    void setQuery(QiQuery<T> query, int pageSize = 60) {
        setSource(pageSize,
            [query]() -> int { return QiQuery<T>(query).count(); },
            [query](int limit, int offset) -> QiSharedList {
                return QiQuery<T>(query).limit(limit).offset(offset).all();
            });
    }

    /// Re-count and drop cached pages (call after the query/filter changes).
    Q_INVOKABLE void refresh();

    /// The total number of rows (from `count(*)`), not just what's cached.
    int count() const;

    /// Field value at an arbitrary row, fetching its page if needed. Handy for
    /// section headers / jump HUDs without materialising the whole list.
    Q_INVOKABLE QVariant valueAt(int row, const QString &field) const;

    /// How many pages to keep cached before evicting the oldest (default 24).
    void setMaxCachedPages(int pages) { m_maxPages = pages > 1 ? pages : 1; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    QiSharedList pageFor(int row) const;      // fetch+cache the page holding `row`
    void setRoles(QiModelMetaInfo *metaInfo);

    int m_pageSize = 60;
    int m_total    = 0;
    int m_maxPages = 24;
    std::function<int()> m_count;
    std::function<QiSharedList(int, int)> m_fetch;

    mutable QHash<int, QiSharedList> m_cache;  // page index -> that page's records
    mutable QVector<int>             m_lru;    // page load/use order for eviction

    QiModelMetaInfo *m_metaInfo = nullptr;
    QHash<int, QByteArray> m_roleNames;
    QHash<int, QString>    m_roleFields;
};

/// A live list for **large** results: thousands of rows, kept up to date one
/// row at a time, like iOS's NSFetchedResultsController.
/**
  It holds the ids of the matching records in order, and nothing else until
  the view asks: records are read `batchSize` at a time, by id, for the rows on
  screen, and the oldest batches are dropped again. When a record is saved or
  removed anywhere through the same connection, the model works out what that
  means for the list and emits just that row's signal:

  - removed: the row goes; no query at all.
  - saved: one query asks the database where the record now falls in the
    list's order (ROW_NUMBER()). The row is inserted there, moved there, or
    refreshed in place; or removed, if it no longer matches the filter.
  - a bulk write (QiQuery::update(), remove() on a query, a batch save): the
    id column is read again and compared, and only the differences are
    signalled.

  So with 10,000 contacts, renaming one costs one small query and one
  `dataChanged`, and a ListView keeps its delegates and its scroll position.

\code
    auto *contacts = new QiLiveListModel(this);
    contacts->setQuery( Contact::objects().filter(Contact::col().archived == false)
                                          .orderBy(QStringList{ "lastName", "firstName" }) );

    // anywhere later, through the same connection:
    ada.lastName = "Byron";
    ada.save();          // the list moves Ada's row to the B's: one rowsMoved
\endcode

\code
    // QML: the roles are the model's field names
    ListView { model: contacts; delegate: Text { text: lastName + ", " + firstName } }
\endcode

  Changes are applied on the next pass of the event loop, so several writes in
  a row are worked out together; call flush() to apply them straight away.

  Row positions need window functions: SQLite 3.25, MySQL 8, MariaDB 10.2,
  PostgreSQL or SQL Server. Where they're missing, and for queries with a
  limit, offset, DISTINCT or GROUP BY, every change re-reads the ids instead —
  still one column, and still only the differences reach the view.

  Writes made with raw SQL aren't seen; call
  `connection.notifyChanged("table")` or refresh() after them.
 */
class QiLiveListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    explicit QiLiveListModel(QObject *parent = nullptr);
    ~QiLiveListModel() override;

    /// Show the records of `query`, kept up to date. Reads the ids now.
    /** @param batchSize How many records to read at once as the view scrolls. */
    void setQuery(const QiSharedQuery &query, int batchSize = 100);

    /// Typed convenience overload
    template <typename T>
    void setQuery(QiQuery<T> query, int batchSize = 100) {
        setQuery(static_cast<const QiSharedQuery &>(query), batchSize);
    }

    /// Other tables whose changes can change this list (a joined table, say).
    /// A change to one of them re-reads the ids.
    void setWatchedTables(const QStringList &tables);

    /// Read the ids again and apply the differences (after a raw SQL write).
    Q_INVOKABLE void refresh();

    /// Apply the changes waiting for the event loop now.
    void flush();

    /// Number of rows: every matching record, whether it has been read or not
    int count() const;

    /// The id of the record at `row`
    Q_INVOKABLE QVariant idAt(int row) const;

    /// The row of the record with `id`, or -1
    Q_INVOKABLE int indexOf(const QVariant &id) const;

    /// One field of the record at `row`, reading its batch if needed
    Q_INVOKABLE QVariant valueAt(int row, const QString &field) const;

    /// The record at `row`, reading its batch if needed. The model owns it,
    /// and may drop it once it has scrolled far away: don't keep the pointer.
    QiAbstractModel *recordAt(int row) const;

    /// The records kept in memory at most (default 2,000), in whole batches
    void setMaxCachedRecords(int records);

    /// How many records are in memory now
    int cachedRecords() const;

    /// How many queries the model has run since setQuery(): ids, positions
    /// and batches. Handy for tests and for seeing what a change cost.
    int queryCount() const { return m_queries; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void countChanged();

private:
    struct Cached {
        int batch;            // the batch it came in, for eviction
        QiAbstractModel *record;
    };

    void onChange(const QiChange &change);
    void schedule();
    void applyPending();
    bool applyRows(const QVector<QiChange> &changes);
    void applyIds(const QVariantList &ids, bool allValues, const QSet<QString> &touched = QSet<QString>());
    QiAbstractModel *load(int row) const;
    void forget(const QString &key) const;

    QiSharedQuery m_query;
    QiConnection  m_conn;
    QiModelMetaInfo *m_metaInfo = nullptr;
    QString       m_table;
    QStringList   m_watch;
    int           m_hookId = -1;
    int           m_batchSize = 100;
    int           m_maxCached = 2000;

    QVariantList  m_ids;        // the rows, in order
    QStringList   m_keys;       // the same ids as text, for comparing

    mutable QHash<QString, Cached>  m_records;   // id -> record, for the rows read so far
    mutable QList<QPair<int, QiSharedList>> m_batches;  // the lists that own them, oldest first
    mutable int   m_nextBatch = 0;
    mutable int   m_queries = 0;

    QMutex             m_pendingLock;
    QVector<QiChange>  m_pending;
    bool               m_scheduled = false;

    QHash<int, QByteArray> m_roleNames;
    QHash<int, QString>    m_roleFields;
};

#endif // QiLISTMODEL_H
