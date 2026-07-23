#ifndef QiMODEL_H
#define QiMODEL_H

/** @file qimodel.h
    @brief Header file for QiModel

 */

#include <QObject>
#include <QVariant>
#include <QStringList>
#include <qifield.h>
#include <qiconnection.h>
#include <qiwhere.h>
#include <qisharedlist.h>
#include <qiquery.h>
#include <qiforeignkey.h>
#include <qierror.h>
#include <qicolumns.h>

/// Database model class
/** QiModel is the core of DQuest that provide an ORM interface to sql database table.
  To declare your database model, you should:

  <ul>
  <li> Create a class that inherits QiModel </li>
  <li> Added a QI_MODEL macro to the class declaration</li>
  <li> Design your database field by using QiField template type</li>
  <li> Register your model with QI_DECLARE_MODEL macro function. </li>
  </ul>

  QiModel can only access a single record in a time. If you need to query multiple
  record , you should use QiQuery. (You may call YourModel::objects() to obtain
  a QiQuery for current model and connection. )

  QiModel itself do not store used query and error message occur. You should
  call QiConnection.lastQuery to obtain the last query object (QSqlQuery).
  It contains the sql being executed and error found.

  @see QI_MODEL
  @see QI_DECLARE_MODEL
  @see QiQuery
  @see QiList
 */

class QiModel : public QiAbstractModel {

public:
    /// QiModel default constructor
    /** The default constructor will set the connection object to be the
      Default connection. So you should use this constructor for single
      database application

     */
    explicit QiModel();

    /// Construct a QiModel object and set the database connection
    explicit QiModel(QiConnection connection);

    virtual ~QiModel();

    /// The primary key. It is default field for every model
    QiPrimaryKey id;

    /// The table name
    virtual QString tableName() const ;

    /// The table name
    static QString TableName();

    enum { QiModelDefined = 0 };

    /// Change the connection
    void setConnection(QiConnection connection);

    /// Get the current connection
    QiConnection connection();

    /// Save the record to database
    /**
      @param forceInsert TRUE if the data should be inserted to the database as a new record regardless of the original id. The id field will be updated after operation.
      @param forceAllField TRUE if all the field should be saved no matter it is null or not. If false, then null field will be skipped.

      If the id is not set , the record will be inserted to the database , then id field will be updated automatically.
      The successive call will update the record instead of insert unless forceInsert is TRUE.

     */
    bool save(bool forceInsert = false,bool forceAllField = false) override;

    /// Insert the record, or update it in place if it conflicts on a key (upsert)
    /**
      A non-destructive alternative to save(): it runs
      "INSERT INTO ... ON CONFLICT(conflictColumns) DO UPDATE ...", so an
      existing row (identified by the conflict column(s)) is updated in place —
      keeping its primary key — instead of being replaced. Ideal for importing
      records from an API by a natural key on repeated fetches.

      @param conflictColumns The unique/primary column(s) identifying an existing
             row (e.g. a natural key from the source API).
      @param forceAllField TRUE to write all fields even if null.
      @return TRUE on success.

      @remarks Requires SQLite 3.24 or newer.
     */
    bool upsert(const QStringList &conflictColumns, bool forceAllField = false);

    /// Load the record that first match with filter
    bool load(QiWhere where);

    /// Remove the record from database
    /**
      @return TRUE if the record is successfully removed
     */
    [[nodiscard]] bool remove();

    /// Soft-delete: stamp the record's `deletedAt` column with the current time
    /// and save it, instead of DELETEing the row.
    /**
      Requires the model to declare a `deletedAt` field (QiField<QDateTime>).
      The row stays in the table; query live vs. trashed rows with qiAlive<T>()
      / qiTrashed<T>() (see qirelation.h). Returns false (with lastError set) if
      the model has no `deletedAt` field.
     */
    [[nodiscard]] bool softRemove();

    // --- Lifecycle hooks ---------------------------------------------------
    // Override these to run logic around persistence. clean() (above) is the
    // pre-save validation hook; these fire around successful operations.

    /// Called after a successful save()/upsert(). `created` is true for an insert.
    virtual void afterSave(bool created) { Q_UNUSED(created); }

    /// Called before remove(); return false to veto the deletion.
    virtual bool beforeRemove() { return true; }

    /// Called after a successful remove().
    virtual void afterRemove() {}

    /// Called after a successful load() populated this record.
    virtual void afterLoad() {}

    /// Model fields validation
    /**
        This method should be used to provide custom model validation, and to modify attributes on your model if desired.

        If the derived class do not implement their own clean function , it will always return TRUE (No error)

        A clean() override may call setError() to explain why validation failed;
        that message is then available from lastError().

        @see save
        @see setError
        @return TRUE if no error was found.
      */
    virtual bool clean();

