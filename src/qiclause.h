/**
 * @author Ben Lau
 */

#ifndef QiCLAUSE_H
#define QiCLAUSE_H

/** @file
  @brief Header file for QiClause

 */

#include <QList>
#include <QVariant>
#include <QVector>

/// The clause of a column definition
/** QiClause is a data structure to describe the clause of column definition for "create table".
  It is used in model declaration (The second argument in QI_FIELD macro)

  Example:
  \code
class User : public QiModel
{
    QI_MODEL
public:
    QiField<QString> userId;
    QiField<QString> name;

    QiField<QString> passwd;

    QiField<QDateTime> creationTime;
    QiField<QDateTime> lastLoginTime;

};

QI_DECLARE_MODEL( User,
                  "user",
                  QI_FIELD(userId, QiClause(QiClause::UNIQUE) | QiClause(QiClause::NOT_NULL) ), // Unique and Not null
                  QI_FIELD(name),
                  QI_FIELD(passwd , QiClause(QiClause::NOT_NULL)),
                  QI_FIELD(creationTime,QiClause(QiClause::DEFAULT,"CURRENT_TIMESTAMP") ),
                  QI_FIELD(lastLoginTime)
                  );

  \endcode

The declaration is equivalent to make this SQL table for SQLITE

  \code

    CREATE TABLE user  (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        userId TEXT NOT NULL UNIQUE,
        name TEXT ,
        passwd TEXT NOT NULL,
        creationTime DATETIME DEFAULT CURRENT_TIMESTAMP ,
        lastLoginTime DATETIME
    );
  \endcode

   @remarks Usually you won't use QiClause directly. There have wrapper macro for common clause available.

    @see QiNotNull
    @see QiDefault
    @see QiUnique

  */

/// Referential action for a foreign key's ON DELETE clause (see QiForeignKey).
enum QiFkAction {
    QiFkNoAction = 0,   ///< no ON DELETE clause (default)
    QiFkCascade,        ///< ON DELETE CASCADE
    QiFkRestrict,       ///< ON DELETE RESTRICT
    QiFkSetNull,        ///< ON DELETE SET NULL
    QiFkSetDefault      ///< ON DELETE SET DEFAULT
};

class QiClause
{
public:
    /// Supported type of clause
    enum Type{
        UNIQUE = 0,
        DEFAULT,
        NOT_NULL,

        /* Extra */
        PRIMARY_KEY,
        FOREIGN_KEY,
        FK_ON_DELETE,   ///< value: a QiFkAction (ON DELETE ... referential action)
        CHECK,          ///< value: a CHECK constraint expression (QString)
        SQL_TYPE,       ///< value: a raw SQL column type override (QString; "" = typeless)
        LAST
    };

    /// Constructs a null QiClause
    QiClause();

    // Copy/move are compiler-generated (rule of zero): the only state is
    // m_flags. A hand-written copy ctor here would suppress the move ctor and
    // trigger -Wdeprecated-copy under Qt 6's QList.

    /// Constructs a QiClause and set the clause type
    QiClause(QiClause::Type type);

    /// Constructs a QiClause and set the clause type and its value
    QiClause(Type type, QVariant value);

    /// TRUE if this type of clause flag is set
    bool testFlag(Type);

    /// Get the value of the clause type
    QVariant flag(Type);

    /// Set the clause type with its value
    void setFlag(Type type,QVariant val = true);

    /// | operation overloading
    /**
      @remarks If both of the flag is set in two clause object. It will use the value specified in second clause
     */
    QiClause operator|(const QiClause& other);

private:
    void init();
    QMap<Type,QVariant> m_flags;
};

/// "Unique" clause
/** The database field should be unique. No other record share the same value.

  e.g In a data model of user information , userId should be unique.
 */
#define QiUnique QiClause(QiClause::UNIQUE)

/// "NotNull" clause
/** This field must be set. Otherwise it will reject the write.
 */
#define QiNotNull QiClause(QiClause::NOT_NULL)

/// The "Default" clause
/** Assign the default value of the field. If it is not set,
  then it will use the assigned value automatically.
 */
#define QiDefault(value) QiClause(QiClause::DEFAULT,value)

/// "Primary key" clause
/** Marks a declared field as the table's PRIMARY KEY. Use it together with
  QI_DECLARE_MODEL_NOID so the model does not also get the built-in auto-`id`
  column — ideal for tables keyed by a meaningful string id (a ULID, a userId):

  \code
    QI_DECLARE_MODEL_NOID(Contact, "contact",
                          QI_FIELD(contactId, QiPrimary | QiNotNull),  // TEXT PRIMARY KEY
                          QI_FIELD(name));
  \endcode

  AUTOINCREMENT is emitted only for an INTEGER primary key; a text primary key
  becomes a plain `PRIMARY KEY`.
 */
#define QiPrimary QiClause(QiClause::PRIMARY_KEY)

/// "Check" constraint
/** Emits a `CHECK (<expr>)` column constraint. The expression is raw SQL
  (validated by SQLite on insert/update):

  \code
    QI_FIELD(age, QiCheck("age >= 0"))
    QI_FIELD(status, QiCheck("status IN ('open','closed')"))
  \endcode
 */
#define QiCheck(expr) QiClause(QiClause::CHECK, expr)

/// Override the emitted SQL column type (see QI_FIELD_AS). An empty string emits
/// a typeless column (SQLite BLOB affinity — the type slot is skipped entirely).
#define QiSqlType(sqltype) QiClause(QiClause::SQL_TYPE, QString(sqltype))

/// Merge a type-override clause with the (optional) user-supplied clause. Lets
/// QI_FIELD_AS accept `QI_FIELD_AS(field, "TYPE")` and
/// `QI_FIELD_AS(field, "TYPE", QiNotNull | QiUnique)` through one macro.
inline QiClause qiMergeSqlType(QiClause typeClause) { return typeClause; }
inline QiClause qiMergeSqlType(QiClause typeClause, QiClause extra) { return typeClause | extra; }

/// Encode the string
QString qiEscape(QString val,bool trimStrings = false);

#endif // QiCLAUSE_H
