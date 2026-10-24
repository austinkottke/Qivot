#ifndef QiCONNECTIONPOOL_H
#define QiCONNECTIONPOOL_H

#include <QString>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QHash>
#include <QAtomicInteger>
#include <QCoreApplication>
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
    // The connections a thread has been given, one name per pool, for as long
    // as the thread lives. The name used to be the thread's native id, but ids
    // are reused once a thread ends (a QThreadPool retires idle threads, and
    // Android hands the id out again at once): a new thread then found the
    // finished one's connection under its name, and Qt refuses a connection
    // from another thread, so its queries failed. When a worker thread ends,
    // its connections are closed and removed with it.
    struct ThreadConnections {
        QHash<const QiConnectionPool *, QString> names;
        bool mainThread = false;
        ~ThreadConnections() {
            if (mainThread)
                return;     // process exit: Qt's connection registry may be gone already
            for (auto it = names.cbegin(); it != names.cend(); ++it)
                QSqlDatabase::removeDatabase(it.value());
        }
    };

    QString connectionName() const {
        static thread_local ThreadConnections thread;
        auto it = thread.names.constFind(this);
        if (it != thread.names.constEnd())
            return it.value();
        static QAtomicInteger<quint64> next(1);
        QCoreApplication *app = QCoreApplication::instance();
        thread.mainThread = app && QThread::currentThread() == app->thread();
        const QString name = QStringLiteral("qivot_pool_%1_%2")
            .arg(reinterpret_cast<quintptr>(this), 0, 16)
            .arg(next.fetchAndAddRelaxed(1));
        thread.names.insert(this, name);
        return name;
    }

    QString m_driver;
    QString m_database;
    QMutex  m_mutex;
};

#endif // QiCONNECTIONPOOL_H
