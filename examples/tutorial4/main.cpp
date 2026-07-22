#include <QtCore/QCoreApplication>

#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    /// The user ID
    QiField<QString> userId;

    virtual QiSharedList initialData() const;
};

QI_DECLARE_MODEL(User,
                 "user",
                 QI_FIELD(userId , QiNotNull | QiUnique)
                 );

QiSharedList User::initialData() const {
    QiList<User> res;
    QiListWriter writer(&res);

    writer << "tester1"
           << "tester2"
           << "tester3"
           << "tester4"
           << "tester5";

    return res;
}

/// A table to store the friendship status between two user
class FriendShip : public QiModel {
    QI_MODEL
public:

    /// Declara two foreign key to table User
    QiForeignKey<User> a;
    QiForeignKey<User> b;

    QiField<QDateTime> creationDate;

};

QI_DECLARE_MODEL(FriendShip,
                 "friendship",
                 QI_FIELD(a , QiNotNull ),
                 QI_FIELD(b , QiNotNull ),
                 QI_FIELD(creationDate , QiDefault("CURRENT_TIMESTAMP") ) // The default value for the field is the current timestamp
                 );

/* The model declared in the above is equivalent to the following sql table on SQLite:

CREATE TABLE friendship  (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    a INTEGER NOT NULL,
    b INTEGER NOT NULL,
    creationDate DATETIME DEFAULT CURRENT_TIMESTAMP ,
    FOREIGN KEY(a) REFERENCES user(id),
    FOREIGN KEY(b) REFERENCES user(id)
);

 */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tutorial4.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    if (!connection.open(db)) // Establish the connection to database. It will become the "default connection" shared by all QiModel
        return 1;

    connection.addModel<User>(); // Register a model to the connection
    connection.addModel<FriendShip>();

    if (!connection.dropTables())   // Drop all the table ,reinitialize the environment
        return 1;
    if (!connection.createTables()) // Create table for all added model
        return 1;

    User tester1;
    tester1.load(User::col().userId == "tester1");

    User tester2;
    tester2.load(User::col().userId == "tester2");

    User tester3;
    tester3.load(User::col().userId == "tester3");

    FriendShip friendship;
    friendship.a = tester1; // Link up tester 1 & tester2
    friendship.b = tester2;
    friendship.save(true); // Insert the link

    friendship.a = tester1; // Link up tester 1 & tester3
    friendship.b = tester3;
    friendship.save(true); // Insert the link

    auto list = FriendShip::objects()
                              .filter(FriendShip::col().a == tester1.id ).all();

    qDebug() << list;

    int n = list.size();
    qDebug() << "The friend of tester1: ";
    for (int i = 0 ; i < n;i++) {
        FriendShip* f = list.at(i);

        qDebug() << f->b->userId;
        /*
          f->b is a QiForeignKey field. QiForeignKey support -> operation overloading
          and you can access the field of the foreign  table. In this case,
          it is User. So f->b->userId is the equivalent to user.userId.

            FriendShip::objects().filter(QiWhere("a = ", tester1.id )).all()

          Morever, the query above will not access the "user" table. QiForeignKey will
          load the foreign table's record on request automatically.
         */
    }

    /*
     The result is :

        The friend of tester1 :
        QVariant(QString, "tester2")
        QVariant(QString, "tester3")
    */

    connection.close();

    return 0;
}
