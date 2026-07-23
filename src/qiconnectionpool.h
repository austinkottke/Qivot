#ifndef QiCONNECTIONPOOL_H
#define QiCONNECTIONPOOL_H

#include <QString>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <qiconnection.h>

/// A pool of database connections — one per thread.
/**
  SQLite requires a separate connection per thread. QiConnectionPool opens (and
  caches) one connection per calling thread to the same database file, so each
  worker thread gets an isolated, reusable QiConnection to the same data. It's
  what QiAsync uses under the hood; you can also use it directly for your own
  worker threads.

  Requires a **file-based** database (a `:memory:` database is private to each
  connection).

\code
    QiConnectionPool pool("QSQLITE", "app.db");
    // on any thread:
    QiConnection c = pool.connection();
    auto rows = User::objects(c).all();
\endcode
 */
class QiConnectionPool {
public:
    explicit QiConnectionPool(const QString &driver = QStringLiteral("QSQLITE"),
                              const QString &database = QString())
        : m_driver(driver), m_database(database) {}

    /// (Re)configure which database the pool's connections open.
    void configure(const QString &driver, const QString &database) {
        QMutexLocker lock(&m_mutex);
        m_driver = driver;
        m_database = database;
    }

    /// A QiConnection for the current thread (opened and cached on first use).
    QiConnection connection() {
        QString driver, database;
        {
            QMutexLocker lock(&m_mutex);
            driver = m_driver;
            database = m_database;
        }
        const QString name = connectionName();

        QSqlDatabase db;
        {
            // QSqlDatabase's registry isn't safe to mutate from many threads.
            QMutexLocker lock(&m_mutex);
            if (QSqlDatabase::contains(name)) {
                db = QSqlDatabase::database(name);
            } else {
                db = QSqlDatabase::addDatabase(driver, name);
                db.setDatabaseName(database);
                db.open();
            }
        }

        QiConnection conn;
        (void)conn.open(db, false);   // isolated worker connection, never the default
        return conn;
    }

private:
    // Unique per (pool instance, thread) so distinct pools never collide.
    QString connectionName() const {
        return QStringLiteral("qivot_pool_%1_%2")
            .arg(reinterpret_cast<quintptr>(this), 0, 16)
            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16);
    }

    QString m_driver;
    QString m_database;
    QMutex  m_mutex;
};

#endif // QiCONNECTIONPOOL_H
