#include <QTimer>
#include <QPointer>
#include <QSet>
#include <algorithm>
#include "qilistmodel.h"
#include "qiabstractmodel.h"
#include "qiqueryrules.h"
#include "qimodel.h"
#include "qimetainfoquery_p.h"

namespace {

/// The steps that turn one ordering of rows into another.
struct QiRowSteps {
    std::function<void(int first, int last)> remove;
    std::function<void(int from, int to)> move;        // `to`: the row's index afterwards
    std::function<void(int at, int count)> insert;     // the rows to[at .. at + count)
};

// Turns the rows `cur` into `to` (both lists of ids as text): removals, then
// moves, then insertions, each indexed against the rows as they are at that
// moment, so each can go straight into begin/end*Rows(). Moves are kept to a
// minimum: the longest run of rows already in the right order stays put.
// Returns false, doing nothing, when the lists hold duplicates or it would take
// more than `maxSteps` steps (a reset is cheaper then).
bool qiApplyRowSteps(QStringList cur, const QStringList &to, const QiRowSteps &steps, int maxSteps) {
    const QSet<QString> toSet(to.begin(), to.end());
    const QSet<QString> fromSet(cur.begin(), cur.end());
    if (toSet.size() != to.size() || fromSet.size() != cur.size())
        return false;

    // Runs of rows to remove, last first so the indexes stay valid.
    QVector<QPair<int, int>> removals;
    for (int i = cur.size() - 1; i >= 0; ) {
        if (toSet.contains(cur.at(i))) {
            --i;
            continue;
        }
        const int last = i;
        while (i >= 0 && !toSet.contains(cur.at(i)))
            --i;
        removals.append(qMakePair(i + 1, last));
    }

    // The rows in both, in their new order, and the longest run of them that's
    // already in that order among the rows that stay.
    QStringList common;
    QHash<QString, int> rank;
    for (const QString &key : to) {
        if (fromSet.contains(key)) {
            rank.insert(key, common.size());
            common << key;
        }
    }
    QStringList kept;
    for (const QString &key : cur) {
        if (toSet.contains(key))
            kept << key;
    }
    QVector<int> tails, tailAt, before(kept.size(), -1);
    for (int i = 0; i < kept.size(); i++) {
        const int r = rank.value(kept.at(i));
        const int pos = int(std::lower_bound(tails.begin(), tails.end(), r) - tails.begin());
        before[i] = pos > 0 ? tailAt.at(pos - 1) : -1;
        if (pos == tails.size()) {
            tails.append(r);
            tailAt.append(i);
        } else {
            tails[pos] = r;
            tailAt[pos] = i;
        }
    }
    QSet<QString> stable;
    for (int i = tailAt.isEmpty() ? -1 : tailAt.last(); i >= 0; i = before.at(i))
        stable.insert(kept.at(i));

    int insertions = 0;
    for (int i = 0; i < to.size(); ) {
        if (fromSet.contains(to.at(i))) {
            ++i;
            continue;
        }
        while (i < to.size() && !fromSet.contains(to.at(i)))
            ++i;
        insertions++;
    }

    if (removals.size() + (common.size() - stable.size()) + insertions > maxSteps)
        return false;

    for (const auto &run : removals) {
        steps.remove(run.first, run.second);
        cur.erase(cur.begin() + run.first, cur.begin() + run.second + 1);
    }

    // Each row that's out of order goes straight after the row that comes
    // before it in the new order, which is already in place.
    for (int k = 0; k < common.size(); k++) {
        const QString &key = common.at(k);
        if (stable.contains(key))
            continue;
        const int from = cur.indexOf(key);
        int dest = 0;
        if (k > 0) {
            const int prev = cur.indexOf(common.at(k - 1));
            dest = prev < from ? prev + 1 : prev;
        }
        if (dest != from) {
            steps.move(from, dest);
            cur.move(from, dest);
        }
    }

    for (int i = 0; i < to.size(); ) {
        if (fromSet.contains(to.at(i))) {
            ++i;
            continue;
        }
        const int start = i;
        while (i < to.size() && !fromSet.contains(to.at(i)))
            ++i;
        steps.insert(start, i - start);
        for (int x = start; x < i; x++)
            cur.insert(x, to.at(x));
    }
    return true;
}

QHash<int, QByteArray> qiRoleNames(QiModelMetaInfo *info, QHash<int, QString> *fields) {
    QHash<int, QByteArray> names;
    fields->clear();
    if (!info)
        return names;
    int role = Qt::UserRole + 1;
    for (const QString &field : info->fieldNameList()) {
        names.insert(role, field.toUtf8());
        fields->insert(role, field);
        role++;
    }
    return names;
}

} // namespace

