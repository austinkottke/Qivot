#include <QJsonObject>
#include <QJsonArray>
#include <qijsonmapper.h>
#include <qitransaction.h>
#include <qifieldref.h>
#include <qirelation.h>
#include <qilog.h>
#include "sqlitetests.h"

// A model with a meaningful string primary key and NO auto-increment id column.
class Contact : public QiModel {
    QI_MODEL
public:
    QiField<QString> contactId;
    QiField<QString> name;
};
QI_DECLARE_MODEL_NOID(Contact, "contact",
                      QI_FIELD(contactId, QiPrimary | QiNotNull),
                      QI_FIELD(name));

// Composite primary key (two QiPrimary columns, no auto id).
class Enrollment : public QiModel {
    QI_MODEL
public:
    QiField<QString> studentId;
    QiField<QString> courseId;
    QiField<int>     grade;
};
QI_DECLARE_MODEL_NOID(Enrollment, "enrollment",
                      QI_FIELD(studentId, QiPrimary | QiNotNull),
                      QI_FIELD(courseId,  QiPrimary | QiNotNull),
                      QI_FIELD(grade));

// Foreign key with an ON DELETE CASCADE action, referencing an integer id.
class Author : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
};
QI_DECLARE_MODEL(Author, "author", QI_FIELD(name));

class Book : public QiModel {
    QI_MODEL
public:
    QiField<QString>                  title;
    QiForeignKey<Author, QiFkCascade> author;   // ON DELETE CASCADE
};
QI_DECLARE_MODEL(Book, "book", QI_FIELD(title), QI_FIELD(author));

// Foreign key that references a model with a custom string primary key.
class Team : public QiModel {
    QI_MODEL
public:
    QiField<QString> teamId;
    QiField<QString> teamName;
};
QI_DECLARE_MODEL_NOID(Team, "team",
                      QI_FIELD(teamId, QiPrimary | QiNotNull),
                      QI_FIELD(teamName));

class Member : public QiModel {
    QI_MODEL
public:
    QiField<QString>   memberName;
    QiForeignKey<Team> team;
};
QI_DECLARE_MODEL(Member, "member", QI_FIELD(memberName), QI_FIELD(team));

// #5 CHECK constraint
class Product : public QiModel {
    QI_MODEL
public:
    QiField<QString> sku;
    QiField<int>     qty;
};
QI_DECLARE_MODEL(Product, "product",
                 QI_FIELD(sku, QiNotNull),
                 QI_FIELD(qty, QiCheck("qty >= 0")));

// #9 WITHOUT ROWID (needs a primary key)
class Session : public QiModel {
    QI_MODEL
public:
    QiField<QString> token;
    QiField<QString> owner;
};
QI_DECLARE_MODEL_NOID(Session, "session",
                      QI_FIELD(token, QiPrimary | QiNotNull),
                      QI_FIELD(owner));

// #6 reverse relation (has many): a Blog has many Comments
class Blog : public QiModel {
    QI_MODEL
public:
    QiField<QString> blogTitle;
};
QI_DECLARE_MODEL(Blog, "blog", QI_FIELD(blogTitle));

class Comment : public QiModel {
    QI_MODEL
public:
    QiField<QString>   body;
    QiForeignKey<Blog> blog;
};
QI_DECLARE_MODEL(Comment, "comment", QI_FIELD(body), QI_FIELD(blog));

// #7 rename / drop column
class Migrate : public QiModel {
    QI_MODEL
public:
    QiField<QString> oldName;
    QiField<QString> dropMe;
    QiField<int>     keep;
};
QI_DECLARE_MODEL(Migrate, "migrate",
                 QI_FIELD(oldName), QI_FIELD(dropMe), QI_FIELD(keep));

// #8 enum field (stored as its underlying integer)
enum Priority { PrLow = 0, PrMedium = 1, PrHigh = 2 };
class Ticket : public QiModel {
    QI_MODEL
public:
    QiField<QString>   subject;
    QiField<Priority>  priority;
};
QI_DECLARE_MODEL(Ticket, "ticket", QI_FIELD(subject), QI_FIELD(priority));
Q_DECLARE_METATYPE(Priority)

// Per-column SQL type override (QI_FIELD_AS): a raw type, a typeless column.
class Activity : public QiModel {
    QI_MODEL
public:
    QiField<QString> ticketNumber;
    QiField<double>  billed;
    QiField<QString> closedUtc;
    QiField<int>     seq;
};
QI_DECLARE_MODEL(Activity, "activity",
                 QI_FIELD_AS(ticketNumber, "STRING", QiNotNull),
                 QI_FIELD_AS(billed,       "REAL"),
                 QI_FIELD_AS(closedUtc,    ""),        // typeless column
                 QI_FIELD(seq));                       // normal -> INTEGER

// Structured JSON fields: nested object + array held in memory, saved as TEXT.
class Doc : public QiModel {
    QI_MODEL
public:
    QiField<QString>     docName;
    QiField<QJsonObject> meta;
    QiField<QJsonArray>  tags;
};
QI_DECLARE_MODEL(Doc, "doc", QI_FIELD(docName), QI_FIELD(meta), QI_FIELD(tags));

SqliteTests::SqliteTests(QObject* parent) : QObject(parent)
{
}

void SqliteTests::initTestCase()
{
    verifyCreateTable();
    foreignKey();

    QiConnection defaultConnection = QiConnection::defaultConnection();

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "tests.db" );

    QVERIFY( db.open() );

    QVERIFY( !defaultConnection.isOpen());
    QVERIFY (connect.open(db) );

    QVERIFY(defaultConnection.isOpen()); // connect become default connection

    QiSql sql = connect.sql();


    QVERIFY( sql.createTableIfNotExists<Model1>() );

    QVERIFY( sql.exists(qiMetaInfo<Model1>() ) );

    QVERIFY( sql.dropTable(qiMetaInfo<Model1>()) );

    QVERIFY( !sql.exists(qiMetaInfo<Model1>() ) );

    QVERIFY( sql.createTableIfNotExists<Model1>() );

    QVERIFY ( connect.addModel<Model1>() );
    QVERIFY ( connect.addModel<Model2>() );
    QVERIFY (!connect.addModel<Model1>() );
    QVERIFY (!connect.addModel<Model3>() );
    QVERIFY ( connect.addModel<Model4>() );
    QVERIFY ( connect.addModel<User>() );
    QVERIFY ( connect.addModel<Config>() );
    QVERIFY ( connect.addModel<ExamResult>() );
    QVERIFY ( connect.addModel<AllType>() );
    QVERIFY ( connect.addModel<HealthCheck>());
    QVERIFY ( connect.addModel<Contact>());   // string PK, no auto id
    QVERIFY ( connect.addModel<Enrollment>()); // composite PK
    QVERIFY ( connect.addModel<Author>());
    QVERIFY ( connect.addModel<Book>());       // FK ON DELETE CASCADE
    QVERIFY ( connect.addModel<Team>());
    QVERIFY ( connect.addModel<Member>());     // FK -> string PK target
    QVERIFY ( connect.addModel<Product>());    // CHECK constraint
    QVERIFY ( connect.addModel<Session>());    // WITHOUT ROWID
    QVERIFY ( connect.addModel<Blog>());
    QVERIFY ( connect.addModel<Comment>());    // has-many
    QVERIFY ( connect.addModel<Migrate>());    // rename/drop column
    QVERIFY ( connect.addModel<Ticket>());     // enum field
    QVERIFY ( connect.addModel<Activity>());   // QI_FIELD_AS type overrides
    QVERIFY ( connect.addModel<Doc>());        // structured JSON fields
    qiWithoutRowid<Session>();                 // create session WITHOUT ROWID

    QVERIFY( connect.dropTables() );

    QVERIFY( connect.createTables() ); // recreate table

    /* Create index */

    QiIndex<Model1> index1("index1");
    index1 << "key" << "value";

    QVERIFY(connect.createIndex(index1));

    // drop the index
    QVERIFY(connect.dropIndex(index1.name()));
}

