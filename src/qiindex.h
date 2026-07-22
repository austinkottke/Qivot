#ifndef QiINDEX_H
#define QiINDEX_H

#include <QObject>
#include <QString>
#include <qimodelmetainfo.h>

/// The based class of QiIndex
class QiBaseIndex
{
public:
    /// The default constructor.
    /**
       @remarks It will set the parent of this object to QCoreApplication automatically
     */
    QiBaseIndex(QiModelMetaInfo* metaInfo, QString name);

    /// Get the associated model's meta info object
    const QiModelMetaInfo* metaInfo() const;

    /// The index name
    QString name() const;

    /// Get the list of fields
    QStringList columnDefList() const;

    /// Set the column definition list
    void setColumnDefList(QStringList columnDefList);

    /// Append column definition through operator<< overloading
    QiBaseIndex& operator<<(QString columnDef);

    /// Make this a UNIQUE index (CREATE UNIQUE INDEX)
    QiBaseIndex& setUnique(bool unique = true);

    /// TRUE if this is a UNIQUE index
    bool isUnique() const;

    /// Restrict the index to rows matching a condition (a partial index)
    /**
      Emits "... WHERE <condition>". Pass a raw SQL predicate, e.g.
      "active = 1".
     */
    QiBaseIndex& setWhere(QString condition);

    /// The partial-index WHERE condition (empty for a full index)
    QString where() const;

protected:

private:
    QiModelMetaInfo* m_metaInfo;

    QString m_name;
    QStringList m_columnDefList;
    bool m_unique;
    QString m_where;
};

/// SQL Indexing information
/** QiIndex is a data sturctore to create index on SQL database.
  You have to declare you index using QiIndex then create it
  by QiConnection.

\code

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

\endcode

For how to use index to optimize the query result , please refer to this document:
http://www.sqlite.org/optoverview.html

 */
template <typename T>
class QiIndex : public QiBaseIndex {
public:

    /// Construct a QiIndex object for specific data model
    QiIndex(QString name) : QiBaseIndex(qiMetaInfo<T>() , name)  {
    }


};

#endif // QiINDEX_H