QiListModel::QiListModel(QObject *parent)
    : QAbstractListModel(parent) , m_metaInfo(nullptr) {
}

QiListModel::~QiListModel() {
    if (m_hookId >= 0)
        m_liveConn.removeChangeHook(m_hookId);
}

void QiListModel::setLive(QiConnection connection, const QStringList &tables,
                          std::function<QiSharedList()> query) {
    if (m_hookId >= 0)
        m_liveConn.removeChangeHook(m_hookId);

    m_liveConn = connection;
    m_watch = tables;
    m_query = std::move(query);

    if (m_query)
        setList(m_query());            // initial load

    QPointer<QiListModel> self(this);
    m_hookId = m_liveConn.addChangeHook([self, this](const QString &table) {
        if (!self)
            return;                    // model gone
        if (m_watch.isEmpty() || m_watch.contains(table))
            scheduleRefresh();
    });
}

void QiListModel::scheduleRefresh() {
    if (m_refreshPending)
        return;                        // coalesce a burst of writes into one refresh
    m_refreshPending = true;
    QTimer::singleShot(0, this, [this]() { refreshNow(); });
}

void QiListModel::refreshNow() {
    m_refreshPending = false;
    if (!m_query)
        return;
    const QiSharedList list = m_query();
    if (!applyDifferences(list))
        setList(list);
}

// Moves the view from the rows it shows to `list` by id: only the rows that
// were added, removed, moved or edited are signalled. False when the records
// have no ids to compare by.
bool QiListModel::applyDifferences(const QiSharedList &list) {
    QiSharedList incoming = list;
    QiModelMetaInfo *info = incoming.metaInfo();
    if (!info && incoming.size() > 0 && incoming.at(0))
        info = incoming.at(0)->metaInfo();
    if (!m_metaInfo || (info && info != m_metaInfo))
        return false;
    const QStringList fields = m_metaInfo->fieldNameList();
    if (!fields.contains(QStringLiteral("id")))
        return false;

    auto keyOf = [this](QiAbstractModel *record) {
        return record ? m_metaInfo->value(record, QStringLiteral("id")).toString() : QString();
    };
    QStringList from, to;
    for (QiAbstractModel *record : m_rows)
        from << keyOf(record);
    QVector<QiAbstractModel *> rows;
    for (int i = 0; i < incoming.size(); i++) {
        rows << incoming.at(i);
        to << keyOf(rows.last());
    }
    if (from.contains(QString()) || to.contains(QString()))
        return false;

    const QiSharedList previous = m_list;   // keeps the old records alive meanwhile
    QiRowSteps steps;
    steps.remove = [this](int first, int last) {
        beginRemoveRows(QModelIndex(), first, last);
        m_rows.remove(first, last - first + 1);
        endRemoveRows();
    };
    steps.move = [this](int from, int to) {
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to);
        m_rows.move(from, to);
        endMoveRows();
    };
    steps.insert = [this, &rows](int at, int count) {
        beginInsertRows(QModelIndex(), at, at + count - 1);
        for (int x = at; x < at + count; x++)
            m_rows.insert(x, rows.at(x));
        endInsertRows();
    };
    const int before = m_rows.size();
    if (!qiApplyRowSteps(from, to, steps, 500))
        return false;

    // The rows are in the new order; switch each to its new record and say
    // which ones read differently now.
    m_list = incoming;
    int first = -1;
    for (int i = 0; i <= m_rows.size(); i++) {
        bool changed = false;
        if (i < m_rows.size()) {
            QiAbstractModel *old = m_rows.at(i);
            m_rows[i] = rows.at(i);
            if (old != rows.at(i)) {
                for (const QString &field : fields) {
                    if (m_metaInfo->value(old, field) != m_metaInfo->value(rows.at(i), field)) {
                        changed = true;
                        break;
                    }
                }
            }
        }
        if (changed && first < 0)
            first = i;
        if (!changed && first >= 0) {
            emit dataChanged(index(first), index(i - 1));
            first = -1;
        }
    }
    Q_UNUSED(previous);
    if (before != m_rows.size())
        emit countChanged();
    return true;
}

