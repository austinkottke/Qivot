#ifndef QiASYNC_H
#define QiASYNC_H

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlDatabase>
#include <QString>
#include <utility>
#include <qiconnection.h>

/// Run Qivot database work on a background thread, returning a QFuture.
/**
  A big query — or a deep page fetch — runs synchronously on the calling thread
  and blocks it. QiAsync moves that work onto Qt's global thread pool and hands
  it back as a QFuture, so a UI thread stays responsive.

  SQLite requires a **separate connection per thread**, so QiAsync opens (and
  reuses) one connection per worker thread to the same database file, and passes
  your callable a QiConnection for that thread.

  Requirements:
    - `QT += concurrent` in the .pro of any translation unit that includes this.
    - A **file-based** database (a `:memory:` database is private per connection).
    - This header is intentionally NOT part of `<qivot.h>`, so the core library
      never pulls in QtConcurrent; include `<qiasync.h>` explicitly.

\code
    QiAsync::configure("QSQLITE", "app.db");            // once, at startup

    QFuture<QiList<Event>> f = QiAsync::run([](QiConnection &c) {
        return Event::objects(c).orderBy(Event::col().id.asc()).limit(1000).all();
    });

    // UI thread keeps running; collect the result via QFutureWatcher (or f.result()).
\endcode
 */
class QiAsync {
public:
    /// Tell QiAsync which database worker threads should open (driver + file).
    static void configure(const QString &driver, const QString &databaseName) {
        State &s = state();
        QMutexLocker lock(&s.mutex);
        s.driver = driver;
        s.database = databaseName;
    }

    /// Run `work(conn)` on a worker thread; returns its result as a QFuture.
    template <typename Fn>
    static auto run(Fn work) -> QFuture<decltype(work(std::declval<QiConnection &>()))> {
        return QtConcurrent::run([work]() {
            QiConnection conn = QiAsync::workerConnection();
            return work(conn);
        });
    }

private:
    struct State { QString driver; QString database; QMutex mutex; };
    static State &state() { static State s; return s; }

    // One connection per worker thread, opened once and cached. QtConcurrent
    // reuses pool threads, so the same physical thread reuses its connection —
    // the QSqlDatabase is created and used only on the thread that owns it.
    static QiConnection workerConnection() {
        static thread_local QiConnection *tls = nullptr;
        if (tls)
            return *tls;

        State &s = state();
        QString driver, database;
        {
            QMutexLocker lock(&s.mutex);
            driver = s.driver;
            database = s.database;
        }

        const QString name = QStringLiteral("qivot_async_%1")
                                 .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()));

        QSqlDatabase db;
        {
            // QSqlDatabase's connection registry isn't thread-safe to mutate.
            QMutexLocker lock(&s.mutex);
            db = QSqlDatabase::addDatabase(driver, name);
            db.setDatabaseName(database);
            db.open();
        }

        tls = new QiConnection();
        (void)tls->open(db, false);   // isolated worker connection, never the default
        return *tls;
    }
};

#endif // QiASYNC_H