void SqliteTests::cleanupTestCase()
{
    connect.close();
}

void SqliteTests::verifyCreateTable(){
    QiSqliteStatement statement;
    QString sql = statement.createTableIfNotExists<Model1>();

    QString answer = "CREATE TABLE IF NOT EXISTS model1  (\n"
    "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "key TEXT NOT NULL,\n"
    "value TEXT \n"
    ");";

    QCOMPARE(sql,answer);

    QString sql2 = statement.createTableIfNotExists<Model2>();
    sql2.replace("model2","model1");
    QCOMPARE(sql,sql2);

    sql = statement.createTableIfNotExists<AllType>();

    QStringList lines = sql.split("\n");

    int n = 8; // no. of field
    QVERIFY(lines.size()  == n +3);

}

void SqliteTests::foreignKey() {
    Model2 model;

    QVERIFY (model.metaInfo()->foreignKeyNameList().size() == 0 );

    QiForeignKey<Model2> key;


    model.key = "test key";
    model.value = "test value";

    key = model;

    QVERIFY(key->key() == "test key");

    QiSqliteStatement statement;
    QString sql = statement.createTableIfNotExists<Config>();


    QString answer = "CREATE TABLE IF NOT EXISTS config  (\n"
    "id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "key TEXT ,\n"
    "value TEXT ,\n"
    "uid INTEGER NOT NULL,\n"
    "FOREIGN KEY(uid) REFERENCES user(id)\n"
    ");" ;

    QCOMPARE(sql,answer);

    Config config;
    QVERIFY (config.metaInfo()->foreignKeyNameList().size() == 1 );

}


void SqliteTests::insertInto(){
    QiSqliteStatement statement;
    QiModelMetaInfo *info = qiMetaInfo<Model2>();
    QString sql = statement.insertInto(info,info->fieldNameList());

    QVERIFY( sql == "INSERT INTO model2 (id,key,value) values (:id,:key,:value);" );

    sql = statement.replaceInto(info,info->fieldNameList());
    QVERIFY( sql == "REPLACE INTO model2 (id,key,value) values (:id,:key,:value);" );

    QiConnection connection = QiConnection::defaultConnection();
    QVERIFY (connection.isOpen() );


}

void SqliteTests::qiModelSave(){
    Model1 model1;

    QVERIFY(model1.id->isNull());

    model1.key = "save1";
    model1.value = "value1";

    QVERIFY ( model1.save() );
    QVERIFY (!model1.id->isNull());

    QVariant id = model1.id();

    QVERIFY ( model1.save() );
    QVERIFY ( model1.id() == id);

    QVERIFY ( model1.save(true) ); // Force insert
    QVERIFY ( model1.id() != id); // ID should be changed.
}

void SqliteTests::deleteAll() {
    QiQuery<Model1> query;

    QVERIFY(query.remove());

    int count = query.count();
    QVERIFY(count == 0);

    Model1 model1;
    model1.key = "temp";
    model1.value = "temp value";
    model1.save();

    query = QiQuery<Model1>();
    count = query.count();
    QVERIFY(count == 1);

    count = query.count(); // call it twice
    QVERIFY(count == 1);

    QVERIFY(!model1.id->isNull());
    QVERIFY(model1.remove());
    QVERIFY(model1.id->isNull());
    QVERIFY(query.count() == 0);
    QVERIFY(model1.save());
    QVERIFY(query.count() == 1);

    int id = model1.id().toInt();
    Model1 model1b;
    QVERIFY(model1b.load( QiWhere("id = ",id) ));
//    QVERIFY(model1.id() == model1b.id() );
    QVERIFY(model1.id == model1b.id );

    query = QiQuery<Model1>().filter(QiWhere("key = " ,"temp")).limit(1);
    QEXPECT_FAIL("","Normally sqlite do not support limit in delete from , unless it is build with special flag",Continue);
    QVERIFY(query.remove());

    qDebug() << "Failed sql : " << query.lastQuery().lastQuery();

    query = QiQuery<Model1>().filter(QiWhere("key" , "=" , "temp"));
    QVERIFY(query.remove());

    qDebug() << query.lastQuery().lastQuery();

    query = QiQuery<Model1>();

    count = query.count();
    QVERIFY(count == 0);


}

void SqliteTests::prepareInitRecords() {
    Model1 model1;
    model1.key = "config1";
    model1.value ="value1";
    QVERIFY ( model1.save(true) );

    model1.key = "config2";
    model1.value ="value2";
    QVERIFY ( model1.save(true) );

    model1.key = "config3";
    model1.value ="value3";
    QVERIFY ( model1.save(true) );

    Model2 model2;
    model2.key = "config1";
    model2.value = "value1";
    QVERIFY(model2.save());

    model2.key = "config2";
    model2.value = "value2";
    QVERIFY(model2.save(true));

    User user;
    user.name = "Ben Lau";
    user.userId = "benlau";

    QVERIFY(!user.save()); // passwd is missed

    user.passwd = "123456";
    QVERIFY (!user.save()); // passwd is too short
    user.passwd = "12345678";
    QVERIFY (user.save());

    Config config;
    config.key = "config1";
    config.value = "value1";
    QVERIFY(!config.save()); // config.uid can not be null

    QVERIFY(config.id->isNull());

    config.uid = user;

    QVERIFY(config.save());

    ExamResult result;
    result.uid = user;
    result.subject = "English";
    result.mark = 50;
    QVERIFY(result.save());

    result.subject = "Maths";
    result.mark = 80;
    QVERIFY(result.save(true));
    QiQuery<ExamResult> examResultQuery;
    QVariant total = examResultQuery.filter(QiWhere("uid = " , user.id )).call("sum",QStringList("mark"));
    QVERIFY(total == 130);
    QVariant avg = examResultQuery.filter(QiWhere("uid = " , user.id() )).call("avg",QStringList("mark"));
    QVERIFY(avg == 65);

}

void SqliteTests::select()
{
    Model1 model1a,model1b;

    QiQuery<Model1> query1 = QiQuery<Model1>().filter(QiWhere("key","=","config1")).limit(1) ;

    QiSqliteStatement statement;
    QString sql = statement.select(query1);

    qDebug() << sql;

    QVERIFY (query1.exec() );
    QVERIFY (query1.next() );

    QVERIFY( query1.recordTo(model1a)) ;

    QVERIFY( !model1a.id->isNull());
    QVERIFY( model1a.key() == "config1");
    QVERIFY( model1a.value() == "value1");

    model1b = query1.record();

    QVERIFY( !model1b.id->isNull());
    QVERIFY( model1b.key() == "config1");
    QVERIFY( model1b.value() == "value1");

    QVERIFY( !query1.next());

}

