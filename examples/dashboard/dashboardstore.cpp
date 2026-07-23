#include "dashboardstore.h"
#include "models.h"
#include <QThread>
#include <qiquery.h>          // qiRawQuery

DashboardStore::DashboardStore(QObject *parent)
    : QObject(parent), m_dbName("dashboard.db") {

    // Poll the worker's shared progress counter on the GUI thread.
    m_poll.setInterval(50);
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (m_prog) { m_progress = m_prog->load(); emit progressChanged(); }
    });
    connect(&m_watcher, &QFutureWatcherBase::finished, this, &DashboardStore::onFinished);

    refresh();
}

void DashboardStore::refresh() {
    // Per-customer totals, ranked, with a running cumulative total — a CTE plus
    // window functions the query builder can't express, mapped to typed rows.
    static const char *SQL =
        "WITH totals AS ("
        "  SELECT customer AS cid, SUM(amount) AS total, COUNT(*) AS orders"
        "  FROM sale GROUP BY customer"
        ") "
        "SELECT c.name AS name, t.total AS total, t.orders AS orders, "
        "       RANK() OVER (ORDER BY t.total DESC) AS rnk, "
        "       SUM(t.total) OVER (ORDER BY t.total DESC "
        "                          ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) AS running "
        "FROM totals t JOIN customer c ON c.id = t.cid "
        "ORDER BY t.total DESC";

    QiList<LeaderRow> rows = qiRawQuery<LeaderRow>(SQL);

    m_leaderboard.clear();
    m_grandTotal = 0;
    for (int i = 0; i < rows.size(); i++) {
        LeaderRow *r = rows.at(i);
        QVariantMap m;
        m["name"]    = r->name->toString();
        m["total"]   = r->total->toInt();
        m["orders"]  = r->orders->toInt();
        m["rank"]    = r->rnk->toInt();
        m["running"] = r->running->toInt();
        m_leaderboard << m;
        m_grandTotal = qMax(m_grandTotal, r->running->toInt());   // last running == grand total
    }
    // add percent-of-total now that we know the grand total
    for (int i = 0; i < m_leaderboard.size(); i++) {
        QVariantMap m = m_leaderboard[i].toMap();
        m["pct"] = m_grandTotal > 0 ? (m["total"].toInt() * 100.0 / m_grandTotal) : 0.0;
        m_leaderboard[i] = m;
    }
    emit leaderboardChanged();
}

void DashboardStore::recompute() {
    if (m_busy) return;
    m_busy = true;       emit busyChanged();
    m_progress = 0;      emit progressChanged();
    m_status = "Recomputing…";  emit statusChanged();

    m_token = QiCancelToken();                                   // fresh token
    m_prog  = QSharedPointer<std::atomic<int>>::create(0);
    m_clock.restart();

    QiAsync::configure("QSQLITE", m_dbName);
    auto prog = m_prog;

    QFuture<int> f = QiAsync::runCancelable(m_token,
        [prog](QiConnection &c, const QiCancelToken &token) -> int {
        const int steps = 100;
        for (int i = 0; i < steps; i++) {
            if (token.isCanceled())
                return -1;                                       // cooperative cancel
            // real per-step work on the worker's own connection, then a small
            // pause so the heavy job is observable.
            (void) Sale::objects(c).call("sum", "amount");
            QThread::msleep(25);
            prog->store((i + 1) * 100 / steps);
        }
        return steps;
    });

    m_watcher.setFuture(f);
    m_poll.start();
}

void DashboardStore::cancel() {
    if (m_busy) {
        m_token.cancel();
        m_status = "Cancelling…"; emit statusChanged();
    }
}

void DashboardStore::onFinished() {
    m_poll.stop();
    const int result = m_watcher.result();
    m_busy = false; emit busyChanged();

    if (result < 0) {
        m_status = QString("Cancelled at %1%").arg(m_progress);
    } else {
        m_progress = 100; emit progressChanged();
        m_status = QString("Recomputed on a worker thread in %1 ms").arg(m_clock.elapsed());
        refresh();   // refresh the leaderboard from the (unchanged) data
    }
    emit statusChanged();
}
