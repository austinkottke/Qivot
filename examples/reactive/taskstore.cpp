#include "taskstore.h"
#include "task.h"

TaskStore::TaskStore(QObject *parent) : QObject(parent) {
    // Bind the model to a live query: it re-runs (coalesced) whenever the
    // "task" table changes on the default connection.
    m_model.setLive<Task>(QiConnection::defaultConnection(), [] {
        return Task::objects().orderBy(Task::col().id.desc()).all();
    });
}

QAbstractItemModel *TaskStore::tasks() { return &m_model; }

void TaskStore::add(const QString &title) {
    const QString t = title.trimmed();
    if (t.isEmpty()) return;
    Task task; task.title = t; task.done = 0;
    task.save();                    // <-- that's it; the ListView updates itself
}

void TaskStore::toggle(int id) {
    Task task;
    if (task.load(Task::col().id == id)) {
        task.done = task.done->toInt() ? 0 : 1;
        task.save();
    }
}

void TaskStore::remove(int id) {
    Task task;
    if (task.load(Task::col().id == id))
        task.remove();
}