void SqliteTests::queryAll(){
    QiQuery<Model2> query;

    QiList<Model2> record = query.all();

    QVERIFY(record.size() == 7); // 5 initial record + 2 newly inserted record
    QVERIFY(record.at(0)->key() == "initial0");
    QVERIFY(record.at(0)->value() == "value0");
    QVERIFY(record.at(1)->key() == "initial1");
    QVERIFY(record.at(1)->value() == "value1");

    // Alernative way

    record = Model2::objects().all();
    QVERIFY(record.size() == 7);

}

void SqliteTests::querySelect() {
    QiQuery<Model2> query;

    QiList<Model2> record = query.select("key").all();

    QVERIFY(record.size() == 7); // 5 initial record + 2 newly inserted record
    QVERIFY(record.at(0)->key() == "initial0");
    QVERIFY(record.at(0)->value().isNull()); // It is not loaded
    QVERIFY(record.at(1)->key() == "initial1");
    QVERIFY(record.at(1)->value().isNull()); // It is not loaded

    // Alernative way

    record = Model2::objects().all();
    QVERIFY(record.size() == 7);

}

void SqliteTests::querySelectWhere(){
    QiQuery<HealthCheck> query;

    QVERIFY(query.remove());

    QiList<HealthCheck> list;
    QiListWriter writer(&list);

    writer << "Tester 1 - Alvin" << 180 << 150 << writer.next()
           << "Tester 2 - Ben" << 170 << 120 << writer.next()
           << "Tester 3 - Candy" << 150 << 180 << writer.next()
           << "Tester 4 - David" << 130 << 130 << writer.next();

    list.save();

    query =query.filter(QiWhere("height") == (QiWhere("weight")));
    list = query.all();

    QString sql = query.lastQuery().lastQuery();
    qDebug() << sql;
    QVERIFY(sql == "SELECT ALL * FROM healthcheck WHERE height = weight ;");

    QVERIFY(list.size() == 1); // Tester 4;

    query.reset();
    QVERIFY(query.filter(QiWhere("height") != 130 ).all().size() == 3);
    QVERIFY(query.filter(QiWhere("height").isNot(130) ).all().size() == 3);
    QVERIFY(query.filter(QiWhere("height").is(130) ).all().size() == 1);

    query.reset();
    query =query.filter(QiWhere("height").between(150,170));
    list = query.all();
//    qDebug() << query.lastQuery().lastQuery();
    QVERIFY(list.size() == 2);

    query.reset();
    QList<QVariant> range;
    range << 120 << 130 << 150 << 300;

    query =query.filter(QiWhere("weight").in(range));
    list = query.all();
    qDebug() << query.lastQuery().lastQuery();
    QVERIFY(list.size() == 3);

    query.reset();
    QVERIFY(query.filter(QiWhere("weight").notIn(range)).all().size() == 1);

    QVERIFY(query.filter(QiWhere("name").like("Tester%")).all().size() == 4);
    QVERIFY(query.filter(QiWhere("name").like("Tester")).all().size() == 0);

    QCOMPARE(query.filter(QiWhere("name").glob("Tester*")).all().size() , 4);

    QVERIFY(query.remove());
}

void SqliteTests::foreignKeyLoad() {
    User user;
    user.name = "foreignKeyLoad";
    user.userId = "foreignKeyLoad";
    user.passwd = "12345678";

    QVERIFY(user.save());
    Config config1;
    config1.key = "autoLogin";
    config1.value = "true";
    QVERIFY(!config1.save()); // config.uid can not be null
    QVERIFY(config1.id->isNull());

    QVERIFY(!config1.uid.isLoaded());

    config1.uid = user; // Test can QiForeignKey read from QiModel.
    QVERIFY(config1.save());

    QVERIFY(config1.uid.isLoaded());

    QiQuery<Config> query = QiQuery<Config>().filter(QiWhere("key = ", "autoLogin")).limit(1);

    QVERIFY(query.exec());
    QVERIFY(query.next());

    Config config2 = query.record();
    QVERIFY(config2.key() == "autoLogin");

    QVERIFY(!config2.uid.isLoaded());
    QVERIFY(config2.uid->name() == "foreignKeyLoad");
    QVERIFY(config2.uid.isLoaded());
}

void SqliteTests::model4() {
    Model4 item1,item2;

    item1.key = "test";
    item1.value = "test";
    QVERIFY (item1.save());

//    qDebug() << item1.lastQuery().lastQuery();
    qDebug() << &item1;

    QVERIFY (item2.load(QiWhere("key = ","test")));

    QVERIFY(item2.help == "...");

}

void SqliteTests::datetime() {
    User user1;

    user1.userId = "tester";
    user1.name = "tester";
    user1.passwd = "12345678";

    QDateTime datetime = QDateTime::currentDateTime().toUTC();

    user1.lastLoginTime = datetime;

    QVERIFY(user1.clean());
    QVERIFY(user1.save());

    User user2;
    QVERIFY(user2.load(QiWhere("userId=","tester")));

#if (QT_VERSION > QT_VERSION_CHECK(5, 0, 2))
    // The timezone checking do not exists in Qt 5.0.2
    QEXPECT_FAIL("","Sqlite driver do not save the time zone of QDateTime type",Continue);
#endif

    QVERIFY(user2.lastLoginTime == datetime);

    QString format("yyyy-MM-ddThh:mm:ss");
    QVERIFY(user2.lastLoginTime.get().toDateTime().toString(format) == datetime.toString(format));

    QVERIFY(!user2.creationTime->isNull());
}

void SqliteTests::checkTypeSaveAndLoad(){

    QStringList sl;
    sl << "a" << "b" << "c";

    AllType type1,type2;
    type1.d = "1.0";
    QByteArray data(100,'0');
    type1.data = data;
    type1.b = true;
    type1.sl = sl;

    QVERIFY(type1.save());

    QVERIFY(type2.load(QiWhere("id=",type1.id) ) );

    QVERIFY(type2.data == data);
    QVERIFY(type2.d == "1.0");
    QVERIFY(type2.b == true);
    qDebug() << type2.sl;
    QVERIFY(type2.sl == sl);

    // try again

    type1.b = false;

    QVERIFY(type1.save());

    QVERIFY(type2.load(QiWhere("id=",type1.id) ) );

    QVERIFY(type2.b == false);

}

void SqliteTests::queryOrderBy(){
    QiQuery<HealthCheck> query;

    QVERIFY(query.remove());

    QiList<HealthCheck> records;

    QiListWriter writer(&records);

    writer << "tester 1" << 160 << 120 << writer.next()
           << "tester 2" << 120 << 170 << writer.next()
           << "tester 3" << 140 << 110 << writer.next();

    writer.close();
    QVERIFY(records.save());

    QiList<HealthCheck> result = query.all();
    QVERIFY(result.size() == 3);

    result = query.orderBy("height asc").all();

    QVERIFY(result.size() == 3);

    QVERIFY(result.at(0)->name == "tester 2");
    QVERIFY(result.at(2)->name == "tester 1");

    result = query.orderBy("height desc").all();
    QVERIFY(result.at(0)->name == "tester 1");
    QVERIFY(result.at(2)->name == "tester 2");

    result = query.orderBy("weight desc").all();
    QVERIFY(result.at(0)->name == "tester 2");
    QVERIFY(result.at(2)->name == "tester 3");

    result = query.orderBy("weight asc").all();
    QVERIFY(result.at(0)->name == "tester 3");
    QVERIFY(result.at(2)->name == "tester 2");

}