void QiListModel::setList(const QiSharedList &list) {
    beginResetModel();

    m_list = list;
    m_rows.clear();
    for (int i = 0; i < m_list.size(); i++)
        m_rows << m_list.at(i);

    // Determine the model's meta info: from the bound list, or the first record.
    m_metaInfo = m_list.metaInfo();
    if (!m_metaInfo && m_list.size() > 0) {
        QiAbstractModel *first = m_list.at(0);
        if (first)
            m_metaInfo = first->metaInfo();
    }

    rebuildRoles();

    endResetModel();
    emit countChanged();
}

void QiListModel::rebuildRoles() {
    m_roleNames.clear();
    m_roleFields.clear();

    if (!m_metaInfo)
        return;

    int role = Qt::UserRole + 1;
    const QStringList fields = m_metaInfo->fieldNameList();
    for (const QString &field : fields) {
        m_roleNames.insert(role, field.toUtf8());
        m_roleFields.insert(role, field);
        role++;
    }
}

QiSharedList QiListModel::list() const {
    return m_list;
}

int QiListModel::count() const {
    return m_rows.size();
}

int QiListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant QiListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    if (!m_metaInfo || !m_roleFields.contains(role))
        return QVariant();

    QiAbstractModel *model = m_rows.at(index.row());
    if (!model)
        return QVariant();

    return m_metaInfo->value(model, m_roleFields.value(role));
}

QHash<int, QByteArray> QiListModel::roleNames() const {
    return m_roleNames;
}


// --- QiLazyListModel: DB-backed infinite scroll ----------------------------

QiLazyListModel::QiLazyListModel(QObject *parent)
    : QAbstractListModel(parent) {
}

void QiLazyListModel::setFetcher(int pageSize, std::function<QiSharedList(int, int)> fetch) {
    m_pageSize = pageSize > 0 ? pageSize : 50;
    m_fetch = std::move(fetch);
}

void QiLazyListModel::reset() {
    beginResetModel();
    m_pages.clear();
    m_total = 0;
    m_offset = 0;
    m_atEnd = false;
    endResetModel();
    emit countChanged();
    emit atEndChanged();
    if (m_fetch)
        fetchMore(QModelIndex());     // eagerly load the first page
}

int QiLazyListModel::count() const { return m_total; }
bool QiLazyListModel::atEnd() const { return m_atEnd; }

int QiLazyListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_total;
}

QVariant QiLazyListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_total)
        return QVariant();
    if (!m_metaInfo || !m_roleFields.contains(role))
        return QVariant();

    int row = index.row();
    for (const QiSharedList &page : m_pages) {
        if (row < page.size()) {
            QiAbstractModel *m = page.at(row);
            return m ? m_metaInfo->value(m, m_roleFields.value(role)) : QVariant();
        }
        row -= page.size();
    }
    return QVariant();
}

