#include <QtCore/QCoreApplication>
#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    /// The user ID
    QiField<QString> userId;


    /// The karma of the user
    QiField<qreal> karma;

};

/// Declare the model and the field clause
QI_DECLARE_MODEL(User,
                 "user", // the table name.
                 QI_FIELD(userId , QiNotNull | QiUnique), // The field can not be null and must be unique
                 QI_FIELD(karma) // If no special requirement to the field , you don't need to pass the second argument
                 );


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tutorial2.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    if (!connection.open(db)) // Establish the connection to database. It will become the "default connection" shared by all QiModel
        return 1;

    connection.addModel<User>(); // Register a model to the connection

    if (!connection.createTables()) // Create table for all added model
        return 1;

    /* Create initial record */

    // Initial data to be inserted to database
    QiList<User> initialData;
    QiListWriter writer(&initialData);

    writer << "tester1" << 50
           << "tester2" << 10
           << "tester3" << 60
           << "tester4" << 80
           << "tester5" << 100;

    initialData.save(); // save the list of record to database

    // Construct query object
    QiQuery<User> query; // A query object for performing database queries and deletion

    // Storage of query result
    QiList<User> list;

    // An instance of user data model
    User user;

    /* Read record one by one */

    query = query.filter(User::col().karma > 50); // Contruct a query object to find user where karma > 50;
    if (query.exec()){ // Execute the query
        while (query.next()) { // contains unread record
            query.recordTo(user); // Save the record
            qDebug() << user.userId;
        }
    }

    /* Alternative way */

    // Get the list of user where "karam > 50 "
    list = query.filter(User::col().karma > 50  ).all();
    qDebug() << list;

    // Get the list of user where "karam > 50 and karma <  80"
    list = query.filter(User::col().karma > 50 && User::col().karma < 80).all();
    qDebug() << list;

    // Get the list of user where "karam >= 50 and karma <= 80"
    list = query.filter(User::col().karma.between(50,80)).all();
    qDebug() << list;

    // Count no. of record .
    int count = query.filter(User::col().karma > 50).count();

    qDebug() << count; // = 3;

    query.reset(); // reset the query

    // Find the average karma
    // It is equivalent to " select avg(karma) "
    int avg = query.call("avg","karma").toInt();
    qDebug() << "Average karam" << avg;

    /* Another shortcut to construct query object by using the objects() call. */

    qDebug() << User::objects().call("sum","karma").toInt(); // Result : 300

    qDebug() << user.objects().call("sum","karma").toInt();// Result : 300

    connection.close();

    return 0;
}
