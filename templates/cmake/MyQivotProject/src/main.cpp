#include <QCoreApplication>
#include <QSqlDatabase>
#include <qivot.h>
#include "models.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Open database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");  // Use in-memory database for this example
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return 1;
    }

    // Create connection and register models
    QiConnection connection;
    connection.open(db);
    connection.addModel<User>();
    connection.addModel<Task>();

    // Create tables
    if (!connection.createTables()) {
        qCritical() << "Failed to create tables:" << connection.lastError().text();
        return 1;
    }

    // Example: Create a user
    User user;
    user.name = "Alice";
    user.email = "alice@example.com";

    if (!user.save()) {
        qCritical() << "Failed to save user:" << user.lastError().text();
        return 1;
    }

    qInfo() << "Created user with id:" << user.id;

    // Example: Query users
    auto users = User::objects().all();
    qInfo() << "Total users:" << users.size();

    for (int i = 0; i < users.size(); i++) {
        User *u = users.at(i);
        qInfo() << "  -" << u->name << "(" << u->email << ")";
    }

    connection.close();
    return 0;
}
