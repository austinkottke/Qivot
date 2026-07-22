= Tutorial 5 - Create index =

=== Index Declaration ===

Create index for your data model / table is a way to optimize the searching speed. DQuest provides a class, QiIndex, which could be used to create index.

Declaration:
{{{

QiIndex<Model> index("index1"); // Declare a index for "Model" with name of "index1";

index << "field1" << "field2"; // field1 and field2 should be indexed.

}}}

=== Creation ===

QiIndex only contains the information to create index. To made happen on the database, you have to call QiConnection.createIndex() 

Example code:
{{{

/** Demonstration of using QiIndex
 */

#include <QtCore/QCoreApplication>
#include <qivot.h>

/// An example model for people's height and weight data
class HealthCheck : public QiModel {
    QI_MODEL
public:

    QiField<QString> name;
    QiField<int>     height;
    QiField<double>  weight;
    QiField<QDate>   recordDate;
};

QI_DECLARE_MODEL(HealthCheck,
                 "healthcheck",
                 QI_FIELD(name , QiNotNull),
                 QI_FIELD(height),
                 QI_FIELD(weight),
                 QI_FIELD(recordDate)
                 );


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "index.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    connection.open(db); // Establish the connection to database. It will become the "default connection" shared by all QiModel

    connection.addModel<HealthCheck>(); // Register a model to the connection

    connection.createTables(); // Create table for all added model

    QiIndex<HealthCheck> index1("index1"); // Going to create an index called "index1"
    index1 << "height";

    QiIndex<HealthCheck> index2("index2"); // Going to create an index called "index2"
    index2 << "weight";

    QiIndex<HealthCheck> index3("index3"); // Going to create an index called "index3"
    index3 << "height" << "weight"; // Index on height and weight

    connection.createIndex(index1);

    connection.createIndex(index2);

    connection.createIndex(index3);

    connection.close();

    /* index1 and index2 will be created by the above command. You may verify it by using 'sqlite3' command:

$ sqlite3 index.db
SQLite version 3.7.2
Enter ".help" for instructions
Enter SQL statements terminated with a ";"
sqlite> .schema
CREATE TABLE healthcheck  (
id INTEGER PRIMARY KEY AUTOINCREMENT,
name TEXT NOT NULL,
height INTEGER ,
weight DOUBLE ,
recordDate DATE
);
CREATE INDEX index1 on healthcheck (height);
CREATE INDEX index2 on healthcheck (weight);
CREATE INDEX index3 on healthcheck (height,weight);

For how to use index to optimize the query result , please refer to this document:
http://www.sqlite.org/optoverview.html
     */

    return 0;
}

}}}

=== Optimization ===

For the technical detail on how to make use of create index to optimize the query. You may refer to this document for further information:

http://www.sqlite.org/optoverview.html