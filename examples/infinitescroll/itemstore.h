#ifndef ITEMSTORE_H
#define ITEMSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <qilistmodel.h>

/// Exposes a lazily-paged model of Items to QML for infinite scroll.
class ItemStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *items READ items CONSTANT)
    Q_PROPERTY(int pageSize READ pageSize CONSTANT)
public:
    explicit ItemStore(QObject *parent = nullptr);
    QAbstractItemModel *items();
    int pageSize() const { return kPageSize; }

private:
    static constexpr int kPageSize = 24;
    QiLazyListModel m_model;
};

#endif // ITEMSTORE_H
