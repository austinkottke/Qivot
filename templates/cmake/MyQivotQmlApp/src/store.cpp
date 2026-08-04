#include "store.h"
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QDir>

Store::Store(QObject *parent)
    : QObject(parent), m_status("Initializing...")
{
    // Set up database
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dataDir + "/app.db");

    if (!db.open()) {
        m_status = "Database error: " + db.lastError().text();
        emit statusChanged();
        return;
    }

    // Initialize connection
    m_connection.open(db);
    m_connection.addModel<Task>();

    if (!m_connection.createTables()) {
        m_status = "Schema error: " + m_connection.lastError().text();
        emit statusChanged();
        return;
    }

    // Set up live model
    m_taskModel.setLive<Task>(m_connection, [this]() {
        return Task::objects().orderBy(Task::col().id.desc()).all();
    });

    updateStatus();
}

void Store::loadTasks()
{
    m_taskModel.setList(Task::objects().orderBy(Task::col().id.desc()).all());
    updateStatus();
}

void Store::addTask(const QString &title, int priority)
{
    Task task;
    task.title = title;
    task.priority = priority;
    task.done = false;

    if (!task.save()) {
        m_status = "Error: " + task.lastError().text();
    } else {
        m_status = "Task added";
    }
    emit statusChanged();
}

void Store::removeTask(int taskId)
{
    Task task;
    if (task.load(Task::col().id == taskId)) {
        if (!task.remove()) {
            m_status = "Error: " + task.lastError().text();
        } else {
            m_status = "Task removed";
        }
    }
    emit statusChanged();
}

void Store::toggleTask(int taskId)
{
    Task task;
    if (task.load(Task::col().id == taskId)) {
        task.done = !task.done->toBool();
        if (!task.save()) {
            m_status = "Error: " + task.lastError().text();
        } else {
            m_status = "Task updated";
        }
    }
    emit statusChanged();
}

void Store::updateStatus()
{
    int count = Task::objects().count();
    m_status = QString("%1 task(s)").arg(count);
    emit statusChanged();
}