    /// The error from the last save() / load() / remove() / upsert() on this record
    /**
      Returns a QiError describing why the last operation failed (or a
      "no error" QiError if it succeeded).

\code
    if (!user.save())
        qWarning() << user.lastError().text();
\endcode
     */
    QiError lastError() const;

    /// The list of initial data which should be inserted to database during table creation. Derived class should override the function to provide their custom record
    /**
      @see QiConnection::createTables
      @see QiListWriter
     */
    virtual QiSharedList initialData() const;

    /// Return a query object to retrieve record from this model
    /** The QI_MODEL macro will add an objects() function for the derived class.
      It will return a QiSharedQuery / QiQuery<T> object instance to query
      data from this data model.

      It provides a faster coding way for you to retrieve data:

      Example code:
\code
    qDebug() << User::objects().call("sum","karma").toInt(); // User is a QiModel.
\endcode
     */
    static QiSharedQuery objects();
    /// Return a query object to retrieve record from this model for specific database connection
    /**
        @param connecion The connection
        It is a overloaded function
     */
    static QiSharedQuery objects(QiConnection connection);

protected:
    /// Set the error for this record (call from a clean() override to explain a validation failure)
    void setError(const QiError &error);

    /// Set a validation error message (convenience for clean() overrides)
    void setError(const QString &message);

private:
    QiConnection m_connection;
    QiError m_error;
};

template<>
class QiModelMetaInfoHelper<QiModel>{
public:
    enum {Defined = 0 };
    /// Return the fields of QiModel
    static inline QList<QiModelMetaInfoField> fields() {
        QList<QiModelMetaInfoField> result;
        QiModel m;
        result << QiModelMetaInfoField("id",
                                       offsetof(QiModel,id),
                                       m.id.type(),
                                       m.id.clause()
                                       );
        return result;
    }
};

/// Declare a database field.
/**
  @param field The name of the field. You don't need to quote the string
  @param CLAUSE The column clause of the field. You may skip this parameter if not needed.
  @remarks This macro should be only used within QI_DECLARE_MODEL / QI_DECLARE_MODEL2

  It expands to a deferred (field, clause) tuple that QI_DECLARE_MODEL consumes
  twice: once to build the runtime metadata, and once to generate the typed
  Model::col() column descriptor. See qicolumns.h.

  @see QiClause
  @see QiNotNull
  @see QiUnique
  @see QiDefault
 */
#define QI_FIELD(field , CLAUSE...) ( field , ##CLAUSE )

/// Declare a field with an explicit SQL column type, overriding the type Qivot
/// would infer from the C++ type.
/**
  @param field  The field name.
  @param sqltype A raw SQL type string (e.g. "REAL", "NUMERIC", "STRING"), or
         "" for a typeless column (no type between the name and its constraints).
  @param CLAUSE Optional column clauses (combine with `|`), as for QI_FIELD.

  \code
    QI_FIELD_AS(billed,    "REAL")                    // REAL instead of DOUBLE
    QI_FIELD_AS(closedUtc, "")                        // typeless column
    QI_FIELD_AS(ticketNo,  "STRING", QiNotNull)       // custom type + clause
  \endcode
 */
