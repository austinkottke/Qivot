#ifndef QiSQL_H
#define QiSQL_H

#include <QExplicitlySharedDataPointer>
#include <QSqlQuery>
#include <qimodelmetainfo.h>
#include <qiindex.h>
#include <qiftsindex.h>

class QiModelMetaInfo;
class QiSqlStatement;
class QiModel;

class QiSqlStatement;

class QiSqlPriv;

/// A helper class for SQL statement exeuction
/**
   QiSql provides a set of pre-defined SQL operation over the connected database.
   It should be a thread safe object (not verified).

   User are not supposed to use this class except for error checking. User
   may call lastQuery() to retreive the detailed information of last
   query. It is useful to debug SQL level error.

   @remarks The thread safe capability is not verified.
 */

class QiSql
{
public:
    /// Copy constructor
    QiSql(const QiSql&);

    /// Default constructor
    ~QiSql();

    /// Get the associated QiSqlStatement generator
    QiSqlStatement* statement();

    /// The connected database
    QSqlDatabase database();

    /// Run create table of a model
    template <typename T>
    inline bool createTableIfNotExists(){
        QiModelMetaInfo* metaInfo = qiMetaInfo<T>();
        return createTableIfNotExists(metaInfo);
    }

    /// Run create table of a model
    bool createTableIfNotExists(QiModelMetaInfo* info);

    /// Run drop table of a model
    bool dropTable(QiModelMetaInfo* info);

    /// Create index
    bool createIndexIfNotExists(const QiBaseIndex &index);

    /// Drop index
    bool dropIndexIfExists(QString name);

    /// Create a full-text search (FTS5) index
    bool createFtsIndex(const QiBaseFtsIndex &index);

    /// Drop a full-text search index
    bool dropFtsIndex(QString name);

    /// Is the model exists on database?
    bool exists(QiModelMetaInfo* info);

    /// The column names of an existing table (via PRAGMA table_info)
    QStringList columnNames(QiModelMetaInfo* info);

    /// Add a single column to an existing table (ALTER TABLE ADD COLUMN)
    bool addColumn(QiModelMetaInfo* info, const QiModelMetaInfoField* field);

    /// Rename a column: ALTER TABLE ... RENAME COLUMN
    bool renameColumn(QiModelMetaInfo* info, const QString &from, const QString &to);

    /// Drop a column: ALTER TABLE ... DROP COLUMN
    bool dropColumn(QiModelMetaInfo* info, const QString &name);

    /// Insert the reocrd to the database.
    /**
      @param info The meta information of writing model
      @param model The data source
      @param fields A list of fields that should be saved
      @param updateId TRUE if the ID of the model should be updated after operation
     */
    bool insertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool updateId);

    /// Replace the record to the database
    /**
      @param info The meta information of writing model
      @param model The data source
      @param fields A list of fields that should be saved
      @param updateId TRUE if the ID of the model should be updated after operation
     */
    bool replaceInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool updateId);

    /// Upsert the record : insert, or update in place on a conflict key
    /**
      @param info The meta information of writing model
      @param model The data source
      @param fields A list of fields that should be saved
      @param conflictColumns The unique/primary column(s) that identify a row
      @param updateId TRUE if the ID of the model should be updated after operation
     */
    bool upsertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,QStringList conflictColumns,bool updateId);

    /// Insert/replace many records that share the same column set, reusing one prepared statement
    /**
      @param info The meta information of the writing model
      @param models The records to write (all must use the same @p fields)
      @param fields The columns to write
      @param replace TRUE to REPLACE INTO , FALSE to INSERT INTO
      @return TRUE if every record was written.
     */
    bool insertIntoBatch(QiModelMetaInfo* info,const QList<QiModel*>& models,QStringList fields,bool replace);

    /// Begin a transaction on the connected database
    bool transaction();

    /// Commit the current transaction
    bool commit();

    /// Roll back the current transaction
    bool rollback();

    /// Create a query object to the connected database
    QSqlQuery query();

    /// The last query object
    QSqlQuery lastQuery();

protected:
    /**
      @param statement A instance of QiSqlStatement. The ownership will be taken.
     */

    explicit QiSql(QiSqlStatement *statement = 0);

    /// Assignment operator overloading
    QiSql& operator=(const QiSql &rhs);

    /// Set the connected database
    void setDatabase(QSqlDatabase db);

    /// Assign a SQL statement generator
    /**
      @param statement A instance of QiSqlStatement. The ownership will be taken.
     */
    void setStatement(QiSqlStatement *statement);

private:
    void setLastQuery(QSqlQuery query);

    bool insertInto(QiModelMetaInfo* info,QiModel *model,QStringList fields,bool with_id,bool replace);

    QExplicitlySharedDataPointer<QiSqlPriv> d;

    friend class QiConnection;
    friend class QiConnectionPriv;
};

#endif // QiSQL_H
