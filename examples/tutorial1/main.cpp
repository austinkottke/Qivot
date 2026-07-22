#include <QtCore/QCoreApplication>
#include <qivot.h>
#include <QSqlDatabase>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    /// The user ID
    QiField<QString> userId;

    /// The account creation date
    QiField<QDateTime> creationDate;

    /// The karma of the user
    QiField<qreal> karma;

};

/// Declare the model and the field clause
QI_DECLARE_MODEL(User,
                 "user", // the table name.
                 QI_FIELD(userId , QiNotNull | QiUnique), // The field can not be null and must be unique
                 QI_FIELD(creationDate , QiDefault("CURRENT_TIMESTAMP") ), // The default value for the field is the current timestamp
                 QI_FIELD(karma) // If no special requirement to the field , you don't need to pass the second argument
                 );

/* The model declared in the above is equivalent to the following sql table on SQLite:

CREATE TABLE user  (
id INTEGER PRIMARY KEY AUTOINCREMENT,
userId TEXT NOT NULL UNIQUE,
creationDate DATETIME DEFAULT CURRENT_TIMESTAMP ,
karma DOUBLE
);

 */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tutorial1.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    if (!connection.open(db)) // Establish the connection to database. It will become the "default connection" shared by all QiModel
        return 1;

    connection.addModel<User>(); // Register a model to the connection

    if (!connection.createTables()) // Create table for all added model
        return 1;

    User user;

    if (!user.load( User::col().userId == "anonymous" )) { // Load the record from database where userId = anonymous
        // The account is not existed. Insert by ourself.

        user.userId = "anonymous"; // Assign the field value directly.
        user.karma = 0;

        user.save() ; // Save the record to database
    }

    qDebug() << user.id; // Result : QVariant(int, 1)
    // The "id" field is the primary key field which is provided by QiModel class automatically.
    // Each entry must have a primary key associated. It is null ( id->isNull() == true)
    // for newly created model instance. It will be assigned by save() function automatically

    qDebug() << user.userId << user.karma; // Result : QVariant(QString, "anonymous") QVariant(double, 0)

    qDebug() <<  user.creationDate; // Result : the record creation time

    qDebug() << user; // Display all the field

    // Remarks: QiField can be casted to QVariant automatically

    connection.close(); // Please close the connection on quit.

    return 0;
}