#define QI_FIELD_AS(field , sqltype , CLAUSE...) \
    ( field , qiMergeSqlType(QiSqlType(sqltype) , ##CLAUSE) )

/**
  See tests/modes/model1.h
 */
#define QI_DECLARE_MODEL_BEGIN(MODEL,NAME) \
        template<> \
        class QiModelMetaInfoHelper<MODEL> { \
        public: \
            typedef MODEL Table; \
            enum {Defined = 1 }; \
            static inline QString className() { \
                return #MODEL; \
            } \
            static inline QList<QiModelMetaInfoField> fields() {\
                QList<QiModelMetaInfoField> result;\
                MODEL m;

#define QI_DECLARE_MODEL_END(MODEL,NAME) \
                return result; \
            } \
        }; \
        inline QString MODEL::tableName() const { \
            return NAME; \
        } \
        inline QString MODEL::TableName() { \
            return NAME; \
        } \
        inline QiModelMetaInfo *MODEL::metaInfo() const { \
            static QiModelMetaInfo *meta = 0; \
            if (!meta){ \
                meta = qiMetaInfo<MODEL>(); \
            } \
            return meta; \
        } \
        inline auto MODEL::objects() { \
            return QiQuery<MODEL>(); \
        } \
        inline auto MODEL::objects(QiConnection connection) { \
            return QiQuery<MODEL>(connection); \
        } \
        inline QDebug operator<< (QDebug d, const MODEL& model) { \
            d.nospace() << &model; \
            return d.space(); \
        }

/// Declare a model
/**
  For example,

\code

#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<QDateTime> creationDate;
    QiField<qreal> karma;
};

/// Declare the model and the field clause
QI_DECLARE_MODEL(User,
                 "user", // the table name.
                 QI_FIELD(userId , QiNotNull | QiUnique),
                 QI_FIELD(creationDate , QiDefault("CURRENT_TIMESTAMP") ),
                 QI_FIELD(karma)
                 );

\endcode

@see QI_MODEL
@see QI_FIELD
 */
#define QI_DECLARE_MODEL(MODEL,NAME,...) \
        QI_DECLARE_MODEL_BEGIN(MODEL,NAME) \
            result << QiModelMetaInfoHelper<QiModel>::fields(); \
            QiModelMetaInfoField* list[] = { QI_COL_FOR_EACH(QI_COL_EMIT_META, __VA_ARGS__) 0 }; \
            result << _qiMetaInfoCreateFields(list) ; \
        QI_DECLARE_MODEL_END(MODEL,NAME) \
        QI_GEN_COLUMNS(MODEL, __VA_ARGS__)

/// Declare a model that has NO built-in auto-increment `id` column.
/**
  Identical to QI_DECLARE_MODEL, except the hidden
  `id INTEGER PRIMARY KEY AUTOINCREMENT` column is omitted. Mark one of your own
  fields as the primary key with QiPrimary — typically a meaningful string id:

\code
    class Contact : public QiModel {
        QI_MODEL
    public:
        QiField<QString> contactId;
        QiField<QString> name;
    };
    QI_DECLARE_MODEL_NOID(Contact, "contact",
                          QI_FIELD(contactId, QiPrimary | QiNotNull),
                          QI_FIELD(name));
\endcode

  save()/load()/remove() and queries all work off the declared primary key.
  Note: there is no `id` column — `Model::col()` has no `.id`, and such a model
  cannot be an FTS content_rowid table. (Foreign keys reference its declared
  primary key instead of `id`.)

  @see QiPrimary
 */
#define QI_DECLARE_MODEL_NOID(MODEL,NAME,...) \
        QI_DECLARE_MODEL_BEGIN(MODEL,NAME) \
            QiModelMetaInfoField* list[] = { QI_COL_FOR_EACH(QI_COL_EMIT_META, __VA_ARGS__) 0 }; \
            result << _qiMetaInfoCreateFields(list) ; \
        QI_DECLARE_MODEL_END(MODEL,NAME) \
        QI_GEN_COLUMNS_NOID(MODEL, __VA_ARGS__)

/// Declare a model which is not a direct sub-class of QiModel
#define QI_DECLARE_MODEL2(MODEL,NAME,PARENT,...) \
        QI_DECLARE_MODEL_BEGIN(MODEL,NAME) \
            result << QiModelMetaInfoHelper<PARENT>::fields(); \
            QiModelMetaInfoField* list[] = { QI_COL_FOR_EACH(QI_COL_EMIT_META, __VA_ARGS__) 0 }; \
            result << _qiMetaInfoCreateFields(list) ; \
        QI_DECLARE_MODEL_END(MODEL,NAME) \
        QI_GEN_COLUMNS(MODEL, __VA_ARGS__)

/// The QI_MODEL macro must appear in the class definition that declares model's virtual function for database access
/** \def QI_MODEL

  For example,

\code

#include <qivot.h>

/// User account database
class User : public QiModel {
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<QDateTime> creationDate;
    QiField<qreal> karma;
};

/// Declare the model and the field clause
QI_DECLARE_MODEL(User,
                 "user", // the table name.
                 QI_FIELD(userId , QiNotNull | QiUnique),
                 QI_FIELD(creationDate , QiDefault("CURRENT_TIMESTAMP") ),
                 QI_FIELD(karma)
                 );

\endcode

@see QI_DECLARE_MODEL
 */
#define QI_MODEL \
public: \
    enum { QiModelDefined = 1 }; \
    virtual inline QString tableName() const ; \
    static inline QString TableName(); \
    virtual inline QiModelMetaInfo *metaInfo() const; \
    static inline auto objects(); \
    static inline auto objects(QiConnection connection); \
    struct Columns; \
    static Columns col(); \
    template <class T> friend class QiModelMetaInfoHelper;\

/// Print model field for debugging
inline QDebug operator<< (QDebug d, const QiModel* model){
    QiModelMetaInfo *metaInfo = model->metaInfo();
    QStringList fields = metaInfo->fieldNameList();
    QStringList col;
    foreach (QString field,fields){
        QVariant value = metaInfo->value(model,field);
        if (value.isNull())
            continue;
        col << QString("%1=%2").arg(field).arg(value.toString());
    }

    QString res = QString("%1[%2]")
                  .arg(metaInfo->className())
                  .arg(col.join(","));

    d.nospace() << res;

    return d.space();
}

#endif // QiMODEL_H
