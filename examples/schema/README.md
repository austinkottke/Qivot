# Tutorial — Qivot's expressive schema layer

One runnable console program that touches every schema feature that makes Qivot
more than a toy ORM: string primary keys, `WITHOUT ROWID`, `CHECK` constraints,
enum fields, cascading foreign keys to a *string* key, composite keys, reverse
relations, bulk updates, and column migrations.

> **Run it**
> ```sh
> cd examples/schema
> qmake && make
> ./schema
> ```
> It runs against an in-memory database and prints the generated DDL + results.

---

## Step 1 — A string-keyed model, no auto `id`

`QI_DECLARE_MODEL_NOID` omits the automatic integer `id`; mark your own column
`QiPrimary`. `qiWithoutRowid<Customer>()` makes it a `WITHOUT ROWID` table.

```cpp
class Customer : public QiModel {
    QI_MODEL
public:
    QiField<QString> customerId;
    QiField<QString> name;
};
QI_DECLARE_MODEL_NOID(Customer, "customer",
                      QI_FIELD(customerId, QiPrimary | QiNotNull),
                      QI_FIELD(name));
// ...
qiWithoutRowid<Customer>();
```

## Step 2 — Foreign keys, enums, and CHECK

`QiForeignKey<Customer, QiFkCascade>` references the customer's *string* key and
cascades on delete. An `enum` field is stored as an integer. `QiCheck(...)` adds
a column `CHECK`.

```cpp
enum OrderStatus { Pending = 0, Shipped = 1, Delivered = 2 };

class Order : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Customer, QiFkCascade> customer;
    QiField<OrderStatus>                status;   // stored as INTEGER
    QiField<int>                        total;    // cents
    QiField<QString>                    note;
};
QI_DECLARE_MODEL(Order, "orders",
                 QI_FIELD(customer),
                 QI_FIELD(status),
                 QI_FIELD(total, QiCheck("total >= 0")),
                 QI_FIELD(note));
```

## Step 3 — Composite primary key

Key `OrderItem` on the `(orderId, sku)` pair — two `QiPrimary` columns.

```cpp
QI_DECLARE_MODEL_NOID(OrderItem, "order_item",
                      QI_FIELD(orderId, QiPrimary | QiNotNull),
                      QI_FIELD(sku,     QiPrimary | QiNotNull),
                      QI_FIELD(qty,     QiCheck("qty > 0")));
```

## Step 4 — Watch the generated DDL

`QiLog::enableAll()` prints every statement. `createTables()` produces exactly
what you declared:

```sql
CREATE TABLE IF NOT EXISTS customer (
    customerId TEXT NOT NULL PRIMARY KEY, name TEXT ) WITHOUT ROWID;

CREATE TABLE IF NOT EXISTS orders (
    id INTEGER PRIMARY KEY AUTOINCREMENT, customer INTEGER, status INTEGER,
    total INTEGER CHECK (total >= 0), note TEXT,
    FOREIGN KEY(customer) REFERENCES customer(customerId) ON DELETE CASCADE );

CREATE TABLE IF NOT EXISTS order_item (
    orderId INTEGER NOT NULL, sku TEXT NOT NULL, qty INTEGER CHECK (qty > 0),
    PRIMARY KEY (orderId, sku) );
```

## Step 5 — Use every feature

The rest of `main.cpp` exercises them and prints the results:

```text
================== Ada's orders (qiHasMany, one query) ==================
  order 1 total 1200 cents
  order 2 total 3400 cents

================== Ship all pending orders (bulk UPDATE) ==================
  marked 3 orders as Shipped
  Shipped now: 3

================== CHECK rejects an invalid row ==================
  refused negative total: CHECK constraint failed: total >= 0

================== Composite key REPLACE ==================
  order_item rows: 1 (still 1)

================== Delete a customer -> their orders cascade away ==================
  orders before: 3
  orders after removing Ada: 1

================== Migrate: rename + drop a column ==================
  orders columns now: id, customer, status, amount
```

Feature by feature:

- **Reverse relation** — `qiHasMany<Order>(ada, "customer").all()` fetches all of
  Ada's orders in one query.
- **Bulk update** — `Order::objects().filter(status == Pending).update({{"status", Shipped}})`
  issues a single `UPDATE … SET`.
- **CHECK** — saving a negative total is refused; the reason is in `lastError()`.
- **Composite REPLACE** — re-saving the same `(orderId, sku)` updates in place.
- **ON DELETE CASCADE** — removing Ada deletes her orders automatically.
- **Migrations** — `renameColumn<Order>("total","amount")` and `dropColumn<Order>("note")`.

---

## Files

| File | Role |
|---|---|
| `main.cpp` | All three models + a scripted tour of every schema feature. |

## See also

- [`jsonnested`](../jsonnested) — map nested JSON into a graph of related models.
- The [main README](../../README.md) has reference docs for each of these features.
