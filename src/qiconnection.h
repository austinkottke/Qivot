#ifndef QiCONNECTION_H
#define QiCONNECTION_H

#include <QObject>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QExplicitlySharedDataPointer>

#include <qimodelmetainfo.h>
#include <qiindex.h>
#include <qiftsindex.h>
#include <qierror.h>

class QiModelMetaInfo;
class QiSql;
class QiConnectionPriv;
template <typename T> inline QiModelMetaInfo* qiMetaInfo();

/// Connection to QSqlDatabase
/**
  DQuest is an ORM library , but it do not interact with database backend(e.g SQLite) directly. Instead
  it just use Qt's own database framework and provides the ORM wrapper interface over the framework.

  In order to provide a more flexible way of operation, user have to create a QSqlDatabase by themself.

  Then the QiConnection will hold the connection information to the QSqlDatabase instance. It will
  store all the supported model , and return a QiSql object to this database for more advanced
  database operation.

  @todo Thread-safe implemention.
  @remarks It is an explicitly shared class


Example code:
\code

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Open database using Qt library
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName( "your.db" );

    db.open();

    // Hold a connection to a database. It is needed before any database access using QiModel.
    QiConnection connection;

    connection.open(db); // Establish the connection to database. It will become the "default connection" shared by all QiModel

    connection.addModel<User>(); // Register a model to the connection

    connection.createTables(); // Create table for all added model if it is not existed.

    ///////////////////
    // Your own code...
    ///////////////////

    connection.close(); // Please close the connection on quit.

    return 0;

}

\endcode

 */

class QiConnection
{
public:
    /// Constructs a new QiConnetion object
    explicit QiConnection();

    /// Constructs a QiConnection which is the reference to other
    QiConnection(const QiConnection& other);

    /// Refer to other QiConnection and returns a reference to this QiConnection
    QiConnection &operator=(const QiConnection &other);

    /// Destructor
    ~QiConnection();

    /// Operator== overloadig
    /** It will return TRUE if they share the same database
     */
    bool operator==(const QiConnection &rhs);

    /// Operator != overloadig
    /** It will return TRUE if they do not share the same database
     */
    bool operator!=(const QiConnection &rhs);

    /// Open the connection to database
    [[nodiscard]] bool open(QSqlDatabase db);

    /// Open the connection to database, controlling default-connection promotion
    /**
      @param db The database to open.
      @param asDefault If TRUE (and no default connection is open yet) this
             connection becomes the shared default connection, as with open(db).
             Pass FALSE for a connection used on a worker thread: it is set up in
             isolation and never reads or mutates the global default connection
             (which is owned by another thread).
     */
    [[nodiscard]] bool open(QSqlDatabase db, bool asDefault);

    /// Close the connection to database
    void close();

    /// Return TRUE if the database is opened,otherwise it is false
    bool isOpen();

    /// Add a model to the connection
    /**
      @return TRUE if it is successful added. False if it is already added or the model is not delcared with QI_DECLARE_MODEL
     */
    template <typename T>
    bool addModel() {
        QiModelMetaInfo* metaInfo = qiMetaInfo<T>();
        return addModel(metaInfo);
    }

    /// Add a model to the connection
    bool addModel(QiModelMetaInfo* metaInfo);

    /// Get the default connection object
    /**
        Default connection is the first opened connection. Any QiConnection instance
        can become the default connection as long as it is the first one to call open().

        It is fine to call defaultConnection() before to open any connection. The result
        returned is still valid and usable after a connection is opened.

        For example,
\code
        QiConnection d = defaultConnection();
        QiConnection c;

        qDebug() << d.isOpen(); // False , as it is not opened.

        c.open(database); // c become the default connection

        qDebug() << d.isOpen(); // The result will become true , both of "c" and "d" are also the default connection.
\endcode
     */
    static QiConnection defaultConnection();

    /// Change this connection to be the default connection
    void setToDefaultConnection();

    /// Run "create table" for all added model.
    /**
      It will run "create table" for all added model if they are not existed. It will also call
      model's initialData() to retrieve the initial data and insert to database.
     */
    [[nodiscard]] bool createTables();

    /// Drop all the tables
    [[nodiscard]] bool dropTables();

    /// Rename a column on a model's table (SQLite 3.25+).
    template <typename T>
    bool renameColumn(const QString &from, const QString &to) {
        return renameColumn(qiMetaInfo<T>(), from, to);
    }
    bool renameColumn(QiModelMetaInfo *metaInfo, const QString &from, const QString &to);

    /// Drop a column from a model's table (SQLite 3.35+).
    template <typename T>
    bool dropColumn(const QString &name) {
        return dropColumn(qiMetaInfo<T>(), name);
    }
    bool dropColumn(QiModelMetaInfo *metaInfo, const QString &name);

    /// Create index
    [[nodiscard]] bool createIndex(const QiBaseIndex &index);

    [[nodiscard]] bool dropIndex(QString name);

    /// Create a full-text search (FTS5) index for a model
    /**
      Builds an FTS5 virtual table over the declared columns and installs
      triggers that keep it in sync with the model's table, then backfills it
      from the existing rows. Requires SQLite built with FTS5.

      @see QiFtsIndex , QiQuery::search
     */
    [[nodiscard]] bool createFtsIndex(const QiBaseFtsIndex &index);

    /// Drop a full-text search index (its triggers and virtual table)
    [[nodiscard]] bool dropFtsIndex(QString name);

    /// Begin a database transaction
    /**
      Wrap a batch of writes in a transaction for atomicity and a large speed-up
      (SQLite otherwise commits — and fsyncs — after every statement). Prefer the
      RAII QiTransaction guard, which rolls back automatically if not committed.
     */
    [[nodiscard]] bool transaction();

    /// Commit the current transaction
    [[nodiscard]] bool commit();

    /// Roll back the current transaction
    bool rollback();

    /// Enable or disable SQLite foreign-key enforcement on this connection
    /**
      SQLite ignores FOREIGN KEY constraints unless this is on. Qivot enables it
      by default when a connection is opened.
     */
    bool setForeignKeysEnforced(bool enabled);

    /// Set the SQLite journal mode (e.g. "WAL", "DELETE", "MEMORY")
    /**
      "WAL" (write-ahead logging) is recommended when readers and writers work
      concurrently (for example the threaded `QiJsonRequest` writing while your
      UI thread reads) — with WAL, readers don't block the writer. WAL is a
      persistent property of the database file.

      @return TRUE if the pragma executed successfully.
     */
    bool setJournalMode(const QString &mode);

    /// Get the SQL interface that you may run predefined sql operations on the database
    QiSql& sql();

    /// Create a QSqlQuery object to the connected database
    QSqlQuery query();

    /// The last query with error used by QiConnection
    /**
      @threadsafe
      @remarks It is thread-safe function
     */
    QSqlQuery lastQuery();

    /// Set the last query
    /// The last query with error used by QiConnection
    /**
      @threadsafe
      @remarks It is thread-safe function
     */
    void setLastQuery(QSqlQuery query);

    /// The error from the last connection-level operation
    /**
      Returns a QiError describing why the last operation (open, createTables,
      transaction, createIndex, createFtsIndex, ...) failed, or a "no error"
      QiError if it succeeded.
     */
    QiError lastError();

signals:

public slots:

protected:

private:
    void setLastError(const QiError &error);

    QExplicitlySharedDataPointer<QiConnectionPriv> d;
};

#endif // QiCONNECTION_H
