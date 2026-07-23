#ifndef QiASYNC_H
#define QiASYNC_H

#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QSharedPointer>
#include <QString>
#include <atomic>
#include <utility>
#include <qiconnection.h>
#include <qiconnectionpool.h>

/// A cooperative cancellation token for asynchronous work.
/**
  QtConcurrent futures can't be interrupted, so cancellation is cooperative: the
  work checks `isCanceled()` at safe points and returns early. Copies share the
  same flag, so cancelling the caller's token is visible to the running work.
 */
class QiCancelToken {
public:
    QiCancelToken() : m_flag(QSharedPointer<std::atomic<bool>>::create(false)) {}

    /// Request cancellation (thread-safe).
    void cancel() { m_flag->store(true); }

    /// True once cancel() has been called (check this inside the work).
    bool isCanceled() const { return m_flag->load(); }

private:
    QSharedPointer<std::atomic<bool>> m_flag;
};

/// Run Qivot database work on a background thread, returning a QFuture.
/**
  A big query — or a deep page fetch — runs synchronously on the calling thread
  and blocks it. QiAsync moves that work onto Qt's global thread pool and hands
  it back as a QFuture, so a UI thread stays responsive. Each worker thread gets
  its own connection (via a QiConnectionPool) to the same database file.

  Requirements:
    - `QT += concurrent` in any translation unit that includes this.
    - A **file-based** database (a `:memory:` database is private per connection).
    - This header is intentionally NOT part of `<qivot.h>`, so the core library
      never pulls in QtConcurrent; include `<qiasync.h>` explicitly.

\code
    QiAsync::configure("QSQLITE", "app.db");            // once, at startup

    QFuture<QiList<Event>> f = QiAsync::run([](QiConnection &c) {
        return Event::objects(c).orderBy(Event::col().id.asc()).limit(1000).all();
    });

    // cancellable long job:
    QiCancelToken token;
    QFuture<int> g = QiAsync::runCancelable(token, [](QiConnection &c, const QiCancelToken &t) {
        int n = 0;
        for (auto &row : bigThing) { if (t.isCanceled()) break; ... n++; }
        return n;
    });
    // later, from the UI thread:
    token.cancel();
\endcode
 */
class QiAsync {
public:
    /// Tell QiAsync which database worker threads should open (driver + file).
    static void configure(const QString &driver, const QString &databaseName) {
        pool().configure(driver, databaseName);
    }

    /// The shared connection pool (one connection per worker thread).
    static QiConnectionPool &pool() {
        static QiConnectionPool p;
        return p;
    }

    /// Run `work(conn)` on a worker thread; returns its result as a QFuture.
    template <typename Fn>
    static auto run(Fn work) -> QFuture<decltype(work(std::declval<QiConnection &>()))> {
        return QtConcurrent::run([work]() {
            QiConnection conn = QiAsync::pool().connection();
            return work(conn);
        });
    }

    /// Run cancellable `work(conn, token)` on a worker thread.
    template <typename Fn>
    static auto runCancelable(QiCancelToken token, Fn work)
        -> QFuture<decltype(work(std::declval<QiConnection &>(),
                                 std::declval<const QiCancelToken &>()))> {
        return QtConcurrent::run([work, token]() {
            QiConnection conn = QiAsync::pool().connection();
            return work(conn, token);
        });
    }
};

#endif // QiASYNC_H
