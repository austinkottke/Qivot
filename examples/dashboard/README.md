# Tutorial — Query Playground

A step-by-step tour of Qivot queries in one small Qt Quick app. Each **step** is
one query, shown with its exact code and its **live result** — read it top to
bottom to learn the query API, from `count()` to window functions to running a
query off the UI thread.

<img src="screenshot.svg" alt="Numbered recipe cards, each with code and a live result" width="300">

*(stylized mockup — run it for all 8 live cards)*

> **Run it**
> ```sh
> cd examples/dashboard
> qmake && make
> ./dashboard
> ```

The data: 8 customers and 600 sales, seeded on launch.

---

## The 8 steps

Each card in the app is one line of code and its result:

| # | What | Code | Result |
|---|------|------|--------|
| 1 | Count rows | `Sale::objects().count()` | `600 sales` |
| 2 | Sum (aggregate) | `Sale::objects().call("sum", "amount")` | `$312,950` |
| 3 | Average | `Sale::objects().call("avg", "amount")` | `$522 per sale` |
| 4 | Filter | `Sale::objects().filter(Sale::col().amount > 900).count()` | `62 sales over $900` |
| 5 | Order + limit | `Sale::objects().orderBy(Sale::col().amount.desc()).limit(3).all()` | `$999, $998, $997` |
| 6 | One-to-many | `qiHasMany<Sale>(customer, "customer").count()` | `Ada made 75 sales` |
| 7 | Window function | `qiRawQuery<LeaderRow>("… RANK() OVER (ORDER BY total DESC) …")` | `#1 Ken Thompson — $39,425` |
| 8 | Off the UI thread | `QiAsync::run([](QiConnection &c){ return Sale::objects(c).count(); })` | `600 sales (worker thread)` |

Steps 1–7 run when the app opens. Step 8 has a **Run** button — it runs the same
count on a background thread (via `QiAsync` and a pooled connection) and fills in
the result when it lands, without blocking the UI.

## How it's built (it's tiny)

The controller builds a list of `{step, title, code, result}` — running each
query and stringifying its result:

```cpp
void DashboardStore::buildRecipes() {
    auto add = [this](int step, QString title, QString code, QString result) {
        m_recipes << QVariantMap{ {"step",step}, {"title",title}, {"code",code}, {"result",result} };
    };

    add(1, "Count every row",
        "Sale::objects().count()",
        QString("%1 sales").arg(Sale::objects().count()));

    add(7, "Rank with a window function",
        "qiRawQuery<LeaderRow>(\"… RANK() OVER (…) …\")",
        topCustomerLine());
    // …
}
```

Step 7's window-function query is the one thing the typed builder can't express,
so it uses `qiRawQuery<LeaderRow>` — raw SQL mapped back into a typed model:

```cpp
QiList<LeaderRow> ranked = qiRawQuery<LeaderRow>(
    "WITH t AS (SELECT customer AS cid, SUM(amount) AS total FROM sale GROUP BY customer) "
    "SELECT c.name AS name, t.total AS total, "
    "       RANK() OVER (ORDER BY t.total DESC) AS rnk "
    "FROM t JOIN customer c ON c.id = t.cid ORDER BY t.total DESC");
```

And the QML just lists the cards — a number badge, the title, the code in a mono
box, and the green result:

```qml
ListView {
    model: store.recipes
    delegate: Rectangle {
        Text { text: modelData.title }
        Text { text: modelData.code; font.family: "Menlo, monospace" }
        Text { text: "→ " + modelData.result; color: "#30D158" }
    }
}
```

## Files

| File | Role |
|---|---|
| `models.h` | `Customer`, `Sale`, and the `LeaderRow` view-model (for step 7). |
| `dashboardstore.h` / `.cpp` | Builds the 8 recipes; runs step 8 on a worker thread. |
| `main.cpp` | Opens a file DB, seeds customers + sales, loads the QML. |
| `main.qml` | The scrolling list of numbered recipe cards. |

## See also

- [`keyset`](../keyset), [`asyncquery`](../asyncquery) — the paging and async
  primitives on their own.
- [`prefetch`](../prefetch) — load a whole list's relations without N+1.
