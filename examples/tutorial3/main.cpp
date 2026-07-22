#include <QtCore/QCoreApplication>
#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    /// The user ID
    QiField<QString> userId;

    QiField<QString> passwd;

    QiField<QDateTime> lastModifiedTime;

    /// Initial data for table creation
    virtual QiSharedList initialData() const;

    /// Model fields validation
    virtual bool clean();

};

/// Declare the model and the field clause
QI_DECLARE_MODEL(User,
                 "user", // the table name.
                 QI_FIELD(userId , QiNotNull | QiUnique), // The field can not be null and must be unique
                 QI_FIELD(passwd , QiNotNull),
                 QI_FIELD(lastModifiedTime)
                 );


QiSharedList User::initialData() const{
    // QiSharedList is the base class QiList , and they can be casted their data type to other.
    QiList<User> res;
    QiListWriter writer(&res);

    // It should create 5 default account in table creation.
    writer << "tester1" << "12345678" << writer.next()
           << "tester2" << "12345678" << writer.next()
           << "tester3" << "12345678" << writer.next()
           << "tester4" << "12345678" << writer.next()
           << "tester5" << "12345678" << writer.next();

    return res;
}

bool User::clean() {
    // clean() is called each time before save the record.
    // You may validate the update database field here

    if (passwd->isNull()) {// Password must be present
        qDebug() << "Password is not set!";
        return false;
    }

    if (passwd->toString().size() < 8){ // Password should contains at least 8 character.
        qDebug() << "Password is too short!";
        return false;
    }

    lastModifiedTime = QDateTime::currentDateTime();

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tutorial3.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    if (!connection.open(db)) // Establish the connection to database. It will become the "default connection" shared by all QiModel
        return 1;

    connection.addModel<User>(); // Register a model to the connection

    if (!connection.createTables()) // Create table for all added model. It will also insert initial data
        return 1;


    auto list = User::objects().all();

    qDebug() << list.size(); // 5 or more record.
    qDebug() << list; // Print out the record

    User user;
    user.userId = "anonoymous";

    qDebug() << user.save(); // False , passwd is not set

    user.passwd = "123456";
    qDebug() << user.save(); // False , passwd is too short

    user.passwd = "123545678";
    qDebug() << user.save(); // True.

    qDebug() << user;

    (void) user.remove(); // Remove the record

    connection.close();

    return 0;
}
