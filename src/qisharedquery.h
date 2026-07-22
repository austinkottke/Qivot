#ifndef QiSHAREDQUERY_H
#define QiSHAREDQUERY_H

#include <QSharedDataPointer>
#include <QExplicitlySharedDataPointer>
#include <qiconnection.h>
#include <qiwhere.h>
#include <qimodelmetainfo.h>
#include <qisharedlist.h>
#include <qijoin.h>

class QiSharedQueryPriv;
class QiConnection;
class QiWhere;

/// QiSharedQuery is the base class of QiQuery that support implicitly data sharing
/**
  QiSharedQuery is the base class of QiQuery that hold all the information for a query.
  It is a implicitly shared class which is designed for query data exchange between
  different objects.

  Although most of the function return QiSharedQuery instead of QiQuery , user should
  always use QiQuery.  They are exchangable, that means QiSharedQuery can be converted
  to QiQuery, and vice visa.

  @remarks It is a implicitly shared class
 */

class QiSharedQuery
{
public:

    /// Construct a QiSharedQuery object and use the default database connection
    QiSharedQuery();

    /// Construct a QiSharedQuery object and set the database connection
    QiSharedQuery(QiConnection connection);

    /// Copy constructor
    QiSharedQuery(const QiSharedQuery &);

    /// Assignment operator overloading
    QiSharedQuery &operator=(const QiSharedQuery &);

    /// Default destructor
    ~QiSharedQuery();

    /// Set the connection
    void setConnection(QiConnection connection);

    /// Construct a new query object with only the fields assigned in result
    /**
        By default, QiSharedQuery retrieve all the field of the model.
        Call this function if you only interest for specific fields.

        @remarks If you don't specific the "id" field. It will not load the field. If you save the model , it will insert a new entity to the database.
     */
    QiSharedQuery select(QStringList fields);

    /// Construct a new query object with only a single field in result
    /**
        By default, QiSharedQuery retrieve all the field of the model.
        Call this function if you only interest for a single field

        @remarks If the field is not the "id" field, and you call the save() method on the model. It will insert a new entity to the database
     */
    QiSharedQuery select(QString field);

    /// Construct a new query object with assigned filter
    QiSharedQuery filter(QiWhere where);

    /// Construct a new query object with an additional JOIN clause
    /**
      The joined table becomes available to the filter() and ON conditions.
      You may qualify column names with the table name (e.g. "user.id") to
      disambiguate between the joined tables. The query still returns records
      of the primary model.

      @param join The join clause, normally a typed QiJoin\<T\> instance.

      Example:
\code
    // SELECT ... FROM user INNER JOIN friendship ON friendship.a = user.id
    //            WHERE friendship.b = <tester2.id>
    QiList<User> friends = User::objects()
            .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) )
            .filter( QiWhere("friendship.b") == tester2.id )
            .all();
\endcode

      @see QiJoin
     */
    QiSharedQuery join(const QiBaseJoin &join);

    /// Construct a new query object with an automatic JOIN to model T
    /**
      The ON condition is derived from a QiForeignKey declared between the
      primary model and T (in either direction). It is a shortcut for
      join( QiJoin\<T\>() ).

\code
    // Config declares QiForeignKey<User> uid , so the ON condition
    // "user.id = config.uid" is derived automatically.
    QiList<Config> list = Config::objects()
            .join<User>()
            .filter( QiWhere("user.userId") == "tester1" )
            .all();
\endcode

      @remarks If the relationship is ambiguous (e.g. two foreign keys to the
      same table), pass an explicit ON condition with QiJoin\<T\> instead.
     */
    template <typename T>
    QiSharedQuery join() {
        return join( QiJoin<T>() );
    }

    /// Construct a new query object that removes duplicate rows (SELECT DISTINCT)
    /**
      This is most useful together with join(), where a single primary record
      may be produced several times (once per matching row in the joined table).

      @param enabled Pass false to turn DISTINCT off again.
     */
    QiSharedQuery distinct(bool enabled = true);