void SqliteTests::join(){
    // Prepare a user and two config records that reference it.
    User user;
    user.userId = "joinUser";
    user.name = "joinUser";
    user.passwd = "12345678";
    QVERIFY(user.save());

    Config config1;
    config1.key = "joinKey1";
    config1.value = "joinValue1";
    config1.uid = user;
    QVERIFY(config1.save());

    Config config2;
    config2.key = "joinKey2";
    config2.value = "joinValue2";
    config2.uid = user;
    QVERIFY(config2.save());

    // A config that references a *different* user - it must NOT be returned.
    User other;
    other.userId = "joinOther";
    other.name = "joinOther";
    other.passwd = "12345678";
    QVERIFY(other.save());

    Config config3;
    config3.key = "joinKey3";
    config3.value = "joinValue3";
    config3.uid = other;
    QVERIFY(config3.save());

    // INNER JOIN with a cross-table filter.
    // SELECT config.<cols> FROM config
    //   INNER JOIN user ON user.id = config.uid
    //   WHERE user.userId = 'joinUser'
    QiQuery<Config> query = QiQuery<Config>()
            .join( QiJoin<User>( QiWhere("user.id") == QiWhere("config.uid") ) )
            .filter( QiWhere("user.userId") == "joinUser" )
            .orderBy("config.key asc");

    QiList<Config> list = query.all();

    QVERIFY(list.size() == 2);
    QVERIFY(list.at(0)->key() == "joinKey1");
    QVERIFY(list.at(1)->key() == "joinKey2");

    // "id" exists on both config and user. The mapped id must be the config's,
    // proving the primary columns were qualified to avoid ambiguity.
    QVERIFY(list.at(0)->id() == config1.id());
    QVERIFY(list.at(1)->id() == config2.id());
    // The foreign key column of config must also be mapped (not user's id).
    QVERIFY(list.at(0)->uid.get() == user.id());

    // COUNT over the join.
    int n = QiQuery<Config>()
            .join( QiJoin<User>( QiWhere("user.id") == QiWhere("config.uid") ) )
            .filter( QiWhere("user.userId") == "joinUser" )
            .count();
    QVERIFY(n == 2);

    // A literal value inside the ON condition must be bound correctly and must
    // not collide with the WHERE clause's placeholders (both would otherwise be
    // ":arg0"). Here the ON restricts the join to config1 only.
    QiList<Config> list2 = QiQuery<Config>()
            .join( QiJoin<User>( (QiWhere("user.id") == QiWhere("config.uid"))
                                 && (QiWhere("config.key") == "joinKey1") ) )
            .filter( QiWhere("user.userId") == "joinUser" )
            .all();
    QVERIFY(list2.size() == 1);
    QVERIFY(list2.at(0)->key() == "joinKey1");

    // LEFT OUTER JOIN: every config still appears (config3 included).
    int total = QiQuery<Config>()
            .join( QiJoin<User>( QiWhere("user.id") == QiWhere("config.uid"),
                                 QiBaseJoin::Left ) )
            .count();
    QVERIFY(total >= 3);

    // --- Auto join: derive the ON condition from the declared foreign key ---
    // Config declares QiForeignKey<User> uid , so join<User>() must produce
    // the same result as the explicit ON above.
    QiList<Config> autoList = Config::objects()
            .join<User>()
            .filter( QiWhere("user.userId") == "joinUser" )
            .orderBy("config.key asc")
            .all();
    QVERIFY(autoList.size() == 2);
    QVERIFY(autoList.at(0)->key() == "joinKey1");
    QVERIFY(autoList.at(1)->key() == "joinKey2");

    // The auto join must also work in the other direction (User -> Config).
    int autoReverse = User::objects()
            .join( QiJoin<Config>() )
            .filter( QiWhere("user.userId") == "joinUser" )
            .count();
    QVERIFY(autoReverse == 2); // joinUser owns config1 and config2

    // --- distinct(): a one-to-many join duplicates the primary row ---
    // joinUser owns two configs, so joining User -> Config yields two identical
    // user rows. distinct() collapses them to one.
    QiList<User> dupRows = User::objects()
            .join( QiJoin<Config>( QiWhere("config.uid") == QiWhere("user.id") ) )
            .filter( QiWhere("user.userId") == "joinUser" )
            .all();
    QVERIFY(dupRows.size() == 2);

    QiList<User> distinctRows = User::objects()
            .join( QiJoin<Config>( QiWhere("config.uid") == QiWhere("user.id") ) )
            .filter( QiWhere("user.userId") == "joinUser" )
            .distinct()
            .all();
    QVERIFY(distinctRows.size() == 1);
    QVERIFY(distinctRows.at(0)->userId->toString() == "joinUser");

    // --- CROSS JOIN: cartesian product ---
    int nConfig = Config::objects().count();
    int nUser   = User::objects().count();
    int cross = Config::objects()
            .join( QiJoin<User>( QiWhere(), QiBaseJoin::Cross ) )
            .count();
    QVERIFY(cross == nConfig * nUser);

    // --- RIGHT OUTER JOIN (only if the SQLite backend supports it >= 3.39) ---
    bool supportsOuter = false;
    {
        QSqlQuery vq(db);
        if (vq.exec("SELECT sqlite_version()") && vq.next()) {
            QStringList parts = vq.value(0).toString().split(".");
            int major = parts.value(0).toInt();
            int minor = parts.value(1).toInt();
            supportsOuter = (major > 3) || (major == 3 && minor >= 39);
        }
    }
    if (supportsOuter) {
        int right = QiQuery<Config>()
                .join( QiJoin<User>( QiWhere("user.id") == QiWhere("config.uid"),
                                     QiBaseJoin::Right ) )
                .count();
        QVERIFY(right >= 3);

        int full = QiQuery<Config>()
                .join( QiJoin<User>( QiWhere("user.id") == QiWhere("config.uid"),
                                     QiBaseJoin::Full ) )
                .count();
        QVERIFY(full >= 3);
    } else {
        qDebug() << "Skipping RIGHT/FULL OUTER JOIN test (SQLite < 3.39)";
    }
}

