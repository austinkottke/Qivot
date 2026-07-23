#include "dashboardstore.h"
#include "models.h"
#include <QLocale>
#include <qiquery.h>          // qiRawQuery
#include <qirelation.h>       // qiHasMany
#include <qiasync.h>

static QString money(int v) {
    return "$" + QLocale(QLocale::English).toString(v);
}

DashboardStore::DashboardStore(QObject *parent) : QObject(parent) {
    connect(&m_watcher, &QFutureWatcherBase::finished, this, [this] {
        m_asyncResult = QString("%1 sales (counted on a worker thread)").arg(m_watcher.result());
        m_asyncBusy = false;
        emit asyncChanged();
    });
    buildRecipes();
}

void DashboardStore::buildRecipes() {
    auto add = [this](int step, const QString &title, const QString &code, const QString &result) {
        QVariantMap m;
        m["step"] = step; m["title"] = title; m["code"] = code; m["result"] = result;
        m_recipes << m;
    };

    // 1 — count
    add(1, "Count every row",
        "Sale::objects().count()",
        QString("%1 sales").arg(Sale::objects().count()));

    // 2 — sum (aggregate)
    add(2, "Sum with an aggregate",
        "Sale::objects().call(\"sum\", \"amount\")",
        money(Sale::objects().call("sum", "amount").toInt()) + " total revenue");

    // 3 — average
    add(3, "Average of a column",
        "Sale::objects().call(\"avg\", \"amount\")",
        money(qRound(Sale::objects().call("avg", "amount").toDouble())) + " per sale");

    // 4 — filter with a typed WHERE
    add(4, "Filter with a typed condition",
        "Sale::objects().filter(Sale::col().amount > 900).count()",
        QString("%1 sales over $900")
            .arg(Sale::objects().filter(Sale::col().amount > 900).count()));

    // 5 — order + limit
    QiList<Sale> top = Sale::objects().orderBy(Sale::col().amount.desc()).limit(3).all();
    QStringList tops;
    for (int i = 0; i < top.size(); i++) tops << money(top.at(i)->amount->toInt());
    add(5, "Order, then take the top 3",
        "Sale::objects().orderBy(Sale::col().amount.desc()).limit(3).all()",
        tops.join(", "));

    // 6 — a relation (one-to-many)
    Customer c;
    int cn = 0;
    QString cname = "—";
    if (c.load(Customer::col().id == 1)) {
        cname = c.name->toString();
        cn = qiHasMany<Sale>(c, "customer").count();
    }
    add(6, "A customer's sales (one-to-many)",
        "qiHasMany<Sale>(customer, \"customer\").count()",
        QString("%1 made %2 sales").arg(cname).arg(cn));

    // 7 — window function via raw SQL, mapped to typed rows
    QiList<LeaderRow> ranked = qiRawQuery<LeaderRow>(
        "WITH t AS (SELECT customer AS cid, SUM(amount) AS total FROM sale GROUP BY customer) "
        "SELECT c.name AS name, t.total AS total, "
        "       RANK() OVER (ORDER BY t.total DESC) AS rnk "
        "FROM t JOIN customer c ON c.id = t.cid ORDER BY t.total DESC LIMIT 1");
    QString topLine = ranked.size()
        ? QString("#1  %1 — %2").arg(ranked.at(0)->name->toString(), money(ranked.at(0)->total->toInt()))
        : "(none)";
    add(7, "Rank with a window function (raw SQL → typed rows)",
        "qiRawQuery<LeaderRow>(\"… RANK() OVER (ORDER BY total DESC) …\")",
        topLine);

    // 8 — async (its own card + button in the UI)
    add(8, "Run a query off the UI thread",
        "QiAsync::run([](QiConnection &c){ return Sale::objects(c).count(); })",
        "");
}

void DashboardStore::runAsync() {
    if (m_asyncBusy) return;
    m_asyncBusy = true; m_asyncResult = "working…"; emit asyncChanged();

    QiAsync::configure("QSQLITE", "dashboard.db");
    m_watcher.setFuture( QiAsync::run([](QiConnection &c) {
        return Sale::objects(c).count();     // runs on a worker thread, own connection
    }) );
}