    /// Construct a new query object with limitation no. of result
    QiSharedQuery limit(int val);

    /// Construct a new query object that skips the first @p val rows (for pagination)
    QiSharedQuery offset(int val);

    /// Construct a new query object grouped by the given column(s) (GROUP BY)
    QiSharedQuery groupBy(QStringList terms);

    /// Construct a new query object grouped by a single column (GROUP BY)
    QiSharedQuery groupBy(QString term);

    /// Construct a new query object with a HAVING filter on grouped results
    QiSharedQuery having(QiWhere where);

    /// Construct a new query object with required sorting order
    /**
      @param terms The ordering terms

      The ordering terms is basically equal to the field name.
      You may add the clause ASC / DESC next to the field name,
      then it will be sorted by asc / desc ordering.

      Example:
\code
    QiQuery<HealthCheck> query;

    QiList<HealthCheck> result;

    result = query.orderBy("height").all();
    result = query.orderBy("height asc").all();
    result = query.orderBy("height desc").all();

\endcode

     */
    QiSharedQuery orderBy(QStringList terms);

    /// Construct a new query object with required sorting order
    /**
      It is a overloaded function
     */
    QiSharedQuery orderBy(QString term);

    /// Execute the query
    [[nodiscard]] bool exec();

    /// Retrieves the next record in the result, if available, and positions the query on the retrieved record.
    bool next();

    /// Retrieves the first field of the current record. It is useful for query like count() / max() , ... that will only have a single field result
    QVariant value();

    /// Retrieves the field at @p index of the current record
    /**
      Useful for reading grouped/aggregate results, e.g. a
      `select({"author", "count(*)"})` + `groupBy("author")` query where each
      row has the group column at index 0 and the aggregate at index 1.
     */
    QVariant value(int index);

    /// Execute the query and count no. of record retrieved
    int count();

    /// Execute the query and call specific function on the result
    /**
      @param func The function name (e.g sum , avg , ...)
      @param fields A list of fields that could be passed to the function
     */
    QVariant call(QString func , QStringList fields = QStringList());

    /// Execute the query and call specific function on the result
    /**
      @param func The function name (e.g sum , avg , ...)
      @param field The field that should be passed to the function
     */
    QVariant call(QString func , QString field);

    /// Delete all the records fullfill the filter rules
    /**
      @return TRUE if the operation is successfully run , otherwise it is false.
     */
    [[nodiscard]] bool remove();

    /// Update all records matching the filter in a single UPDATE statement
    /**
      Sets the given columns on every row that matches the query's filter,
      without loading them:

\code
    int changed = User::objects()
                      .filter( User::col().karma < 0 )
                      .update({ {"karma", 0}, {"name", "reset"} });
\endcode

      @param values Column name -> new value.
      @return The number of rows updated, or -1 on error.
     */
    int update(const QVariantMap &values);

    /// Execute the query and return all the record retrieved
    QiSharedList all();

    /// Returns the QSqlQuery object being used
    QSqlQuery lastQuery();

    /// Reset the query to initial status , but keep the connection and associated object unchanged.
    void reset();

protected:

    /// Set the associated data model
    void setMetaInfo(QiModelMetaInfo *info);

    /* The design of QiSharedQuery do not allow user to pass QiModel to any argument.
       Prevent invalid pointer type passed
     */

    /// Save the current record to a QiModel
    bool recordTo(QiAbstractModel *model);

    /// Get a single record that first match with the filter (limit(1) will be set)
    /**
      @param model The record will be saved to the object
      @return TRUE if success , the result will be saved to the model argument. Otherwise it is false.
     */
    bool get(QiAbstractModel* model);


private:
    QSharedDataPointer<QiSharedQueryPriv> data;

    friend class QiQueryRules;
};

#endif // QiSHAREDQUERY_H
