#include "itemstore.h"
#include "item.h"

ItemStore::ItemStore(QObject *parent) : QObject(parent) {
    // Page through the whole table, kPageSize rows at a time. reset() loads
    // page 1; the ListView pulls the rest as the user scrolls.
    m_model.setQuery( Item::objects().orderBy(Item::col().id.asc()), kPageSize );
    m_model.reset();
}

QAbstractItemModel *ItemStore::items() { return &m_model; }
