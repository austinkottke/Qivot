#ifndef SANDBOXSTORE_H
#define SANDBOXSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QVector>
#include <QQmlEngine>          // QML_ELEMENT
#include <qilistmodel.h>
#include <qitransaction.h>

/// Controller for the nested-transaction sandbox.
/**
  Each "scope" is a QiTransaction. The first begins a real BEGIN; nested ones use
  SAVEPOINTs — so rolling back an inner scope undoes only the edits made since it
  opened, while the outer scope continues. Items you add/remove are written to the
  database immediately (inside the current scope); rolling back makes them vanish.
 */
class SandboxStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *items READ items CONSTANT)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)
public:
    explicit SandboxStore(QObject *parent = nullptr);
    ~SandboxStore() override;

    QAbstractItemModel *items();
    int depth() const { return m_stack.size(); }

    Q_INVOKABLE void addItem(const QString &label);
    Q_INVOKABLE void removeItem(int id);

    /// Open a nested scope (BEGIN for the first, SAVEPOINT for deeper ones).
    Q_INVOKABLE void beginScope();
    /// Roll back the innermost scope — undo edits made since it opened.
    Q_INVOKABLE void rollbackScope();
    /// Commit (release) the innermost scope, keeping its edits.
    Q_INVOKABLE void commitScope();

signals:
    void depthChanged();

private:
    void refresh();

    QiListModel m_model;
    QVector<QiTransaction *> m_stack;
};

#endif // SANDBOXSTORE_H
