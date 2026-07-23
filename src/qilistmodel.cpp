#include <QTimer>
#include <QPointer>
#include "qilistmodel.h"
#include "qiabstractmodel.h"

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
    if (m_query)
        setList(m_query());
}

void QiListModel::setList(const QiSharedList &list) {
    beginResetModel();

    m_list = list;

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
    return m_list.size();
}

int QiListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_list.size();
}

QVariant QiListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_list.size())
        return QVariant();
    if (!m_metaInfo || !m_roleFields.contains(role))
        return QVariant();

    QiAbstractModel *model = m_list.at(index.row());
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
