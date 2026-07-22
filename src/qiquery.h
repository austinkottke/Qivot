#ifndef QiQUERY_H
#define QiQUERY_H

#include <qisharedquery.h>
#include <qilist.h>

///  QiQuery is a template class for performing database queries and record deletion on specific model
/**
  @remarks It is a implicitly shared class
 */
template <typename T>
class QiQuery : public QiSharedQuery{
public:

    /// Construct q QiQuery and use default connection
    QiQuery();

    /// Construct a QiQuery object and set the database connection
    QiQuery(QiConnection connection)  : QiSharedQuery(connection) {
        setMetaInfo(qiMetaInfo<T>());
    }

    /// Copy and convert from a QiSharedQuery instance
    QiQuery(const QiSharedQuery &rhs) : QiSharedQuery(rhs) {
        setMetaInfo(qiMetaInfo<T>());
    }

    /// Copy from other QiQuery instance
    QiQuery& operator=(const QiQuery &rhs ) {
        QiSharedQuery::operator =(rhs);
        setMetaInfo(qiMetaInfo<T>());
        return *this;
    }

    /// Copy and convert from a QiSharedQuery instance
    QiQuery& operator=(const QiSharedQuery &rhs ) {
        QiSharedQuery::operator =(rhs);
        setMetaInfo(qiMetaInfo<T>());
        return *this;
    }

    /* Typed fluent overloads.

       These shadow the QiSharedQuery methods so the whole chain stays a
       QiQuery<T>, and all() returns a QiList<T>. That keeps element access
       typed (list.at(i) -> T*) and lets callers write `auto`:

           auto users = QiQuery<User>().filter(QiWhere("karma") > 0).all();
     */

    /// Construct a new query object with only the given fields in the result
    QiQuery<T> select(QStringList fields) { return QiSharedQuery::select(fields); }

    /// Construct a new query object with only a single field in the result
    QiQuery<T> select(QString field) { return QiSharedQuery::select(field); }

    /// Construct a new query object with an assigned filter
    QiQuery<T> filter(QiWhere where) { return QiSharedQuery::filter(where); }

    /// Construct a new query object with an additional JOIN clause
    QiQuery<T> join(const QiBaseJoin &join) { return QiSharedQuery::join(join); }

    /// Construct a new query object with an automatic JOIN to model M
    template <typename M>
    QiQuery<T> join() { return QiSharedQuery::join<M>(); }

    /// Construct a new query object that removes duplicate rows (SELECT DISTINCT)
    QiQuery<T> distinct(bool enabled = true) { return QiSharedQuery::distinct(enabled); }

    /// Construct a new query object with a limit on the number of results
    QiQuery<T> limit(int val) { return QiSharedQuery::limit(val); }

    /// Construct a new query object that skips the first @p val rows (pagination)
    QiQuery<T> offset(int val) { return QiSharedQuery::offset(val); }

    /// Construct a new query object grouped by the given column(s)
    QiQuery<T> groupBy(QStringList terms) { return QiSharedQuery::groupBy(terms); }

    /// Construct a new query object grouped by a single column
    QiQuery<T> groupBy(QString term) { return QiSharedQuery::groupBy(term); }

    /// Construct a new query object with a HAVING filter on grouped results
    QiQuery<T> having(QiWhere where) { return QiSharedQuery::having(where); }

    /// Construct a new query object with the required sorting order
    QiQuery<T> orderBy(QStringList terms) { return QiSharedQuery::orderBy(terms); }

    /// Construct a new query object with the required sorting order
    QiQuery<T> orderBy(QString term) { return QiSharedQuery::orderBy(term); }

    /// Execute the query and return all the records as a typed QiList<T>
    QiList<T> all() { return QiList<T>( QiSharedQuery::all() ); }

    /// Construct a full-text search query against an FTS5 index
    /**
      Joins the model to its FTS5 index on the row id, filters by the MATCH
      query, and orders by relevance (rank). The index must have been created
      with QiConnection::createFtsIndex.

\code
    QiList<Article> hits = QiQuery<Article>()
            .search("article_fts", "sqlite AND orm")
            .limit(20)
            .all();
\endcode

      @param ftsTable The FTS5 virtual table name (as given to QiFtsIndex).
      @param query    An FTS5 MATCH query string.
     */
    QiQuery<T> search(const QString &ftsTable, const QString &query) {
        QString base = qiMetaInfo<T>()->name();
        QiBaseJoin join( ftsTable ,
                         QiWhere(base + ".id") == QiWhere(ftsTable + ".rowid") );
        return QiSharedQuery::join(join)
                .filter( QiWhere(ftsTable).expr("match", query) )
                .orderBy( ftsTable + ".rank" );
    }

    /// Save the next record to QiModel
    bool recordTo(T &model) {
        return QiSharedQuery::recordTo(&model);
    }

    /// Read the next record
    T record() {
        T t;
        QiSharedQuery::recordTo(&t);
        return t;
    }

};

template <typename T>
QiQuery<T>::QiQuery() : QiSharedQuery() {
    setMetaInfo(qiMetaInfo<T>());
}

#endif // QiQUERY_H