QHash<int, QByteArray> QiLazyListModel::roleNames() const {
    return m_roleNames;
}

bool QiLazyListModel::canFetchMore(const QModelIndex &parent) const {
    if (parent.isValid())
        return false;
    return static_cast<bool>(m_fetch) && !m_atEnd;
}

void QiLazyListModel::fetchMore(const QModelIndex &parent) {
    if (parent.isValid() || !m_fetch || m_atEnd)
        return;

    QiSharedList page = m_fetch(m_pageSize, m_offset);
    const int n = page.size();
    if (n == 0) {
        m_atEnd = true;
        emit atEndChanged();
        return;
    }

    if (!m_metaInfo) {
        QiModelMetaInfo *info = page.metaInfo();
        if (!info && page.size() > 0 && page.at(0))
            info = page.at(0)->metaInfo();
        ensureRoles(info);
    }

    beginInsertRows(QModelIndex(), m_total, m_total + n - 1);
    m_pages.append(page);
    m_total  += n;
    m_offset += n;
    endInsertRows();

    if (n < m_pageSize) {
        m_atEnd = true;
        emit atEndChanged();
    }
    emit countChanged();
}

void QiLazyListModel::ensureRoles(QiModelMetaInfo *metaInfo) {
    m_metaInfo = metaInfo;
    m_roleNames.clear();
    m_roleFields.clear();
    if (!m_metaInfo)
        return;
    int role = Qt::UserRole + 1;
    const QStringList fields = m_metaInfo->fieldNameList();
    for (const QString &field : fields) {
        m_roleNames.insert(role, field.toUtf8());
        m_roleFields.insert(role, field);
        role++;
    }
}


// --- QiWindowedListModel: full count, pages fetched on demand ---------------

QiWindowedListModel::QiWindowedListModel(QObject *parent)
    : QAbstractListModel(parent) {
}

void QiWindowedListModel::setSource(int pageSize,
                                    std::function<int()> countFn,
                                    std::function<QiSharedList(int, int)> fetchFn) {
    m_pageSize = pageSize > 0 ? pageSize : 60;
    m_count = std::move(countFn);
    m_fetch = std::move(fetchFn);
}

void QiWindowedListModel::setRoles(QiModelMetaInfo *metaInfo) {
    m_metaInfo = metaInfo;
    m_roleNames.clear();
    m_roleFields.clear();
    if (!m_metaInfo)
        return;
    int role = Qt::UserRole + 1;
    const QStringList fields = m_metaInfo->fieldNameList();
    for (const QString &field : fields) {
        m_roleNames.insert(role, field.toUtf8());
        m_roleFields.insert(role, field);
        role++;
    }
}

void QiWindowedListModel::refresh() {
    beginResetModel();
    m_cache.clear();
    m_lru.clear();
    m_total = m_count ? m_count() : 0;

    // Prime page 0 so roles / meta are known before the view queries them.
    if (m_fetch && m_total > 0) {
        QiSharedList first = m_fetch(m_pageSize, 0);
        m_cache.insert(0, first);
        m_lru.append(0);
        QiModelMetaInfo *info = first.metaInfo();
        if (!info && first.size() > 0 && first.at(0))
            info = first.at(0)->metaInfo();
        setRoles(info);
    }
    endResetModel();
    emit countChanged();
}

int QiWindowedListModel::count() const { return m_total; }

int QiWindowedListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_total;
}

QiSharedList QiWindowedListModel::pageFor(int row) const {
    const int p = m_pageSize > 0 ? row / m_pageSize : 0;
    auto it = m_cache.find(p);
    if (it != m_cache.end()) {
        m_lru.removeAll(p);           // touch: most-recently used
        m_lru.append(p);
        return it.value();
    }
    QiSharedList page = m_fetch ? m_fetch(m_pageSize, p * m_pageSize) : QiSharedList();
    m_cache.insert(p, page);
    m_lru.append(p);
    while (m_lru.size() > m_maxPages) {   // evict the oldest page(s)
        const int victim = m_lru.takeFirst();
        m_cache.remove(victim);
    }
    return page;
}

