#pragma once

#include <QObject>
#include <QQmlEngine>
#include <qilistmodel.h>
#include "models.h"

class Store : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *tasks READ tasks CONSTANT)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    Store(QObject *parent = nullptr);

    QAbstractItemModel *tasks() { return &m_taskModel; }
    QString status() const { return m_status; }

    Q_INVOKABLE void loadTasks();
    Q_INVOKABLE void addTask(const QString &title, int priority = 1);
    Q_INVOKABLE void removeTask(int taskId);
    Q_INVOKABLE void toggleTask(int taskId);

signals:
    void statusChanged();

private:
    void updateStatus();

    QiListModel m_taskModel;
    QString m_status;
    QiConnection m_connection;
};