void SqliteTests::jsonMapper() {
    // --- object -> model, extra keys ignored, missing keys left alone ---
    QJsonObject obj;
    obj["userId"] = "jsonUser";
    obj["name"]   = "Json User";
    obj["passwd"] = "12345678";
    obj["bogusField"] = "should be ignored";   // not a declared field

    User user = QiJsonMapper::map<User>(obj);
    QVERIFY(user.userId->toString() == "jsonUser");
    QVERIFY(user.name->toString()   == "Json User");
    QVERIFY(user.passwd->toString() == "12345678");

    // --- model -> object round trip ---
    QJsonObject out = QiJsonMapper::toJson(&user);
    QVERIFY(out.value("userId").toString() == "jsonUser");
    QVERIFY(out.value("name").toString()   == "Json User");
    QVERIFY(out.contains("id")); // the primary key field is serialized too

    // --- type coercion: ISO date-time string -> QDateTime field ---
    QJsonObject withTime;
    withTime["userId"] = "jsonTime";
    withTime["passwd"] = "12345678";
    withTime["lastLoginTime"] = "2020-01-02T03:04:05";
    User timed = QiJsonMapper::map<User>(withTime);
    QVERIFY(timed.lastLoginTime->toDateTime()
            == QDateTime::fromString("2020-01-02T03:04:05", Qt::ISODate));

    // --- array -> QiList, then persist to the database ---
    QJsonArray arr;
    QJsonObject a; a["userId"] = "jsonA"; a["name"] = "A"; a["passwd"] = "12345678";
    QJsonObject b; b["userId"] = "jsonB"; b["name"] = "B"; b["passwd"] = "12345678";
    arr.append(a);
    arr.append(b);

    QiList<User> list = QiJsonMapper::map<User>(arr);
    QVERIFY(list.size() == 2);
    QVERIFY(list.save());

    User loaded;
    QVERIFY(loaded.load(QiWhere("userId") == "jsonA"));
    QVERIFY(loaded.name->toString() == "A");
}

void SqliteTests::upsert() {
    // First upsert on the natural key "userId" -> INSERT
    User u1;
    u1.userId = "upsertUser";
    u1.name   = "First";
    u1.passwd = "12345678";
    QVERIFY(u1.upsert(QStringList() << "userId"));

    QVariant firstId = u1.id();
    QVERIFY(!firstId.isNull());

    // Second upsert with the SAME natural key but new data -> UPDATE in place
    User u2;
    u2.userId = "upsertUser";
    u2.name   = "Second";
    u2.passwd = "12345678";
    QVERIFY(u2.upsert(QStringList() << "userId"));

    // No duplicate row was created
    int count = User::objects().filter(QiWhere("userId") == "upsertUser").count();
    QVERIFY(count == 1);

    User loaded;
    QVERIFY(loaded.load(QiWhere("userId") == "upsertUser"));
    QVERIFY(loaded.name->toString() == "Second");   // the row was updated
    QVERIFY(loaded.id() == firstId);                 // ...in place: same primary key (non-destructive)
}

void SqliteTests::fts() {
    // Build an FTS5 index over User(userId, name)
    QiFtsIndex<User> idx("user_fts");
    idx << "userId" << "name";
    QVERIFY(connect.createFtsIndex(idx));

    User u1; u1.userId = "fts_alpha"; u1.name = "quick brown fox";   u1.passwd = "12345678";
    QVERIFY(u1.save());
    User u2; u2.userId = "fts_beta";  u2.name = "lazy sleeping dog";  u2.passwd = "12345678";
    QVERIFY(u2.save());

    // MATCH search
    QiList<User> hits = QiQuery<User>().search("user_fts", "brown").all();
    QVERIFY(hits.size() == 1);
    QVERIFY(hits.at(0)->userId->toString() == "fts_alpha");
    QVERIFY(QiQuery<User>().search("user_fts", "dog").all().size() == 1);

    // Update via save() (REPLACE) keeps the index in sync (recursive_triggers)
    u1.name = "swift red panda";
    QVERIFY(u1.save());
    QVERIFY(QiQuery<User>().search("user_fts", "brown").all().size() == 0);  // old term gone
    QVERIFY(QiQuery<User>().search("user_fts", "panda").all().size() == 1);  // new term indexed

    // Update via upsert() (ON CONFLICT DO UPDATE) keeps the index in sync too
    User u3; u3.userId = "fts_beta"; u3.name = "curious cat"; u3.passwd = "12345678";
    QVERIFY(u3.upsert(QStringList() << "userId"));
    QVERIFY(QiQuery<User>().search("user_fts", "dog").all().size() == 0);
    QVERIFY(QiQuery<User>().search("user_fts", "cat").all().size() == 1);

    // Delete removes it from the index
    QVERIFY(u1.remove());
    QVERIFY(QiQuery<User>().search("user_fts", "panda").all().size() == 0);

    // Tear down the index (drops triggers + virtual table)
    QVERIFY(connect.dropFtsIndex("user_fts"));
}

void SqliteTests::transaction() {
    // Rolled-back transaction leaves nothing behind
    {
        QiTransaction tx;
        Model1 m; m.key = "tx_rollback"; m.value = "1";
        QVERIFY(m.save());
        // tx goes out of scope without commit() -> rolled back
    }
    QVERIFY(QiQuery<Model1>().filter(QiWhere("key") == "tx_rollback").count() == 0);

    // Committed transaction persists
    {
        QiTransaction tx;
        Model1 m; m.key = "tx_commit"; m.value = "1";
        QVERIFY(m.save());
        QVERIFY(tx.commit());
    }
    QVERIFY(QiQuery<Model1>().filter(QiWhere("key") == "tx_commit").count() == 1);
}

void SqliteTests::fkEnforcement() {
    // Config.uid is a NOT NULL foreign key to user. With foreign keys enforced,
    // referencing a non-existent user must be rejected.
    Config c;
    c.key = "fk_reject";
    c.value = "x";
    c.uid = QVariant(9999999); // no such user id
    QVERIFY(!c.save());        // FK violation -> save fails

    // A reference to a real user is accepted
    User u; u.userId = "fk_owner"; u.name = "owner"; u.passwd = "12345678";
    QVERIFY(u.save());
    Config ok;
    ok.key = "fk_ok";
    ok.value = "x";
    ok.uid = u;
    QVERIFY(ok.save());
}

void SqliteTests::migration() {
    QiSql sql = connect.sql();
    QSqlQuery q = connect.query();

    // Recreate model1's table missing the "value" column
    QVERIFY(q.exec("DROP TABLE IF EXISTS model1"));
    QVERIFY(q.exec("CREATE TABLE model1 (id INTEGER PRIMARY KEY AUTOINCREMENT, key TEXT)"));

    QStringList before = sql.columnNames(qiMetaInfo<Model1>());
    QVERIFY(before.contains("key"));
    QVERIFY(!before.contains("value"));

    // createTables() should migrate the existing table (ALTER TABLE ADD COLUMN value)
    QVERIFY(connect.createTables());

    QStringList after = sql.columnNames(qiMetaInfo<Model1>());
    QVERIFY(after.contains("value"));

    // The migrated column is usable
    Model1 m; m.key = "migrated"; m.value = "works";
    QVERIFY(m.save());
    Model1 loaded;
    QVERIFY(loaded.load(QiWhere("key") == "migrated"));
    QVERIFY(loaded.value->toString() == "works");
}

void SqliteTests::batchSave() {
    QiList<User> list;

    // Two records with different non-null field sets: this exercises the
    // field-signature grouping (each group reuses its own prepared statement).
    User *u1 = new User(); u1->userId = "batch_one"; u1->name = "one"; u1->passwd = "12345678";
    User *u2 = new User(); u2->userId = "batch_two";                   u2->passwd = "12345678";
    list.append(u1);   // ownership passes to the list
    list.append(u2);

    QVERIFY(list.save());

    // Both persisted, and their ids were assigned back
    QVERIFY(!u1->id->isNull());
    QVERIFY(!u2->id->isNull());
    QVERIFY(u1->id() != u2->id());
    QVERIFY(QiQuery<User>().filter(QiWhere("userId") == "batch_one").count() == 1);
    QVERIFY(QiQuery<User>().filter(QiWhere("userId") == "batch_two").count() == 1);
}

