/**
    @author Ben Lau
 */

#ifndef QiLISTWRITER_H
#define QiLISTWRITER_H

#include <qilist.h>
#include <qistream.h>

/// QiListWriter is a utility class to create the content on QiList object
/**

  QiListWriter provides a stream interface for writing data to target QiList.

  In the first time you call append() or << operator , QiListWriter inserts a new
  model instance to the target list automatically.

  Then it bind a QiStream object with the model instance, the input from append()
  or << operator is redirected to the model instance through the QiStream
  interface. The stream will be closed when all of the field is written already.

  The next time you call append() or << operator will inserts a new model instance
  and repeat the above process.

  For example:

  Assume you have the following data model:

\code

/// An example model for people's height and weight data
class HealthCheck : public QiModel {
    QI_MODEL
public:

    QiField<QString> name;
    QiField<int>     height;
    QiField<double>  weight;
};

QI_DECLARE_MODEL(HealthCheck,
                 "healthcheck",
                 QI_FIELD(name , QiNotNull),
                 QI_FIELD(height),
                 QI_FIELD(weight)
                 );
\endcode

  You may create the content on QiList by :

\code

    QiList<HealthCheck> list;
    QiListWriter writer(&list);

    writer << "Tester 1" << 179 << 120.5; // list.size() == 1

    writer << "Tester 2" << 160 << 80
           << "Tester 3" << 120 << 60; // list.size() == 3

\endcode

  Sometimes you may not want to set all of the field on the model instance.
  Only the first n-th column are interested. Then you may
  append the result of next() to the stream, the stream will be closed immediately.
  The next append() or trigger will trigger the insertion of new model instance.

Example:

\code
    QiList<HealthCheck> list;
    QiListWriter writer(&list);

    writer << "Tester 1" << 179 << QiListWriter::next()  // weight field is ignored.
           << "Tester 2" << 160 << writer.next() // An alternative to call next().
           << "Tester 3" << 120;  // list.size() == 3

\endcode
  @see QiStream

 */

class QiListWriter
{
public:
    /// Construct a QiListWriter object. Before you can use it for reading or writing , you should call open() to assign a target list.
    QiListWriter();

    /// Construct a QiListwriter operates on target list
    /** This constructor will construct the QiListWriter object and open the target list for reading or writing.
      @see open
     */
    QiListWriter(QiSharedList *list);

    /// Construct a QiListWriter operates on target list with specific database connection
    /**
      This constructor will construct the QiListWriter object and open the target list for reading or writing.
      Moreover, it will set the default database connection for newly created model.

      @see open
      @see setConnection
     */
    QiListWriter(QiSharedList *list,QiConnection connection);

    /// Open and set the target list for writing
    /**
      @param list The target list for writing. The list must be binded with a data model(It's metaInfo() should be non-null)
     */
    bool open(QiSharedList *list);

    /// Get the target list for writing
    QiSharedList *list();

    /// Append a value to data model field
    void append(QVariant value);

    /// Close the writer
    void close();

    /// Append a value to data model field
    /**
      @remarks It is a wrapper of append().
     */
    QiListWriter& operator<< (const QVariant value);

    /// A symbol for stream termination
    /**
      @see next();
     */
    class Next {
    };

    /// Return a symbol to close the stream to current input model.
    /**
      If the result of this function is passed to append() or << operator,
      then the stream is closed immediately. The next call to append() or <<
      operator will insert a new model instance instead of write to
      the orignal writing model.

Example model:

\code
class HealthCheck : public QiModel {
    QI_MODEL
public:

    QiField<QString> name;
    QiField<int>     height;
    QiField<double>  weight;
};

QI_DECLARE_MODEL(HealthCheck,
                 "healthcheck",
                 QI_FIELD(name , QiNotNull),
                 QI_FIELD(height),
                 QI_FIELD(weight)
                 );
\endcode

Example code:
\code
    QiList<HealthCheck> list;
    QiListWriter writer(&list);

    writer << "Tester 1" << 179 << QiListWriter::next()  // weight field is ignored.
           << "Tester 2" << 160 << writer.next() // An alternative to call next().
           << "Tester 3" << 120;  // list.size == 3

\endcode

     */
    static QVariant next();

    /// Set the database connection for newly created model
    /** QiListWriter create new model on the target QiList
      when user append content. It will initialize model's
      connection according to the value set in this function.

      Usually you don't need to use this function unless you
      have multiple database connection in the application.
     */
    void setConnection(QiConnection value);

    /// Get the database connection set for newly created model
    QiConnection connection();

private:

    /// The target list for writing
    QiSharedList *m_list;

    /// The stream for writing to target list
    QiStream m_stream;

    /// The connection set for newly created model instance
    QiConnection m_connection;
};

Q_DECLARE_METATYPE(QiListWriter::Next);

#endif // QiLISTWRITER_H
