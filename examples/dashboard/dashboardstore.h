#ifndef DASHBOARDSTORE_H
#define DASHBOARDSTORE_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>          // QML_ELEMENT
#include <QFutureWatcher>
#include <QTimer>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <atomic>
#include <qiasync.h>           // QiCancelToken

/// The controller behind the analytics dashboard.
/**
  - `leaderboard` is computed with a window-function query via qiRawQuery.
  - `recompute()` runs a deliberately heavy job on a worker thread (QiAsync +
    QiConnectionPool), reporting `progress` live and honouring `cancel()`
    through a QiCancelToken.
 */
class DashboardStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList leaderboard READ leaderboard NOTIFY leaderboardChanged)
    Q_PROPERTY(int   grandTotal READ grandTotal NOTIFY leaderboardChanged)
    Q_PROPERTY(bool  busy       READ busy       NOTIFY busyChanged)
    Q_PROPERTY(int   progress   READ progress   NOTIFY progressChanged)
    Q_PROPERTY(QString status   READ status     NOTIFY statusChanged)
public:
    explicit DashboardStore(QObject *parent = nullptr);

    QVariantList leaderboard() const { return m_leaderboard; }
    int grandTotal() const { return m_grandTotal; }
    bool busy() const { return m_busy; }
    int progress() const { return m_progress; }
    QString status() const { return m_status; }

    /// Recompute the leaderboard (fast, synchronous window-function query).
    Q_INVOKABLE void refresh();

    /// Run a heavy analytics job on a worker thread — cancellable, with progress.
    Q_INVOKABLE void recompute();

    /// Ask the running recompute() to stop.
    Q_INVOKABLE void cancel();

signals:
    void leaderboardChanged();
    void busyChanged();
    void progressChanged();
    void statusChanged();

private:
    void onFinished();

    QString      m_dbName;
    QVariantList m_leaderboard;
    int          m_grandTotal = 0;

    bool    m_busy = false;
    int     m_progress = 0;
    QString m_status;

    QiCancelToken                     m_token;
    QSharedPointer<std::atomic<int>>  m_prog;
    QFutureWatcher<int>               m_watcher;
    QTimer                            m_poll;
    QElapsedTimer                     m_clock;
};

#endif // DASHBOARDSTORE_H
