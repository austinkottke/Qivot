/** Schema & relations showcase.

    One runnable program touching the features that make Qivot's schema layer
    expressive:
      - string primary keys with no auto id      (QI_DECLARE_MODEL_NOID + QiPrimary)
      - WITHOUT ROWID tables                      (qiWithoutRowid)
      - CHECK constraints                         (QiCheck)
      - enum fields stored as integers            (QiField<Enum>)
      - foreign keys with ON DELETE CASCADE that reference a string key
      - composite primary keys
      - reverse "has many" relations              (qiHasMany)
      - bulk UPDATE ... SET                        (QiQuery::update)
      - rename / drop column migrations

    Run it: SQL statements are printed by QiLog so you can see the generated DDL.
 */

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <qivot.h>

// --- Models ---------------------------------------------------------------

// A customer is keyed by a meaningful string id (e.g. a ULID) — no auto `id`.
class Customer : public QiModel {
    QI_MODEL
public:
    QiField<QString> customerId;
    QiField<QString> name;
};
QI_DECLARE_MODEL_NOID(Customer, "customer",
                      QI_FIELD(customerId, QiPrimary | QiNotNull),
                      QI_FIELD(name));

enum OrderStatus { Pending = 0, Shipped = 1, Delivered = 2 };

// An order has an auto id, a foreign key to the (string-keyed) customer that
// cascades on delete, an enum status, and a CHECK-constrained total.
class Order : public QiModel {
    QI_MODEL
public:
    QiForeignKey<Customer, QiFkCascade> customer;
    QiField<OrderStatus>                status;
    QiField<int>                        total;   // in cents
    QiField<QString>                    note;
};
QI_DECLARE_MODEL(Order, "orders",
                 QI_FIELD(customer),
                 QI_FIELD(status),
                 QI_FIELD(total, QiCheck("total >= 0")),
                 QI_FIELD(note));

// A line item is identified by the (order, sku) pair — a composite key.
class OrderItem : public QiModel {
    QI_MODEL
public:
    QiField<int>     orderId;
    QiField<QString> sku;
    QiField<int>     qty;
};
QI_DECLARE_MODEL_NOID(OrderItem, "order_item",
                      QI_FIELD(orderId, QiPrimary | QiNotNull),
                      QI_FIELD(sku,     QiPrimary | QiNotNull),
                      QI_FIELD(qty,     QiCheck("qty > 0")));

Q_DECLARE_METATYPE(OrderStatus)

static void banner(const QString &title) {
    qDebug().noquote() << "\n==================" << title << "==================";
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();

    QiConnection conn;
    conn.open(db);
    conn.addModel<Customer>();
    conn.addModel<Order>();
    conn.addModel<OrderItem>();

    qiWithoutRowid<Customer>();          // customer table has no shadow rowid

    banner("CREATE TABLES (watch the generated DDL)");
    QiLog::enableAll();                  // show every statement Qivot runs
    QiLog::setCategories(QiLog::Sql);
    conn.createTables();
    QiLog::setEnabled(false);            // quieten down for the rest

    // --- seed -------------------------------------------------------------
    Customer ada;  ada.customerId  = "CUST-ADA";  ada.name  = "Ada";  ada.save();
    Customer bob;  bob.customerId  = "CUST-BOB";  bob.name  = "Bob";  bob.save();

    auto place = [](Customer &c, int total) {
        Order o; o.customer = c; o.status = Pending; o.total = total; o.note = "";
        o.save();
        return o.id->toInt();
    };
    int adaOrder1 = place(ada, 1200);
    (void)         place(ada, 3400);
    (void)         place(bob,  999);

    OrderItem it; it.orderId = adaOrder1; it.sku = "WIDGET"; it.qty = 2; it.save();

    // --- #6 reverse relation (has many) -----------------------------------
    banner("Ada's orders (qiHasMany, one query)");
    QiList<Order> adaOrders = qiHasMany<Order>(ada, "customer").all();
    for (int i = 0; i < adaOrders.size(); i++) {
        Order *o = adaOrders.at(i);
        qDebug().noquote() << "  order" << o->id->toInt()
                           << "total" << o->total->toInt() << "cents";
    }

    // --- #2 bulk update ---------------------------------------------------
    banner("Ship all pending orders (bulk UPDATE)");
    int shipped = Order::objects()
                      .filter( Order::col().status == Pending )
                      .update({ {"status", (int)Shipped} });
    qDebug().noquote() << "  marked" << shipped << "orders as Shipped";
    qDebug().noquote() << "  Shipped now:"
                       << Order::objects().filter( Order::col().status == Shipped ).count();

    // --- #5 CHECK constraint ---------------------------------------------
    banner("CHECK rejects an invalid row");
    Order bad; bad.customer = ada; bad.status = Pending; bad.total = -50;
    if (!bad.save())
        qDebug().noquote() << "  refused negative total:" << bad.lastError().text();

    // --- #4 composite key: same (order, sku) replaces in place ------------
    banner("Composite key REPLACE");
    OrderItem again; again.orderId = adaOrder1; again.sku = "WIDGET"; again.qty = 5;
    again.save();
    qDebug().noquote() << "  order_item rows:" << OrderItem::objects().count() << "(still 1)";

    // --- #1/#3 foreign key ON DELETE CASCADE ------------------------------
    banner("Delete a customer -> their orders cascade away");
    qDebug().noquote() << "  orders before:" << Order::objects().count();
    ada.remove();
    qDebug().noquote() << "  orders after removing Ada:" << Order::objects().count();

    // --- #7 rename / drop column ------------------------------------------
    banner("Migrate: rename + drop a column");
    conn.renameColumn<Order>("total", "amount");
    conn.dropColumn<Order>("note");
    QSqlQuery q = conn.query();
    q.exec("PRAGMA table_info(orders)");
    QStringList cols;
    while (q.next()) cols << q.value(1).toString();
    qDebug().noquote() << "  orders columns now:" << cols.join(", ");

    qDebug().noquote() << "\nDone.";
    return 0;
}
