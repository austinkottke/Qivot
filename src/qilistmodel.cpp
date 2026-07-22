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
