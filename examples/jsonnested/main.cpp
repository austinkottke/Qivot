/** Nested JSON <-> a graph of related models.

    Qivot's QiJsonMapper (de)serializes one model at a time. A nested document —
    an order that embeds its customer and a list of line items — is handled by
    composing the mapper over the object graph: map the parent, then map each
    nested object / array, and wire the relations by id.

    Everything happens in memory; the graph is persisted only when we choose to.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <qivot.h>

// --- Models (related by id) ------------------------------------------------

class Customer : public QiModel {
    QI_MODEL
public:
    QiField<QString> customerId;
    QiField<QString> name;
};
QI_DECLARE_MODEL_NOID(Customer, "customer",
                      QI_FIELD(customerId, QiPrimary | QiNotNull),
                      QI_FIELD(name));

class LineItem : public QiModel {
    QI_MODEL
public:
    QiField<QString> orderRef;   // -> Order.orderId
    QiField<QString> sku;
    QiField<int>     qty;
};
QI_DECLARE_MODEL(LineItem, "line_item",
                 QI_FIELD(orderRef), QI_FIELD(sku), QI_FIELD(qty));

class Order : public QiModel {
    QI_MODEL
public:
    QiField<QString> orderId;
    QiField<QString> customerRef; // -> Customer.customerId
    QiField<int>     status;
};
QI_DECLARE_MODEL_NOID(Order, "orders",
                      QI_FIELD(orderId, QiPrimary | QiNotNull),
                      QI_FIELD(customerRef),
                      QI_FIELD(status));

// --- The in-memory graph ---------------------------------------------------

struct OrderGraph {
    Order            order;
    Customer         customer;
    QiList<LineItem> items;
};

// Deserialize a nested JSON document into the model graph (nothing persisted).
static OrderGraph orderFromJson(const QByteArray &bytes) {
    QJsonObject root = QJsonDocument::fromJson(bytes).object();

    OrderGraph g;
    g.order    = QiJsonMapper::map<Order>(root);                              // orderId, status
    g.customer = QiJsonMapper::map<Customer>(root.value("customer").toObject());
    g.items    = QiJsonMapper::map<LineItem>(root.value("items").toArray());

    // Wire the relations by id, in memory.
    g.order.customerRef = g.customer.customerId->toString();
    for (int i = 0; i < g.items.size(); i++)
        g.items.at(i)->orderRef = g.order.orderId->toString();

    return g;
}

// Serialize the graph back into one nested JSON document.
static QByteArray orderToJson(const OrderGraph &g) {
    QJsonObject root = QiJsonMapper::toJson(&g.order);
    root.remove("customerRef");                              // present the nested object instead
    root.insert("customer", QiJsonMapper::toJson(&g.customer));

    QJsonArray items;
    for (int i = 0; i < g.items.size(); i++) {
        QJsonObject item = QiJsonMapper::toJson(g.items.at(i));
        item.remove("orderRef");                             // implied by nesting
        item.remove("id");
        items.append(item);
    }
    root.insert("items", items);

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const QByteArray input = R"({
        "orderId": "ORD-1001",
        "status": 1,
        "customer": { "customerId": "CUST-ADA", "name": "Ada" },
        "items": [
            { "sku": "WIDGET", "qty": 2 },
            { "sku": "GADGET", "qty": 1 }
        ]
    })";

    // 1) Deserialize the nested document into a model graph — in memory only.
    OrderGraph g = orderFromJson(input);
    qDebug().noquote() << "Parsed graph (not saved):";
    qDebug().noquote() << "  order   " << g.order.orderId->toString()
                       << "status" << g.order.status->toInt();
    qDebug().noquote() << "  customer" << g.customer.customerId->toString()
                       << "-" << g.customer.name->toString();
    for (int i = 0; i < g.items.size(); i++)
        qDebug().noquote() << "  item    " << g.items.at(i)->sku->toString()
                           << "x" << g.items.at(i)->qty->toInt()
                           << " (orderRef" << g.items.at(i)->orderRef->toString() << ")";

    // 2) Serialize the graph back to nested JSON (round-trip).
    qDebug().noquote() << "\nRe-serialized:\n" << orderToJson(g);

    // 3) Persist the whole graph — on demand — with the relations intact.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();
    QiConnection conn; conn.open(db);
    conn.addModel<Customer>(); conn.addModel<Order>(); conn.addModel<LineItem>();
    conn.createTables();

    g.customer.save();          // parent rows first
    g.order.save();
    g.items.save();             // QiList saves every element

    qDebug().noquote() << "\nPersisted:"
                       << Customer::objects().count() << "customer,"
                       << Order::objects().count() << "order,"
                       << LineItem::objects().count() << "items";

    // 4) Reload from the DB and re-nest into JSON — proving the round trip.
    OrderGraph loaded;
    loaded.order.load( Order::col().orderId == "ORD-1001" );
    loaded.customer.load( Customer::col().customerId == loaded.order.customerRef->toString() );
    loaded.items = qiHasMany<LineItem>(loaded.order, "orderRef").all();

    qDebug().noquote() << "\nReloaded from DB, re-nested:\n" << orderToJson(loaded);
    return 0;
}
