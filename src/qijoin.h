#ifndef QiJOIN_H
#define QiJOIN_H

#include <QString>
#include <QtGlobal>
#include <qiwhere.h>
#include <qiclause.h>
#include <qimodelmetainfo.h>

/// A single JOIN clause in a query
/**
  QiBaseJoin is the base class of QiJoin. It stores the joined table (through
  its QiModelMetaInfo), the join type and the ON condition.

  Normally you do not construct QiBaseJoin directly. Use the typed QiJoin\<T\>
  and pass it to QiSharedQuery::join().

\code
    // SELECT ... FROM user INNER JOIN friendship ON friendship.a = user.id
    QiList<User> list = User::objects()
            .join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) )
            .filter( QiWhere("friendship.b") == tester2.id )
            .all();
\endcode
 */
class QiBaseJoin {
public:
    /// The type of the join
    enum Type {
        Inner = 0,  ///< INNER JOIN
        Left,       ///< LEFT OUTER JOIN
        Right,      ///< RIGHT OUTER JOIN (requires SQLite >= 3.39)
        Full,       ///< FULL OUTER JOIN (requires SQLite >= 3.39)
        Cross       ///< CROSS JOIN (no ON condition)
    };

    /// Construct a null join
    QiBaseJoin() : m_metaInfo(0) , m_type(Inner) {
    }

    /// Construct a join
    /**
      @param metaInfo The meta info of the joined model
      @param on The ON condition. Leave it null for a CROSS JOIN.
      @param type The type of join
     */
    QiBaseJoin(QiModelMetaInfo *metaInfo, QiWhere on , Type type = Inner)
        : m_metaInfo(metaInfo) , m_on(on) , m_type(type) {
    }

    /// Construct a join to a raw table name (not a registered model)
    /**
      Useful for joining tables Qivot does not model, such as an FTS5 virtual
      table. Only an explicit ON condition is supported (no auto foreign-key
      derivation).
     */
    QiBaseJoin(QString table, QiWhere on , Type type = Inner)
        : m_metaInfo(0) , m_table(table) , m_on(on) , m_type(type) {
    }

    /// The meta info of the joined model (null for a raw-table join)
    QiModelMetaInfo *metaInfo() const {
        return m_metaInfo;
    }

    /// The table name of the joined table
    QString table() const {
        if (!m_table.isEmpty())
            return m_table;
        return m_metaInfo ? m_metaInfo->name() : QString();
    }

    /// The ON condition of the join, as supplied by the user (may be null)
    QiWhere on() const {
        return m_on;
    }

    /// The effective ON condition of the join given the primary model
    /**
      If an explicit ON condition was supplied, it is returned unchanged.
      Otherwise (and unless this is a CROSS join) the ON condition is derived
      from a QiForeignKey declared between the primary model and the joined
      model, in either direction:

        - joined model has a foreign key referencing the primary table
              => "<joined>.<fk> = <primary>.id"
        - primary model has a foreign key referencing the joined table
              => "<primary>.<fk> = <joined>.id"

      @param primary The meta info of the query's primary model.
      @return The resolved ON condition, or a null QiWhere if none applies
              (CROSS join, or no foreign key relationship could be found).
     */
    QiWhere resolvedOn(QiModelMetaInfo *primary) const {
        QiWhere explicitOn = m_on;
        if (!explicitOn.isNull())
            return explicitOn;

        if (m_type == Cross || primary == 0 || m_metaInfo == 0)
            return QiWhere();

        QString pTable = primary->name();
        QString jTable = m_metaInfo->name();

        QList<QiWhere> candidates;

        // Case 1: the joined model references the primary model.
        QList<QiModelMetaInfoField> jfks = m_metaInfo->foreignKeyList();
        for (int i = 0 ; i < jfks.size() ; i++) {
            if (foreignKeyTarget(jfks.at(i)) == primary) {
                candidates << ( QiWhere(jTable + "." + jfks.at(i).name)
                                == QiWhere(pTable + ".id") );
            }
        }

        // Case 2: the primary model references the joined model.
        QList<QiModelMetaInfoField> pfks = primary->foreignKeyList();
        for (int i = 0 ; i < pfks.size() ; i++) {
            if (foreignKeyTarget(pfks.at(i)) == m_metaInfo) {
                candidates << ( QiWhere(pTable + "." + pfks.at(i).name)
                                == QiWhere(jTable + ".id") );
            }
        }

        if (candidates.isEmpty()) {
            qWarning("QiJoin: no foreign key relationship found between \"%s\" and \"%s\". "
                     "Provide an explicit ON condition.",
                     qPrintable(pTable) , qPrintable(jTable));
            return QiWhere();
        }

        if (candidates.size() > 1) {
            qWarning("QiJoin: ambiguous foreign key relationship between \"%s\" and \"%s\" "
                     "(%d candidates). Using the first; provide an explicit ON condition "
                     "to disambiguate.",
                     qPrintable(pTable) , qPrintable(jTable) , candidates.size());
        }

        return candidates.first();
    }

    /// The type of the join
    Type type() const {
        return m_type;
    }

    /// The SQL keyword(s) for the join type (e.g. "INNER JOIN")
    QString keyword() const {
        switch (m_type) {
        case Left:  return QStringLiteral("LEFT OUTER JOIN");
        case Right: return QStringLiteral("RIGHT OUTER JOIN");
        case Full:  return QStringLiteral("FULL OUTER JOIN");
        case Cross: return QStringLiteral("CROSS JOIN");
        case Inner:
        default:    return QStringLiteral("INNER JOIN");
        }
    }

private:
    /// Resolve the target model referenced by a foreign key field
    static QiModelMetaInfo *foreignKeyTarget(QiModelMetaInfoField field) {
        QVariant v = field.clause.flag(QiClause::FOREIGN_KEY);
        return (QiModelMetaInfo *) v.value<void *>();
    }

    QiModelMetaInfo *m_metaInfo;
    QString m_table;
    QiWhere m_on;
    Type m_type;
};

/// A typed JOIN clause on model T
/**
  QiJoin\<T\> is a convenience wrapper that resolves the QiModelMetaInfo of the
  model T automatically.

\code
    // INNER JOIN (default)
    query.join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id") ) );

    // LEFT OUTER JOIN
    query.join( QiJoin<FriendShip>( QiWhere("friendship.a") == QiWhere("user.id"),
                                    QiBaseJoin::Left ) );

    // CROSS JOIN (no ON condition)
    query.join( QiJoin<FriendShip>( QiWhere() , QiBaseJoin::Cross ) );
\endcode
 */
template <typename T>
class QiJoin : public QiBaseJoin {
public:
    /// Construct a join to the table of model T
    /**
      @param on   The ON condition of the join. Leave null for a CROSS JOIN.
      @param type The type of join (defaults to INNER JOIN)
     */
    QiJoin(QiWhere on = QiWhere() , Type type = Inner)
        : QiBaseJoin( qiMetaInfo<T>() , on , type ) {
    }
};

#endif // QiJOIN_H
