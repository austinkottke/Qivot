/** multidb — the same Qivot code on SQLite, MySQL/MariaDB, or PostgreSQL.

    Pick the backend with the QIVOT_DB environment variable:

        ./multidb                 # SQLite (default) — works out of the box
        QIVOT_DB=mysql    ./multidb
        QIVOT_DB=postgres ./multidb

    Connection details come from env vars (with sensible defaults matching the
    Docker setup in tests/integration/). The ONLY backend-specific code is the
    connection in openDatabase(); everything after it is identical.
 */
#include "product.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

// The one place the backend matters: building the QSqlDatabase.
static QSqlDatabase openDatabase() {
    const QString backend = qEnvironmentVariable("QIVOT_DB", "sqlite").toLower();

    if (backend == "mysql" || backend == "mariadb") {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(qEnvironmentVariable("QIVOT_HOST", "127.0.0.1"));
        db.setPort(qEnvironmentVariable("QIVOT_PORT", "3306").toInt());
        db.setDatabaseName(qEnvironmentVariable("QIVOT_NAME", "qivot_test"));
        db.setUserName(qEnvironmentVariable("QIVOT_USER", "root"));
        db.setPassword(qEnvironmentVariable("QIVOT_PASS", "qivot"));
        return db;
    }
    if (backend == "postgres" || backend == "pg") {
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
        db.setHostName(qEnvironmentVariable("QIVOT_HOST", "127.0.0.1"));
        db.setPort(qEnvironmentVariable("QIVOT_PORT", "5432").toInt());
        db.setDatabaseName(qEnvironmentVariable("QIVOT_NAME", "qivot_test"));
        db.setUserName(qEnvironmentVariable("QIVOT_USER", "postgres"));
        db.setPassword(qEnvironmentVariable("QIVOT_PASS", "qivot"));
        return db;
    }
    // Default: SQLite, in a local file.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qEnvironmentVariable("QIVOT_NAME", "multidb.db"));
    return db;
}

static QJsonArray tags(const QStringList &items) {
    QJsonArray a;
    for (const QString &s : items) a.append(s);
    return a;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = openDatabase();
    if (!db.isValid()) {
        qWarning() << "This Qt has no driver for the requested backend. Available:"
                   << QSqlDatabase::drivers();
        return 1;
    }
    if (!db.open()) {
        qWarning().noquote() << "Could not connect:" << db.lastError().text();
        return 1;
    }

    qInfo().noquote() << "Connected via" << db.driverName() << "\n";

    // ---- everything below is backend-agnostic ----
    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Product>();
    if (!connection.dropTables() || !connection.createTables()) {
        qWarning() << "schema setup failed:" << connection.lastError().text();
        return 1;
    }

    // Insert a couple of products.
    { Product p; p.sku = "SKU-1"; p.name = "Coffee Mug"; p.price = 9.99; p.inStock = true;
      p.tags = tags({ "kitchen", "ceramic" }); p.save(); }
    { Product p; p.sku = "SKU-2"; p.name = "Notebook";   p.price = 4.50; p.inStock = false;
      p.tags = tags({ "office" });               p.save(); }

    // Query with a filter (in-stock only).
    qInfo().noquote() << "In stock:";
    QiList<Product> avail = QiQuery<Product>().filter(QiWhere("inStock = ", true)).orderBy("sku").all();
    for (int i = 0; i < avail.size(); i++)
        qInfo().noquote() << "  -" << QString(avail.at(i)->name)
                          << QString("($%1)").arg(double(avail.at(i)->price), 0, 'f', 2);

    // Upsert on the natural key: same SKU -> update the price in place (no duplicate).
    { Product p; p.sku = "SKU-1"; p.name = "Coffee Mug"; p.price = 7.49; p.inStock = true;
      p.tags = tags({ "kitchen", "ceramic", "sale" }); p.upsert(QStringList() << "sku"); }

    Product mug;
    mug.load(QiWhere("sku = ", "SKU-1"));
    qInfo().noquote() << "\nAfter upsert, SKU-1 price = $"
                      << QString::number(double(mug.price), 'f', 2)
                      << " tags =" << QJsonDocument(QJsonArray(mug.tags)).toJson(QJsonDocument::Compact);
    qInfo().noquote() << "Total products =" << QiQuery<Product>().count();

    connection.close();
    qInfo().noquote() << "\nSame code, any database. ✅";
    return 0;
}
