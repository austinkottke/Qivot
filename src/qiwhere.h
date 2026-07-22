#ifndef QiWHERE_H
#define QiWHERE_H

#include <QVariant>

/// The filter rules
/**
   The QiWhere object represent an expression / rules in query for filter the result
   retrived.  It represent the expression used in WHERE sql statement.

   Each QiWhere object contains three parts : The left operand ,
   operator and right operand in an expression.

   Operand in QiWhere can be the field name of database model , a value
   or other QiWhere object with more complex rule.

   The operator compare or mathematical calculation between left and right
   operand. It is stored by QString. Therefore, you may specific the operator
   by yourself using expr() or using the predefined operator in QiWhere. QiWhere
   should have supported most of the operator in SQLite.

   For the complete list of supported operaters, please refer to the document of SQLite

   Reference: http://www.sqlite.org/lang_expr.html
 */

class QiWhere
{
public:
    /// Construct a null QiWhere object
    /** It is the default constructor of QiWhere. Every operand is not set. It is completely null.
     */
    QiWhere();

    /// Construct a QiWhere object with expression
    /**
      @param field The field name in target data model
      @param op The operator in expression
      @param right The right operand in expression

      Example:
\code
    QiWhere filter1("field1","=",1);
    QiWhere filter2("field2","<",2);
\endcode

      @deprecated
      @remarks It is a deprecated way. It is recommended to use QiWhere(QString field) constructor
     */
    QiWhere(QString field,QString op, QVariant right);

    /// Construct a QiWhere with left operand and operator combined in a single string
    /**
      @param fieldAndOp A argument with data model field name and operator combined. It is faster way to construct a QiWhere object.
      @param right The right operand

      Example:

\code
    QiWhere filter1("field1 = ",1);
    QiWhere filter2("field2 <",2);
\endcode

    @deprecated
    @remarks It is a deprecated way. It is recommended to use QiWhere(QString field) constructor
     */
    QiWhere(QString fieldAndOp , QVariant right);

    /// Construct a QiWhere object to represent a data model field
    /** The QiWhere constructed in this way will only contains the
      left operand field to represent a data model field(isField()
      returns true). The operator and right operand are missed.

        To form a valid expression , you have to provide the operator
      and right operand by using other function and/or overloaded operator

\code
    QiWhere field("price"); // The price field in a data model

    qDebug() << field.isField(); // The value is true

    QiWhere filter = field < 3;

    qDebug() << filter.toString(); // The expression is "price < 3"
\endcode

@see expr

     */
    QiWhere(QString field);

    /// Construct a QiWhere object which is a copy of other.
    QiWhere(const QiWhere &other);

    /// Returns true if the object is null; otherwise returns false.
    /** QiWhere is null if no any operend is assigned.
      */
    bool isNull();

    /// Returns true if this object represent a field in data model
    bool isField();

    /// Get the left operand in the expression
    QVariant left();

    /// Get the right operand in the expression
    QVariant right();

    /// Get the operator in the expression
    QString op();

    /// Convert the expression to string
    QString toString();

    /// Ordering term "<field> asc" for orderBy() — use on a column reference
    /** e.g. User::col().karma.asc() yields "karma asc". */
    QString asc();

    /// Ordering term "<field> desc" for orderBy() — use on a column reference
    /** e.g. User::col().karma.desc() yields "karma desc". */
    QString desc();

    /// Form an expression which is the result of doing an operation between "this" and "other"
    /** It is a function mainly used for making an expression where the operator is not supported
      by DQuest yet. You may pass any operator to this function as long as the database
      backend support the operator.

      @param op The operator in string form
      @param right The right operand.
      @return A QiWhere represent the expression

      For the list of supported operator, please check SQLite site: http://www.sqlite.org/lang_expr.html
     */
    QiWhere expr(QString op,QVariant right);

    /// Return a QiWhere object which is the result of "this" and "other"
    QiWhere operator&&(const QiWhere other);

    /// Return a QiWhere object which is the result of "this" or "other"
    QiWhere operator||(const QiWhere other);

    /// Return a QiWhere object which is the expression of "this" < "other"
    QiWhere operator< (QVariant right);

    /// Return a QiWhere object which is the expression of "this" <= "other"
    QiWhere operator<= (QVariant right);

    /// Return a QiWhere object which is the expression of "this" > "other"
    QiWhere operator> (QVariant right);

    /// Return a QiWhere object which is the expression of "this" >= "other"
    QiWhere operator>= (QVariant right);

    /// Return a QiWhere object which is the expression of "this" = "other"
    QiWhere operator==(QVariant right);

    /// Return a QiWhere object which is the expression of "this" <> "other"
    QiWhere operator!=(QVariant right);

    /// Return a QiWhere object which is the expression of "this" + "other"
    QiWhere operator+(QVariant right);

    /// Return a QiWhere object which is the expression of "this" - "other"
    QiWhere operator-(QVariant right);

    /// Return a QiWhere object which is the expression of "this" * "other"
    QiWhere operator*(QVariant right);

    /// Return a QiWhere object which is the expression of "this" / "other"
    QiWhere operator/(QVariant right);

    /// Return a QiWhere object which is the expression of "this" % "other"
    QiWhere operator%(QVariant right);

    /// Return a QiWhere object which is the expression of "this" = "other"
    QiWhere equal(QVariant right);

    /// Return a QiWhere object which is the expression of "this" <> "other"
    QiWhere notEqual (QVariant right);

    /// Return a QiWhere object which is the expression of "this between v1 and v2"
    QiWhere between(QVariant v1,QVariant v2);

    /// Return a QiWhere object which is the expression of "this in (list)"
    QiWhere in (QList<QVariant> list);

    /// Return a QiWhere object which is the expression of "this not in (list)"
    QiWhere notIn (QList<QVariant> list);

    /// Return a QiWhere object which is the expression of "this" "like"  "v"
    QiWhere like (QVariant other);

    /// Return a QiWhere object which is the expression of "this" "glob"  "v"
    QiWhere glob (QVariant other);

    /// Return a QiWhere object which is the expression of "this" "is"  "v"
    QiWhere is (QVariant other);

    /// Return a QiWhere object which is the expression of "this" "is not"  "v"
    QiWhere isNot (QVariant other);

    /// Cast the object to QVariant type
    operator QVariant() const;

private:
    /// left Operand
    QVariant m_left;

    /// The operator
    QString m_op;

    /// right Operand
    QVariant m_right;

    /// Is it null?
    bool m_isNull;

};

Q_DECLARE_METATYPE(QiWhere)

#endif // QiWHERE_H
