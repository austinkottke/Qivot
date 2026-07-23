# Tutorial — nested JSON ↔ a graph of related models

`QiJsonMapper` maps *one* model at a time. This example shows how to handle a
**nested** document — an order that embeds its customer and a list of line
items — by composing the mapper over the object graph: map the parent, map each
nested object/array, then wire the relations by id. Nothing touches the database
until you choose to persist.

> **Run it**
> ```sh
> cd examples/jsonnested
> qmake && make
> ./jsonnested
> ```

---

## Step 1 — Models related by id

Three plain models. `Order` and `LineItem` carry reference fields
(`customerRef`, `orderRef`) that link the graph.

```cpp
QI_DECLARE_MODEL_NOID(Customer, "customer",
                      QI_FIELD(customerId, QiPrimary | QiNotNull), QI_FIELD(name));

QI_DECLARE_MODEL(LineItem, "line_item",
                 QI_FIELD(orderRef), QI_FIELD(sku), QI_FIELD(qty));

QI_DECLARE_MODEL_NOID(Order, "orders",
                      QI_FIELD(orderId, QiPrimary | QiNotNull),
                      QI_FIELD(customerRef), QI_FIELD(status));
```

## Step 2 — Deserialize the nested document into a graph

Map the root into an `Order`, the `"customer"` object into a `Customer`, and the
`"items"` array into a `QiList<LineItem>` — then wire the ids. All in memory.

```cpp
OrderGraph orderFromJson(const QByteArray &bytes) {
    QJsonObject root = QJsonDocument::fromJson(bytes).object();

    OrderGraph g;
    g.order    = QiJsonMapper::map<Order>(root);                                // orderId, status
    g.customer = QiJsonMapper::map<Customer>(root.value("customer").toObject());
    g.items    = QiJsonMapper::map<LineItem>(root.value("items").toArray());    // whole array

    g.order.customerRef = g.customer.customerId->toString();     // wire relations
    for (int i = 0; i < g.items.size(); i++)
        g.items.at(i)->orderRef = g.order.orderId->toString();
    return g;
}
```

Given this input:

```json
{
  "orderId": "ORD-1001",
  "status": 1,
  "customer": { "customerId": "CUST-ADA", "name": "Ada" },
  "items": [ { "sku": "WIDGET", "qty": 2 }, { "sku": "GADGET", "qty": 1 } ]
}
```

it prints the parsed graph (nothing saved yet):

```text
Parsed graph (not saved):
  order    ORD-1001 status 1
  customer CUST-ADA - Ada
  item     WIDGET x 2  (orderRef ORD-1001 )
  item     GADGET x 1  (orderRef ORD-1001 )
```

## Step 3 — Serialize the graph back to nested JSON

Compose `QiJsonMapper::toJson()` the other way: take the parent, drop the flat
reference fields, and nest the mapped objects.

```cpp
QByteArray orderToJson(const OrderGraph &g) {
    QJsonObject root = QiJsonMapper::toJson(&g.order);
    root.remove("customerRef");                          // nest the object instead
    root.insert("customer", QiJsonMapper::toJson(&g.customer));

    QJsonArray items;
    for (int i = 0; i < g.items.size(); i++) {
        QJsonObject item = QiJsonMapper::toJson(g.items.at(i));
        item.remove("orderRef"); item.remove("id");      // implied by nesting
        items.append(item);
    }
    root.insert("items", items);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}
```

## Step 4 — Persist on demand, then reload and re-nest

Save the parents first, then the `QiList` (which saves every element). Later,
reload from the DB — `qiHasMany<LineItem>(order, "orderRef").all()` re-fetches
the items — and re-nest to prove the round trip is lossless.

```cpp
g.customer.save();  g.order.save();  g.items.save();     // parents first

Order loaded;   loaded.load( Order::col().orderId == "ORD-1001" );
QiList<LineItem> items = qiHasMany<LineItem>(loaded, "orderRef").all();
```

```text
Persisted: 1 customer, 1 order, 2 items

Reloaded from DB, re-nested:
 {
    "customer": { "customerId": "CUST-ADA", "name": "Ada" },
    "items": [ { "qty": 2, "sku": "WIDGET" }, { "qty": 1, "sku": "GADGET" } ],
    "orderId": "ORD-1001",
    "status": 1
}
```

---

## Files

| File | Role |
|---|---|
| `main.cpp` | The models, the graph struct, and `orderFromJson` / `orderToJson`. |

## See also

- [`jsonhttp`](../jsonhttp) — the same mapper fed by a REST API fetched on a
  worker thread.
- [`schema`](../schema) — the relation + key features used here, in depth.