void SqliteTests::errorHandling() {
    // 1. Validation error : clean() rejects, and its message surfaces via lastError()
    User bad;
    bad.userId = "err_user";
    bad.name   = "x";
    bad.passwd = "short";                  // < 8 chars -> clean() fails
    QVERIFY(!bad.save());
    QVERIFY(bad.lastError().type() == QiError::ValidationError);
    QVERIFY(bad.lastError().text() == "password must be at least 8 characters");

    // ...and a successful save clears the error
    bad.passwd = "12345678";
    QVERIFY(bad.save());
    QVERIFY(bad.lastError().type() == QiError::NoError);

    // 2. Statement error : a foreign-key violation
    Config cfg;
    cfg.key   = "err_cfg";
    cfg.value = "v";
    cfg.uid   = QVariant(88888888);        // no such user
    QVERIFY(!cfg.save());
    QVERIFY(cfg.lastError().type() == QiError::StatementError);
    QVERIFY(!cfg.lastError().text().isEmpty());

    // 3. Not found
    User missing;
    QVERIFY(!missing.load(QiWhere("userId") == "no_such_user"));
    QVERIFY(missing.lastError().type() == QiError::NotFound);

    // 4. Connection-level error : an index on a non-existent column
    QiIndex<Model1> badIndex("bad_index");
    badIndex << "no_such_column";
    QVERIFY(!connect.createIndex(badIndex));
    QVERIFY(connect.lastError().type() == QiError::StatementError);
    QVERIFY(!connect.lastError().text().isEmpty());
}

void SqliteTests::queryExtras() {
    // Seed a controlled, isolated set of users (prefix "qx")
    for (int i = 1; i <= 5; i++) {
        User u;
        u.userId = QString("qx%1").arg(i);
        u.name   = (i <= 3) ? "groupA" : "groupB";   // groupA x3, groupB x2
        u.passwd = "12345678";
        QVERIFY(u.save());
    }

    // --- OFFSET + LIMIT + ORDER BY together (also proves the clause order) ---
    QiList<User> page = QiQuery<User>()
            .filter(QiWhere("userId").like("qx%"))
            .orderBy("userId asc")
            .limit(2)
            .offset(1)
            .all();
    QVERIFY(page.size() == 2);
    QVERIFY(page.at(0)->userId->toString() == "qx2");   // qx1 skipped
    QVERIFY(page.at(1)->userId->toString() == "qx3");

    // --- GROUP BY + HAVING + value(int) ---
    // SELECT name, count(*) FROM user WHERE userId like 'qx%'
    //   GROUP BY name HAVING count(*) > 2
    QiQuery<User> grouped = QiQuery<User>()
            .filter(QiWhere("userId").like("qx%"))
            .select(QStringList() << "name" << "count(*)")
            .groupBy("name")
            .having(QiWhere("count(*)") > 2)
            .orderBy("name asc");
    QVERIFY(grouped.exec());
    QVERIFY(grouped.next());
    QVERIFY(grouped.value(0).toString() == "groupA");   // only groupA has > 2
    QVERIFY(grouped.value(1).toInt() == 3);
    QVERIFY(!grouped.next());                            // groupB filtered out by HAVING

    // --- WAL journal mode ---
    QVERIFY(connect.setJournalMode("WAL"));
    QVERIFY(connect.setJournalMode("DELETE"));           // restore for the rest of the suite

    // --- UNIQUE index ---
    QiIndex<User> uq("uq_userid");                       // userId is unique -> succeeds
    uq << "userId";
    uq.setUnique(true);
    QVERIFY(connect.createIndex(uq));

    QiIndex<User> dup("uq_name");                        // name has duplicates (groupA x3)
    dup << "name";
    dup.setUnique(true);
    QVERIFY(!connect.createIndex(dup));                  // fails: proves UNIQUE was applied
    QVERIFY(connect.lastError().type() == QiError::StatementError);

    QVERIFY(connect.dropIndex("uq_userid"));
}

void SqliteTests::fieldRef() {
    // A pointer-to-member resolves to the column name (compiler-checked, no string)
    QVERIFY(qiField(&User::userId).toString() == "userId");
    QVERIFY(qiField(&User::name).toString()   == "name");

    User u;
    u.userId = "fieldRefUser";
    u.name   = "Ada";
    u.passwd = "12345678";
    QVERIFY(u.save());

    // Use it in load() and in a query builder — no hardcoded "userId"
    User loaded;
    QVERIFY(loaded.load( qiField(&User::userId) == "fieldRefUser" ));
    QVERIFY(loaded.name->toString() == "Ada");

    int c = User::objects().filter( qiField(&User::userId) == "fieldRefUser" ).count();
    QVERIFY(c == 1);
}

void SqliteTests::columns() {
    // Each .field is a real member -> renaming a column is a compile error here.
    QVERIFY(User::col().userId.toString() == "userId");
    QVERIFY(User::col().name.toString()   == "name");
    QVERIFY(User::col().id.toString()     == "id");   // built-in primary key

    User a; a.userId = "colUserA"; a.name = "Grace"; a.passwd = "12345678"; QVERIFY(a.save());
    User b; b.userId = "colUserB"; b.name = "Alan";  b.passwd = "12345678"; QVERIFY(b.save());

    // Cache the descriptor once, reuse across a compound query
    auto U = User::col();

    User loaded;
    QVERIFY(loaded.load( U.userId == "colUserA" ));
    QVERIFY(loaded.name->toString() == "Grace");

    int both = User::objects()
                   .filter( U.userId == "colUserA" || U.userId == "colUserB" )
                   .count();
    QVERIFY(both == 2);

    int grace = User::objects()
                    .filter( U.name == "Grace" && U.userId != "colUserB" )
                    .count();
    QVERIFY(grace == 1);

    // Descriptor QiWhere and qiField() QiWhere are interchangeable
    QVERIFY(User::col().userId.toString() == qiField(&User::userId).toString());

    // Type-safe ordering terms
    QVERIFY(User::col().userId.asc()  == "userId asc");
    QVERIFY(User::col().name.desc()   == "name desc");

    // ... and they drive orderBy() end to end
    auto ordered = User::objects()
                       .filter( U.userId == "colUserA" || U.userId == "colUserB" )
                       .orderBy( User::col().userId.desc() )
                       .all();
    QVERIFY(ordered.size() == 2);
    QVERIFY(ordered.at(0)->userId->toString() == "colUserB");   // desc: B before A
}

