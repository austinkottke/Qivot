#include <QtCore/QCoreApplication>
#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<qreal>   karma;

    virtual QiSharedList initialData() const;
};

QI_DECLARE_MODEL(User,
                 "user",
                 QI_FIELD(userId , QiNotNull | QiUnique),
                 QI_FIELD(karma));

QiSharedList User::initialData() const {
    QiList<User> res;
    QiListWriter writer(&res);
    writer << "tester1" << 10
           << "tester2" << 20
           << "tester3" << 30
           << "tester4" << 40
           << "tester5" << 50;
    return res;
}

/// A table storing a directed friendship a -> b
class FriendShip : public QiModel {
    QI_MODEL
public:
    QiForeignKey<User> a;
    QiForeignKey<User> b;
};

QI_DECLARE_MODEL(FriendShip,
                 "friendship",
                 QI_FIELD(a , QiNotNull),
                 QI_FIELD(b , QiNotNull));

static void link(User &from, User &to) {
    FriendShip fs;
    fs.a = from;
    fs.b = to;
    fs.save(true);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("tutorial5.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db))
        return 1;
    connection.addModel<User>();
    connection.addModel<FriendShip>();

    if (!connection.dropTables() || !connection.createTables())
        return 1;

    User t1; t1.load(User::col().userId == "tester1");
    User t2; t2.load(User::col().userId == "tester2");
    User t3; t3.load(User::col().userId == "tester3");

    // tester1 -> tester2, tester1 -> tester3, tester2 -> tester3
    link(t1, t2);
    link(t1, t3);
    link(t2, t3);

    qDebug() << "=== INNER JOIN: friends of tester1 ===";
    {
        // SELECT user.* FROM user
        //   INNER JOIN friendship ON friendship.b = user.id
        //   WHERE friendship.a = <t1.id>
        auto query = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
                .filter( QiWhere("friendship.a") == t1.id );

        auto friends = query.all();
        qDebug() << "SQL   :" << query.lastQuery().executedQuery();
        qDebug() << "count :" << friends.size();          // expect 2
        for (int i = 0; i < friends.size(); i++)
            qDebug() << "friend:" << friends.at(i)->userId->toString();
    }

    qDebug() << "\n=== INNER JOIN + aggregate: avg karma of tester1's friends ===";
    {
        auto query = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
                .filter( QiWhere("friendship.a") == t1.id );

        QVariant avg = query.call("avg", "user.karma");
        qDebug() << "avg karma:" << avg.toDouble();       // expect (20+30)/2 = 25
    }

    qDebug() << "\n=== LEFT OUTER JOIN: every user, matched with friendships they initiate ===";
    {
        auto query = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id"),
                                           QiBaseJoin::Left ) )
                .orderBy("user.userId asc");

        auto rows = query.all();
        qDebug() << "SQL  :" << query.lastQuery().executedQuery();
        qDebug() << "rows :" << rows.size();               // tester1 x2, tester2 x1, tester3/4/5 x1 = 6
        for (int i = 0; i < rows.size(); i++)
            qDebug() << "row  :" << rows.at(i)->userId->toString();
    }

    qDebug() << "\n=== JOIN + count ===";
    {
        auto query = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.b") == QiWhere("user.id") ) )
                .filter( QiWhere("friendship.b") == t3.id );

        qDebug() << "friendships pointing at tester3:" << query.count();  // expect 2
    }

    qDebug() << "\n=== distinct(): users who have initiated at least one friendship ===";
    {
        // tester1 initiates two friendships, so without distinct() it appears twice.
        auto dup = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) )
                .all();
        qDebug() << "with duplicates :" << dup.size();       // expect 3 (t1, t1, t2)

        auto uniq = QiQuery<User>()
                .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) )
                .distinct()
                .all();
        qDebug() << "distinct        :" << uniq.size();       // expect 2 (t1, t2)
    }

    qDebug() << "\n=== auto join: ON derived from the declared foreign key ===";
    {
        // FriendShip declares QiForeignKey<User> a and b. Because there are TWO
        // foreign keys to User the relationship is ambiguous, so here we pass an
        // explicit ON. When a model has a single foreign key to the joined
        // table, QiJoin<T>() derives the ON automatically:  .join<User>()
        auto query = QiQuery<FriendShip>()
                .join( QiJoin<User>( QiWhere("user.id") == QiWhere("friendship.a") ) )
                .filter( QiWhere("user.userId") == "tester1" );
        qDebug() << "friendships initiated by tester1:" << query.count();  // expect 2
    }

    connection.close();
    return 0;
}
