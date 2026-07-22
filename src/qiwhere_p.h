#ifndef QiWHERE_P_H
#define QiWHERE_P_H

#include <QVariant>

/// A private database structure used by QiWhere & QiExpression
/** It is used to store the data for special operator.
 */
class QiWhereDataPriv {

public:

    enum Type {
        None,
        In,
        Between
    };

    QiWhereDataPriv();

    QiWhereDataPriv(Type type);

    QiWhereDataPriv& operator<<(QVariant v);

    QList<QVariant> list();

    void setList(QList<QVariant> list);

    Type type();

private:
    QList<QVariant> m_list;
    Type m_type;
};

Q_DECLARE_METATYPE(QiWhereDataPriv)


#endif // QiWHERE_P_H