void SqliteTests::logging() {
    // Capture into function-static storage so the installed handler never
    // dangles, even if a QVERIFY returns early.
    static QStringList capLines;
    static QList<int>  capLevels;
    capLines.clear();
    capLevels.clear();
    QiLog::setHandler([](QiLog::Level lvl, int, const QString &line) {
        capLevels << lvl;
        capLines  << line;
    });

    // 1) Enabled: statements are captured, timestamped and tagged [SQL ].
    QiLog::enableAll();
    User u; u.userId = "logUser"; u.name = "Log"; u.passwd = "12345678";
    QVERIFY(u.save());
    int c = User::objects().filter( User::col().userId == "logUser" ).count();
    QVERIFY(c == 1);

    bool sawSelect = false, sawSqlTag = false, sawTimestamp = false;
    for (const QString &l : capLines) {
        if (l.contains("SELECT"))  sawSelect = true;
        if (l.contains("[SQL ]"))  sawSqlTag = true;
        if (l.contains("] DEBUG ") || l.contains("] INFO ")) sawTimestamp = true;
    }
    QVERIFY(sawSelect);
    QVERIFY(sawSqlTag);
    QVERIFY(sawTimestamp);

    // 2) Category filter: with only Connection enabled, a SELECT logs no SQL.
    capLines.clear();
    QiLog::setCategories(QiLog::Connection);
    (void) User::objects().filter( User::col().userId == "logUser" ).count();
    for (const QString &l : capLines)
        QVERIFY(!l.contains("[SQL ]"));

    // 3) Level filter: at Error, a successful (Debug) query is silent, but a
    //    failing statement is still captured.
    capLines.clear();
    QiLog::setCategories(QiLog::AllCategories);
    QiLog::setLevel(QiLog::Error);
    (void) User::objects().filter( User::col().userId == "logUser" ).count();
    QVERIFY(capLines.isEmpty());

    User bad; bad.userId = "badUser"; bad.name = "Bad"; bad.passwd = "short"; // < 8 chars
    QVERIFY(!bad.save());   // clean() validation fails -> a Model ERROR line
    bool sawError = false;
    for (const QString &l : capLines)
        if (l.contains("ERROR")) sawError = true;
    QVERIFY(sawError);

    // Restore global logger to its default (off) state.
    QiLog::setHandler(nullptr);
    QiLog::setEnabled(false);
    QiLog::setLevel(QiLog::Debug);
    QiLog::setCategories(QiLog::AllCategories);
}

void SqliteTests::noAutoId() {
    // 1) DDL: a text PRIMARY KEY, no AUTOINCREMENT, and no hidden id column.
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Contact>());
    QVERIFY(sql.contains("contactId TEXT"));
    QVERIFY(sql.contains("PRIMARY KEY"));
    QVERIFY(!sql.contains("AUTOINCREMENT"));         // invalid on a TEXT PK
    QVERIFY(!sql.contains("id INTEGER"));            // no auto id column

    // Column descriptors work for the string key.
    QVERIFY(Contact::col().contactId.toString() == "contactId");

    // 2) Round-trip keyed entirely on the string id.
    Contact a; a.contactId = "01HXULIDAAAA"; a.name = "Ada"; QVERIFY(a.save());
    Contact b; b.contactId = "01HXULIDBBBB"; b.name = "Linus"; QVERIFY(b.save());

    Contact loaded;
    QVERIFY(loaded.load( Contact::col().contactId == "01HXULIDAAAA" ));
    QVERIFY(loaded.name->toString() == "Ada");
    QVERIFY(Contact::objects().count() == 2);

    // 3) save() updates the existing row in place (REPLACE on the string PK).
    a.name = "Ada L.";
    QVERIFY(a.save());
    QVERIFY(Contact::objects().count() == 2);        // not a new row
    Contact reloaded;
    QVERIFY(reloaded.load( Contact::col().contactId == "01HXULIDAAAA" ));
    QVERIFY(reloaded.name->toString() == "Ada L.");

    // 4) remove() works via the declared primary key (no id column).
    QVERIFY(reloaded.remove());
    QVERIFY(Contact::objects().count() == 1);
    QVERIFY(Contact::objects().filter( Contact::col().contactId == "01HXULIDBBBB" ).count() == 1);
}

void SqliteTests::bulkUpdate() {
    User a; a.userId = "bu1"; a.name = "bulkOld"; a.passwd = "12345678"; QVERIFY(a.save());
    User b; b.userId = "bu2"; b.name = "bulkOld"; b.passwd = "12345678"; QVERIFY(b.save());

    // One statement updates both matching rows, no per-row load.
    int changed = User::objects()
                      .filter( User::col().name == "bulkOld" )
                      .update({ {"name", "bulkNew"} });
    QVERIFY(changed == 2);
    QVERIFY(User::objects().filter( User::col().name == "bulkNew" ).count() == 2);
    QVERIFY(User::objects().filter( User::col().name == "bulkOld" ).count() == 0);

    // Empty map is a no-op.
    QVERIFY(User::objects().filter( User::col().name == "bulkNew" ).update({}) == 0);

    (void) User::objects().filter( User::col().name == "bulkNew" ).remove();
}

void SqliteTests::compositeKey() {
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Enrollment>());
    QVERIFY(sql.contains("PRIMARY KEY (studentId, courseId)"));   // table-level
    QVERIFY(!sql.contains("id INTEGER"));                          // no auto id
    QVERIFY(!sql.contains("studentId TEXT NOT NULL PRIMARY KEY")); // not per-column

    Enrollment e1; e1.studentId="s1"; e1.courseId="c1"; e1.grade=90; QVERIFY(e1.save());
    Enrollment e2; e2.studentId="s1"; e2.courseId="c2"; e2.grade=80; QVERIFY(e2.save());
    QVERIFY(Enrollment::objects().count() == 2);

    // Same composite key -> REPLACE in place, not a new row.
    Enrollment e1b; e1b.studentId="s1"; e1b.courseId="c1"; e1b.grade=95; QVERIFY(e1b.save());
    QVERIFY(Enrollment::objects().count() == 2);

    Enrollment loaded;
    QVERIFY(loaded.load( Enrollment::col().studentId == "s1" && Enrollment::col().courseId == "c1" ));
    QVERIFY(loaded.grade->toInt() == 95);

    // remove() matches on the whole composite key.
    QVERIFY(loaded.remove());
    QVERIFY(Enrollment::objects().count() == 1);
    QVERIFY(Enrollment::objects().filter( Enrollment::col().courseId == "c2" ).count() == 1);
}

void SqliteTests::foreignKeyActions() {
    QiSqliteStatement stmt;

    // 1) FK references the target's id and carries the ON DELETE action.
    QString bookSql = stmt.createTableIfNotExists(qiMetaInfo<Book>());
    QVERIFY(bookSql.contains("FOREIGN KEY(author) REFERENCES author(id)"));
    QVERIFY(bookSql.contains("ON DELETE CASCADE"));

    // 2) FK references a custom string primary key (not "id").
    QString memSql = stmt.createTableIfNotExists(qiMetaInfo<Member>());
    QVERIFY(memSql.contains("FOREIGN KEY(team) REFERENCES team(teamId)"));

    // 3) ON DELETE CASCADE actually cascades.
    Author au; au.name = "Ada"; QVERIFY(au.save());
    Book bk; bk.title = "ORM Internals"; bk.author = au; QVERIFY(bk.save());
    QVERIFY(Book::objects().filter( Book::col().title == "ORM Internals" ).count() == 1);
    QVERIFY(au.remove());   // cascades to the book
    QVERIFY(Book::objects().filter( Book::col().title == "ORM Internals" ).count() == 0);

    // 4) FK to a string-PK target: assign stores the key, lazy-load resolves it.
    Team t; t.teamId = "TEAM-X"; t.teamName = "X-Team"; QVERIFY(t.save());
    Member m; m.memberName = "Bob"; m.team = t; QVERIFY(m.save());
    Member ml;
    QVERIFY(ml.load( Member::col().memberName == "Bob" ));
    QVERIFY(ml.team->teamName->toString() == "X-Team");   // resolved via teamId
}