QVariant QiWindowedListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_total)
        return QVariant();
    if (!m_metaInfo || !m_roleFields.contains(role))
        return QVariant();
    const QiSharedList page = pageFor(index.row());
    const int local = index.row() % m_pageSize;
    if (local < 0 || local >= page.size())
        return QVariant();
    QiAbstractModel *m = page.at(local);
    return m ? m_metaInfo->value(m, m_roleFields.value(role)) : QVariant();
}

QVariant QiWindowedListModel::valueAt(int row, const QString &field) const {
    if (row < 0 || row >= m_total)
        return QVariant();
    const QiSharedList page = pageFor(row);
    const int local = row % m_pageSize;
    if (local < 0 || local >= page.size())
        return QVariant();
    QiAbstractModel *m = page.at(local);
    if (!m || !m->metaInfo())
        return QVariant();
    return m->metaInfo()->value(m, field);
}

QHash<int, QByteArray> QiWindowedListModel::roleNames() const {
    return m_roleNames;
}


// --- QiLiveListModel: ids in memory, records by the batch, row-level updates -

QiLiveListModel::QiLiveListModel(QObject *parent)
    : QAbstractListModel(parent) {
}

QiLiveListModel::~QiLiveListModel() {
    if (m_hookId >= 0)
        m_conn.removeChangeHook(m_hookId);
}

void QiLiveListModel::setQuery(const QiSharedQuery &query, int batchSize) {
    if (m_hookId >= 0)
        m_conn.removeChangeHook(m_hookId);
    m_hookId = -1;

    m_query = query;
    m_conn = m_query.connection();
    QiQueryRules rules;
    rules = m_query;
    m_metaInfo = rules.metaInfo();
    m_table = m_metaInfo ? m_metaInfo->name() : QString();
    m_batchSize = batchSize > 0 ? batchSize : 100;
    m_queries = 0;
    {
        QMutexLocker lock(&m_pendingLock);
        m_pending.clear();
    }

    beginResetModel();
    m_roleNames = qiRoleNames(m_metaInfo, &m_roleFields);
    m_records.clear();
    m_batches.clear();
    m_ids.clear();
    m_keys.clear();
    if (m_metaInfo) {
        m_ids = m_query.ids();
        m_queries++;
        for (const QVariant &id : m_ids)
            m_keys << id.toString();
    }
    endResetModel();
    emit countChanged();

    if (!m_metaInfo)
        return;
    QPointer<QiLiveListModel> self(this);
    m_hookId = m_conn.addRowChangeHook([self, this](const QiChange &change) {
        if (self)
            onChange(change);
    });
}

void QiLiveListModel::setWatchedTables(const QStringList &tables) {
    m_watch = tables;
}

void QiLiveListModel::onChange(const QiChange &change) {
    if (change.table != m_table && !m_watch.contains(change.table))
        return;
    QiChange queued = change;
    if (change.table != m_table)
        queued.kind = QiChange::Many;      // another table: can't tell which rows
    {
        QMutexLocker lock(&m_pendingLock);
        m_pending.append(queued);
        if (m_scheduled)
            return;                        // several writes in a row: worked out together
        m_scheduled = true;
    }
    QMetaObject::invokeMethod(this, [this]() { applyPending(); }, Qt::QueuedConnection);
}

void QiLiveListModel::flush() {
    applyPending();
}

void QiLiveListModel::applyPending() {
    QVector<QiChange> changes;
    {
        QMutexLocker lock(&m_pendingLock);
        changes.swap(m_pending);
        m_scheduled = false;
    }
    if (changes.isEmpty())
        return;

    // A handful of records: place each one. A bulk write, or a lot at once:
    // read the ids again, which is one query however many rows changed.
    bool reread = changes.size() > 32;
    for (const QiChange &change : changes) {
        if (change.kind == QiChange::Many || change.id.isNull())
            reread = true;
    }
    if (!reread && applyRows(changes))
        return;
    refresh();
}

