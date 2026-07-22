#ifndef TASKSTORE_H
#define TASKSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <qilistmodel.h>

/// A QML controller whose model is *live* — it never manually reloads.
/**
  The only thing add()/toggle()/remove() do is write to the database. Because the
  exposed QiListModel is bound with setLive(), any change to the "task" table —
  from here, from a background timer, from anywhere — refreshes the bound
  ListView automatically. That's Reactive Qivot.
 */
class TaskStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *tasks READ tasks CONSTANT)
public:
    explicit TaskStore(QObject *parent = nullptr);
    QAbstractItemModel *tasks();

    Q_INVOKABLE void add(const QString &title);   // just save() — no refresh call
    Q_INVOKABLE void toggle(int id);
    Q_INVOKABLE void remove(int id);

private:
    QiListModel m_model;
};

#endif // TASKSTORE_H
