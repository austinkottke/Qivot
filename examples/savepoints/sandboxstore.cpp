#include "sandboxstore.h"
#include "models.h"

SandboxStore::SandboxStore(QObject *parent) : QObject(parent) {
    refresh();
}

SandboxStore::~SandboxStore() {
    // Roll back and drop any still-open scopes (RAII does the rollback).
    while (!m_stack.isEmpty())
        delete m_stack.takeLast();
}

QAbstractItemModel *SandboxStore::items() { return &m_model; }

void SandboxStore::refresh() {
    // Not a live model: we refresh explicitly, since a rollback isn't a write
    // and so doesn't fire a change notification.
    m_model.setList( Item::objects().orderBy(Item::col().id.asc()).all() );
}

void SandboxStore::addItem(const QString &label) {
    if (label.trimmed().isEmpty()) return;
    Item i; i.label = label.trimmed(); i.save();
    refresh();
}

void SandboxStore::removeItem(int id) {
    Item i;
    if (i.load(Item::col().id == id)) { (void)i.remove(); refresh(); }
}

void SandboxStore::beginScope() {
    m_stack.append(new QiTransaction());   // BEGIN or SAVEPOINT depending on depth
    emit depthChanged();
}

void SandboxStore::rollbackScope() {
    if (m_stack.isEmpty()) return;
    QiTransaction *t = m_stack.takeLast();
    t->rollback();                          // ROLLBACK or ROLLBACK TO savepoint
    delete t;
    emit depthChanged();
    refresh();                              // reverted rows reappear/vanish
}

void SandboxStore::commitScope() {
    if (m_stack.isEmpty()) return;
    QiTransaction *t = m_stack.takeLast();
    t->commit();                            // COMMIT or RELEASE savepoint
    delete t;
    emit depthChanged();
    refresh();
}