bool QiLiveListModel::applyRows(const QVector<QiChange> &changes) {
    // The last word on each record
    QHash<QString, QiChange> latest;
    QStringList order;
    for (const QiChange &change : changes) {
        const QString key = change.id.toString();
        if (!latest.contains(key))
            order << key;
        latest.insert(key, change);
    }

    // Take each changed record out of the list, ask where it belongs now, and
    // put them back in order of their new rows. The other rows haven't moved
    // relative to each other, so that is the new list.
    QVariantList ids = m_ids;
    QStringList keys = m_keys;
    QVector<QPair<int, QVariant>> placed;
    bool inserted = false;
    QSet<QString> touched;
    for (const QString &key : order) {
        const QiChange &change = latest.value(key);
        const int at = keys.indexOf(key);
        if (at >= 0) {
            keys.removeAt(at);
            ids.removeAt(at);
        }
        forget(key);
        if (change.kind == QiChange::Removed)
            continue;
        bool ok = false;
        const int row = m_query.rowOf(change.id, &ok);
        m_queries++;
        if (!ok)
            return false;
        if (row >= 0) {
            placed << qMakePair(row, change.id);
            touched.insert(key);
        }
        if (change.kind == QiChange::Inserted)
            inserted = true;
    }
    std::sort(placed.begin(), placed.end(), [](const QPair<int, QVariant> &a, const QPair<int, QVariant> &b) {
        return a.first < b.first;
    });
    for (const auto &p : placed) {
        if (p.first > keys.size())
            return false;                  // the list is out of step with the table
        keys.insert(p.first, p.second.toString());
        ids.insert(p.first, p.second);
    }

    // Something else may have changed the table meanwhile (a raw REPLACE that
    // removed a clashing row, another process): check the count after an insert.
    if (inserted) {
        QiSharedQuery counter(m_query);
        const int total = counter.count();
        m_queries++;
        if (total != ids.size())
            return false;
    }

    applyIds(ids, false, touched);
    return true;
}

void QiLiveListModel::refresh() {
    {
        QMutexLocker lock(&m_pendingLock);
        m_pending.clear();
    }
    if (!m_metaInfo)
        return;
    bool ok = false;
    const QVariantList ids = m_query.ids(&ok);
    m_queries++;
    if (ok)
        applyIds(ids, true);
}

void QiLiveListModel::applyIds(const QVariantList &ids, bool allValues, const QSet<QString> &touched) {
    QStringList keys;
    for (const QVariant &id : ids)
        keys << id.toString();

    QiRowSteps steps;
    steps.remove = [this](int first, int last) {
        beginRemoveRows(QModelIndex(), first, last);
        for (int i = last; i >= first; i--) {
            forget(m_keys.at(i));
            m_keys.removeAt(i);
            m_ids.removeAt(i);
        }
        endRemoveRows();
    };
    steps.move = [this](int from, int to) {
        beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to);
        m_keys.move(from, to);
        m_ids.move(from, to);
        endMoveRows();
    };
    steps.insert = [this, &ids, &keys](int at, int count) {
        beginInsertRows(QModelIndex(), at, at + count - 1);
        for (int x = at; x < at + count; x++) {
            m_keys.insert(x, keys.at(x));
            m_ids.insert(x, ids.at(x));
        }
        endInsertRows();
    };

    const int before = m_ids.size();
    if (!qiApplyRowSteps(m_keys, keys, steps, 256)) {
        beginResetModel();                 // too much moved: start again
        m_ids = ids;
        m_keys = keys;
        m_records.clear();
        m_batches.clear();
        endResetModel();
    } else if (allValues) {
        // A bulk write may have changed any record: drop what's in memory, and
        // the view reads its rows again (only the ones it is showing).
        m_records.clear();
        m_batches.clear();
        if (!m_ids.isEmpty())
            emit dataChanged(index(0), index(m_ids.size() - 1));
    } else {
        for (const QString &key : touched) {
            const int row = m_keys.indexOf(key);
            if (row >= 0)
                emit dataChanged(index(row), index(row));
        }
    }
    if (before != m_ids.size())
        emit countChanged();
}