void SqliteTests::checkConstraint() {
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Product>());
    QVERIFY(sql.contains("CHECK (qty >= 0)"));

    Product ok; ok.sku = "SKU1"; ok.qty = 5; QVERIFY(ok.save());
    Product bad; bad.sku = "SKU2"; bad.qty = -1;
    QVERIFY(!bad.save());   // violates CHECK (qty >= 0)
    QVERIFY(Product::objects().count() == 1);
}

void SqliteTests::withoutRowid() {
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Session>());
    QVERIFY(sql.contains("WITHOUT ROWID"));
    QVERIFY(sql.contains("token TEXT NOT NULL PRIMARY KEY"));

    // Round-trips on the string key, same as a normal table.
    Session s; s.token = "tok-abc"; s.owner = "u1"; QVERIFY(s.save());
    Session l;
    QVERIFY(l.load( Session::col().token == "tok-abc" ));
    QVERIFY(l.owner->toString() == "u1");
    QVERIFY(l.remove());
    QVERIFY(Session::objects().count() == 0);
}

void SqliteTests::hasMany() {
    Blog b; b.blogTitle = "Qivot News"; QVERIFY(b.save());
    for (int i = 0; i < 3; i++) {
        Comment c; c.body = QString("c%1").arg(i); c.blog = b; QVERIFY(c.save());
    }
    Blog other; other.blogTitle = "Other"; QVERIFY(other.save());
    Comment oc; oc.body = "elsewhere"; oc.blog = other; QVERIFY(oc.save());

    // One query returns exactly this blog's comments (no per-child load).
    QiList<Comment> comments = qiHasMany<Comment>(b, "blog").all();
    QVERIFY(comments.size() == 3);

    // Composable like any query.
    int n = qiHasMany<Comment>(b, "blog").filter( Comment::col().body == "c1" ).count();
    QVERIFY(n == 1);
}

void SqliteTests::alterColumn() {
    QVERIFY(connect.renameColumn<Migrate>("oldName", "newName"));
    QVERIFY(connect.dropColumn<Migrate>("dropMe"));

    QSqlQuery q = connect.query();
    QVERIFY(q.exec("PRAGMA table_info(migrate)"));
    QStringList cols;
    while (q.next())
        cols << q.value(1).toString();
    QVERIFY(cols.contains("newName"));
    QVERIFY(!cols.contains("oldName"));
    QVERIFY(!cols.contains("dropMe"));
    QVERIFY(cols.contains("keep"));
}

void SqliteTests::enumField() {
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Ticket>());
    QVERIFY(sql.contains("priority INTEGER"));   // enum -> integer column

    Ticket t; t.subject = "bug"; t.priority = PrHigh; QVERIFY(t.save());
    Ticket l;
    QVERIFY(l.load( Ticket::col().subject == "bug" ));
    QVERIFY(l.priority->toInt() == PrHigh);

    // Filter by the enum value.
    QVERIFY(Ticket::objects().filter( Ticket::col().priority == PrHigh ).count() == 1);
    QVERIFY(Ticket::objects().filter( Ticket::col().priority == PrLow ).count() == 0);
}

void SqliteTests::fieldTypeOverride() {
    QiSqliteStatement stmt;
    QString sql = stmt.createTableIfNotExists(qiMetaInfo<Activity>());

    QVERIFY(sql.contains("ticketNumber STRING NOT NULL"));   // raw type + clause
    QVERIFY(sql.contains("billed REAL"));                     // REAL, not DOUBLE
    QVERIFY(!sql.contains("billed DOUBLE"));
    QVERIFY(sql.contains("seq INTEGER"));                     // inferred as usual
    QVERIFY(sql.contains("closedUtc,"));                      // typeless: no type slot
    QVERIFY(!sql.contains("closedUtc TEXT"));

    // Values still round-trip (SQLite's flexible typing).
    Activity a;
    a.ticketNumber = "T-1"; a.billed = 12.5; a.closedUtc = "2026-07-21T00:00:00Z"; a.seq = 1;
    QVERIFY(a.save());
    Activity l;
    QVERIFY(l.load( Activity::col().ticketNumber == "T-1" ));
    QVERIFY(qAbs(l.billed->toDouble() - 12.5) < 1e-9);
    QVERIFY(l.closedUtc->toString() == "2026-07-21T00:00:00Z");
    QVERIFY(l.seq->toInt() == 1);
}

void SqliteTests::jsonComplex() {
    QJsonObject src{
        { "docName", "report" },
        { "meta", QJsonObject{
            { "author", "Ada" },
            { "rev", 3 },
            { "nested", QJsonObject{ { "deep", true } } }
        }},
        { "tags", QJsonArray{ "sql", "orm", "json" } }
    };

    // 1) Deserialize a nested document into a model IN MEMORY — nothing persisted.
    Doc d = QiJsonMapper::map<Doc>(src);
    QVERIFY(d.meta->toJsonObject().value("author").toString() == "Ada");
    QVERIFY(d.meta->toJsonObject().value("nested").toObject().value("deep").toBool());
    QVERIFY(d.tags->toJsonArray().size() == 3);
    QVERIFY(Doc::objects().count() == 0);         // not saved yet

    // 2) Mutate the nested structure in memory, then persist when ready.
    QJsonObject m = d.meta->toJsonObject();
    m["rev"] = 4;
    d.meta = m;
    QVERIFY(d.save());
    QVERIFY(Doc::objects().count() == 1);

    // 3) Reload from the DB — nested structure survives the TEXT round-trip.
    Doc l;
    QVERIFY(l.load( Doc::col().docName == "report" ));
    QVERIFY(l.meta->toJsonObject().value("rev").toInt() == 4);
    QVERIFY(l.meta->toJsonObject().value("nested").toObject().value("deep").toBool());
    QVERIFY(l.tags->toJsonArray().size() == 3);

    // 4) Serialize back to JSON — nested object/array embedded, not stringified.
    QJsonObject out = QiJsonMapper::toJson(&l);
    QVERIFY(out.value("meta").isObject());
    QVERIFY(out.value("meta").toObject().value("rev").toInt() == 4);
    QVERIFY(out.value("tags").isArray());
    QVERIFY(out.value("tags").toArray().size() == 3);

    // 5) pure JSON string <-> object, no database involved.
    QByteArray bytes = QJsonDocument(src).toJson(QJsonDocument::Compact);
    Doc d2 = QiJsonMapper::deserialize<Doc>(bytes);          // JSON text -> object
    QVERIFY(d2.meta->toJsonObject().value("author").toString() == "Ada");
    QVERIFY(d2.tags->toJsonArray().size() == 3);

    QByteArray outBytes = QiJsonMapper::serialize(&d2);       // object -> JSON text
    QJsonObject reparsed = QJsonDocument::fromJson(outBytes).object();
    QVERIFY(reparsed.value("docName").toString() == "report");
    QVERIFY(reparsed.value("meta").toObject().value("author").toString() == "Ada");
    QVERIFY(reparsed.value("meta").toObject().value("nested").toObject().value("deep").toBool());
    QVERIFY(reparsed.value("tags").toArray().size() == 3);
}