void QiLiveListModel::forget(const QString &key) const {
    m_records.remove(key);
}

QiAbstractModel *QiLiveListModel::load(int row) const {
    if (row < 0 || row >= m_keys.size() || !m_metaInfo)
        return nullptr;
    const QString key = m_keys.at(row);
    auto found = m_records.constFind(key);
    if (found != m_records.constEnd())
        return found.value().record;

    // Read the rows around it that aren't in memory, in one query by id.
    const int start = row - row % m_batchSize;
    const int end = qMin(int(m_ids.size()), start + m_batchSize);
    QVariantList wanted;
    for (int i = start; i < end; i++) {
        if (!m_records.contains(m_keys.at(i)))
            wanted << m_ids.at(i);
    }
    _QiMetaInfoQuery query(m_metaInfo, m_conn);
    QiSharedList batch = query.filter(QiWhere(QStringLiteral("id")).in(wanted)).all();
    m_queries++;

    const int serial = m_nextBatch++;
    for (int i = 0; i < batch.size(); i++) {
        QiAbstractModel *record = batch.at(i);
        if (record)
            m_records.insert(m_metaInfo->value(record, QStringLiteral("id")).toString(), Cached{ serial, record });
    }
    m_batches.append(qMakePair(serial, batch));

    // Too much in memory: drop the oldest batches (never the one just read).
    while (m_records.size() > m_maxCached && m_batches.size() > 1) {
        const int victim = m_batches.first().first;
        for (auto it = m_records.begin(); it != m_records.end(); ) {
            if (it.value().batch == victim)
                it = m_records.erase(it);
            else
                ++it;
        }
        m_batches.removeFirst();
    }
    // Batches nothing points at any more (their records were all edited since)
    for (int b = m_batches.size() - 2; b >= 0 && m_batches.size() > 8; b--) {
        const int serialAt = m_batches.at(b).first;
        bool used = false;
        for (auto c = m_records.cbegin(); c != m_records.cend(); ++c) {
            if (c.value().batch == serialAt) { used = true; break; }
        }
        if (!used)
            m_batches.removeAt(b);
    }

    found = m_records.constFind(key);
    return found != m_records.constEnd() ? found.value().record : nullptr;
}

int QiLiveListModel::count() const {
    return m_ids.size();
}

QVariant QiLiveListModel::idAt(int row) const {
    return row >= 0 && row < m_ids.size() ? m_ids.at(row) : QVariant();
}

int QiLiveListModel::indexOf(const QVariant &id) const {
    return m_keys.indexOf(id.toString());
}

QVariant QiLiveListModel::valueAt(int row, const QString &field) const {
    QiAbstractModel *record = load(row);
    return record ? m_metaInfo->value(record, field) : QVariant();
}

QiAbstractModel *QiLiveListModel::recordAt(int row) const {
    return load(row);
}

void QiLiveListModel::setMaxCachedRecords(int records) {
    m_maxCached = records > 0 ? records : 1;
}

int QiLiveListModel::cachedRecords() const {
    return m_records.size();
}

int QiLiveListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_ids.size();
}

QVariant QiLiveListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || !m_roleFields.contains(role))
        return QVariant();
    return valueAt(index.row(), m_roleFields.value(role));
}

QHash<int, QByteArray> QiLiveListModel::roleNames() const {
    return m_roleNames;
}
