#ifndef QiRELATION_H
#define QiRELATION_H

#include <QSqlQuery>
#include <QVariant>
#include <QSet>
#include <QHash>
#include <QMultiMap>
#include <QPair>
#include <QMutex>
#include <QMutexLocker>
#include <qiquery.h>
#include <qiconnection.h>
#include <qimodelmetainfo.h>

/** @file qirelation.h
    @brief Reverse ("has many") relations.

    A QiForeignKey walks a relation one way (child -> parent). qiHasMany() walks
    it the other way: given a parent row, it returns a query for every child that
    points back at it — fetched in a single statement instead of one-per-child.

\code
    // every Post whose "author" foreign key references this user
    QiList<Post> posts = qiHasMany<Post>(user, "author").all();

    // compose like any query
    int recent = qiHasMany<Post>(user, "author")
                     .filter( Post::col().remoteId > 100 )
                     .count();
\endcode

    To prefetch children for many parents at once (avoiding N+1 across a list),
    collect the parents' keys and filter the children with a single IN query:

\code
    QList<QVariant> ids;
    for (const User &u : users) ids << u.id();
    QiList<Post> all = QiQuery<Post>().filter( QiWhere("author").in(ids) ).all();
    // ... then group `all` by its author column in memory.
\endcode
 */

/// The primary-key value of a model instance (the built-in `id`, or a declared
/// primary key for a QI_DECLARE_MODEL_NOID model).
template <typename T>
inline QVariant qiKeyOf(T &model) {
    QiModelMetaInfo *info = qiMetaInfo<T>();
    QString pk = info->primaryKeyName();
    return (pk.isEmpty() || pk == QLatin1String("id"))
           ? model.id()
           : info->value(&model, pk);
}

/// A query for every `Child` whose `foreignKeyField` references `parent`.
template <typename Child, typename Parent>
inline QiQuery<Child> qiHasMany(Parent &parent, const QString &foreignKeyField) {
    return QiQuery<Child>().filter( QiWhere(foreignKeyField) == qiKeyOf(parent) );
}

/// Declare a one-to-many relation accessor on a model (the reverse of a foreign
/// key): every `CHILD` whose `FKFIELD` column references this row.
/**
  Returns a composable `QiQuery<CHILD>`, so you can `.all()`, `.count()`, or chain
  `.filter()` / `.orderBy()`.

\code
    class Artist : public QiModel {
        QI_MODEL
    public:
        QiField<QString> name;
        QI_HAS_MANY(Song, songs, "artist")   // artist.songs().all(), .count(), ...
    };
    // Song holds the foreign key:  QiForeignKey<Artist> artist;  (column "artist")
\endcode

  Like QI_MANY_TO_MANY the accessor is a defaulted template, so `CHILD` may be
  forward-declared here and completed at the call site.
 */
#define QI_HAS_MANY(CHILD, NAME, FKFIELD)                                      \
    template <typename _QiChild = CHILD>                                       \
    QiQuery<_QiChild> NAME() {                                                 \
        return qiHasMany<_QiChild>(*this, QStringLiteral(FKFIELD));            \
    }


// --- Many-to-many via a join table -----------------------------------------
//
// A join table has two columns, one referencing each side (e.g. photo_tag with
// photoId, tagId). These helpers link, unlink, and fetch across it.

/// Link `source` and `target` through a join table (INSERT OR IGNORE — safe to
/// repeat). `sourceColumn`/`targetColumn` are the join table's key columns.
template <typename Source, typename Target>
inline bool qiAttach(Source &source, Target &target, const QString &joinTable,
                     const QString &sourceColumn, const QString &targetColumn) {
    QiConnection conn = source.connection();
    QSqlQuery q = conn.query();
    q.prepare(QStringLiteral("INSERT OR IGNORE INTO %1 (%2, %3) VALUES (?, ?)")
                  .arg(joinTable, sourceColumn, targetColumn));
    q.addBindValue(qiKeyOf(source));
    q.addBindValue(qiKeyOf(target));
    const bool ok = q.exec();
    if (ok) conn.notifyChanged(joinTable);
    return ok;
}

/// Remove the link between `source` and `target` from the join table.
template <typename Source, typename Target>
inline bool qiDetach(Source &source, Target &target, const QString &joinTable,
                     const QString &sourceColumn, const QString &targetColumn) {
    QiConnection conn = source.connection();
    QSqlQuery q = conn.query();
    q.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = ? AND %3 = ?")
                  .arg(joinTable, sourceColumn, targetColumn));
    q.addBindValue(qiKeyOf(source));
    q.addBindValue(qiKeyOf(target));
    const bool ok = q.exec();
    if (ok) conn.notifyChanged(joinTable);
    return ok;
}

/// Every `Target` linked to `source` through the join table.
/**
\code
    QiList<Tag> tags = qiManyToMany<Tag>(photo, "photo_tag", "photoId", "tagId");
\endcode
  Resolves in two steps: read the target keys from the join table, then load
  those targets with one IN query.
 */
template <typename Target, typename Source>
inline QiList<Target> qiManyToMany(Source &source, const QString &joinTable,
                                   const QString &sourceColumn, const QString &targetColumn) {
    QiConnection conn = source.connection();
    QSqlQuery q = conn.query();
    q.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE %3 = ?")
                  .arg(targetColumn, joinTable, sourceColumn));
    q.addBindValue(qiKeyOf(source));

    QList<QVariant> ids;
    if (q.exec())
        while (q.next()) ids << q.value(0);
    if (ids.isEmpty())
        return QiList<Target>();

    QiModelMetaInfo *tinfo = qiMetaInfo<Target>();
    QString tpk = tinfo->primaryKeyName();
    if (tpk.isEmpty()) tpk = QStringLiteral("id");
    return QiQuery<Target>().filter( QiWhere(tpk).in(ids) ).all();
}


// --- Batched eager-loading (avoid N+1) --------------------------------------
//
// Calling a relation accessor in a loop runs one query per parent. These
// prefetch helpers load every related row for a whole list of parents in a
// fixed number of queries, then group them in memory.

/// The result of a prefetch: the fetched rows (owned) plus an index from each
/// owner key to its related rows (non-owning pointers into `rows`).
template <typename Target>
struct QiPrefetch {
    QiList<Target> rows;                        ///< all fetched rows (owns them)
    QMultiMap<QString, Target *> index;         ///< ownerKey.toString() -> rows

    /// The related rows for one owner key.
    QList<Target *> forKey(const QVariant &ownerKey) const {
        return index.values(ownerKey.toString());
    }
    int count() const { return rows.size(); }
};

/// One-to-many prefetch: load the children of every parent in ONE query.
/**
\code
    QiList<User> users = User::objects().all();
    QiPrefetch<Post> posts = qiPrefetchHasMany<Post>(users, "author");
    for (int i = 0; i < users.size(); i++)
        QList<Post*> theirs = posts.forKey(users.at(i)->id());   // no extra query
\endcode
 */
template <typename Child, typename Parent>
inline QiPrefetch<Child> qiPrefetchHasMany(QiList<Parent> parents, const QString &fkField) {
    QiPrefetch<Child> out;
    QList<QVariant> keys;
    for (int i = 0; i < parents.size(); i++)
        if (parents.at(i)) keys << qiKeyOf(*parents.at(i));
    if (keys.isEmpty())
        return out;

    out.rows = QiQuery<Child>().filter( QiWhere(fkField).in(keys) ).all();   // one query
    QiModelMetaInfo *info = qiMetaInfo<Child>();
    for (int i = 0; i < out.rows.size(); i++) {
        Child *c = out.rows.at(i);
        if (c) out.index.insert(info->value(c, fkField).toString(), c);
    }
    return out;
}

/// Many-to-many prefetch: resolve the links for a whole list of owners in TWO
/// queries (the join table, then the targets), instead of two per owner.
template <typename Target, typename Owner>
inline QiPrefetch<Target> qiPrefetchManyToMany(QiList<Owner> owners, const QString &joinTable,
                                               const QString &ownerColumn, const QString &targetColumn,
                                               QiConnection conn = QiConnection::defaultConnection()) {
    QiPrefetch<Target> out;
    QList<QVariant> ownerKeys;
    for (int i = 0; i < owners.size(); i++)
        if (owners.at(i)) ownerKeys << qiKeyOf(*owners.at(i));
    if (ownerKeys.isEmpty())
        return out;

    // 1) every link for these owners
    QStringList ph;
    for (int i = 0; i < ownerKeys.size(); i++) ph << QStringLiteral("?");
    QSqlQuery jq = conn.query();
    jq.prepare(QStringLiteral("SELECT %1, %2 FROM %3 WHERE %1 IN (%4)")
                   .arg(ownerColumn, targetColumn, joinTable, ph.join(QStringLiteral(","))));
    for (const QVariant &k : ownerKeys) jq.addBindValue(k);

    QList<QPair<QString, QVariant>> pairs;   // ownerKeyStr -> targetKey
    QList<QVariant> targetKeys;
    if (jq.exec()) {
        while (jq.next()) {
            pairs.append(qMakePair(jq.value(0).toString(), jq.value(1)));
            targetKeys << jq.value(1);
        }
    }
    if (targetKeys.isEmpty())
        return out;

    // 2) every target
    QiModelMetaInfo *tinfo = qiMetaInfo<Target>();
    QString tpk = tinfo->primaryKeyName();
    if (tpk.isEmpty()) tpk = QStringLiteral("id");
    out.rows = QiQuery<Target>(conn).filter( QiWhere(tpk).in(targetKeys) ).all();

    QHash<QString, Target *> byKey;
    for (int i = 0; i < out.rows.size(); i++) {
        Target *t = out.rows.at(i);
        if (t) byKey.insert(tinfo->value(t, tpk).toString(), t);
    }
    for (const QPair<QString, QVariant> &p : pairs) {
        Target *t = byKey.value(p.second.toString(), nullptr);
        if (t) out.index.insert(p.first, t);
    }
    return out;
}


// --- Soft-delete scopes ----------------------------------------------------
// For models with a `deletedAt` column (see QiModel::softRemove()).

/// Query for rows that are NOT soft-deleted (`deletedAt IS NULL`).
template <typename T>
inline QiQuery<T> qiAlive() {
    return QiQuery<T>().filter( QiWhere(QStringLiteral("deletedAt")).is(QVariant()) );
}

/// Query for rows that ARE soft-deleted (`deletedAt IS NOT NULL`).
template <typename T>
inline QiQuery<T> qiTrashed() {
    return QiQuery<T>().filter( QiWhere(QStringLiteral("deletedAt")).isNot(QVariant()) );
}


// --- Declarative many-to-many: a relation collection --------------------------
//
// Declare the relation once on a model with QI_MANY_TO_MANY(...); then work with
// it as a typed collection bound to that row:
//
//   photo.tags().add(tag);          photo.tags().remove(tag);
//   photo.tags().all();             photo.tags().contains(tag);
//   photo.tags().count();           photo.tags().clear();
//   photo.tags().set(newTags);      photo.tags() << tag;
//
// The join table is created automatically on first use, so you don't declare a
// join model or repeat table/column names at every call site.

/// A typed set of `Target` rows linked to one owner row through a join table.
template <typename Target>
class QiRelationSet {
public:
    QiRelationSet(QiConnection conn, QVariant ownerKey, QString joinTable,
                  QString ownerColumn, QString targetColumn)
        : m_conn(conn), m_key(ownerKey), m_join(joinTable),
          m_ownerCol(ownerColumn), m_targetCol(targetColumn) {
        ensureTable();
    }

    /// Every linked Target (one join lookup + one IN query).
    QiList<Target> all() const {
        const QList<QVariant> ids = targetKeys();
        if (ids.isEmpty())
            return QiList<Target>();
        QiModelMetaInfo *info = qiMetaInfo<Target>();
        QString pk = info->primaryKeyName();
        if (pk.isEmpty()) pk = QStringLiteral("id");
        return QiQuery<Target>(m_conn).filter(QiWhere(pk).in(ids)).all();
    }

    /// Number of linked rows.
    int count() const {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("SELECT count(*) FROM %1 WHERE %2 = ?")
                      .arg(m_join, m_ownerCol));
        q.addBindValue(m_key);
        return (q.exec() && q.next()) ? q.value(0).toInt() : 0;
    }

    /// True if `target` is linked to the owner.
    bool contains(Target &target) const {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("SELECT 1 FROM %1 WHERE %2 = ? AND %3 = ? LIMIT 1")
                      .arg(m_join, m_ownerCol, m_targetCol));
        q.addBindValue(m_key);
        q.addBindValue(qiKeyOf(target));
        return q.exec() && q.next();
    }

    /// Link `target` (INSERT OR IGNORE — safe to repeat).
    bool add(Target &target) {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("INSERT OR IGNORE INTO %1 (%2, %3) VALUES (?, ?)")
                      .arg(m_join, m_ownerCol, m_targetCol));
        q.addBindValue(m_key);
        q.addBindValue(qiKeyOf(target));
        return notify(q.exec());
    }

    /// Unlink `target`.
    bool remove(Target &target) {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = ? AND %3 = ?")
                      .arg(m_join, m_ownerCol, m_targetCol));
        q.addBindValue(m_key);
        q.addBindValue(qiKeyOf(target));
        return notify(q.exec());
    }

    /// Unlink everything from this owner.
    bool clear() {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = ?").arg(m_join, m_ownerCol));
        q.addBindValue(m_key);
        return notify(q.exec());
    }

    /// Replace the whole set with exactly `targets`.
    bool set(QiList<Target> targets) {
        if (!clear())
            return false;
        bool ok = true;
        for (int i = 0; i < targets.size(); i++)
            if (targets.at(i))
                ok = add(*targets.at(i)) && ok;
        return ok;
    }

    /// Sugar for add().
    QiRelationSet &operator<<(Target &target) { add(target); return *this; }

private:
    QList<QVariant> targetKeys() const {
        QSqlQuery q = m_conn.query();
        q.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE %3 = ?")
                      .arg(m_targetCol, m_join, m_ownerCol));
        q.addBindValue(m_key);
        QList<QVariant> ids;
        if (q.exec())
            while (q.next()) ids << q.value(0);
        return ids;
    }

    bool notify(bool ok) const {
        if (ok) m_conn.notifyChanged(m_join);   // reactive: live models refresh
        return ok;
    }

    // Create the join table on first touch: two typeless key columns (so an int
    // `id` or a string primary key both work) with a composite primary key.
    void ensureTable() const {
        static QMutex mutex;
        static QSet<QString> ensured;
        {
            QMutexLocker lock(&mutex);
            if (ensured.contains(m_join))
                return;
            ensured.insert(m_join);
        }
        QSqlQuery q = m_conn.query();
        q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS %1 (%2 NOT NULL, %3 NOT NULL, PRIMARY KEY (%2, %3))")
                   .arg(m_join, m_ownerCol, m_targetCol));
    }

    mutable QiConnection m_conn;   // query()/notifyChanged() are non-const
    QVariant     m_key;
    QString      m_join, m_ownerCol, m_targetCol;
};

/// Declare a many-to-many relation accessor on a model.
/**
  Adds a `NAME()` method returning a QiRelationSet<TARGET> bound to this row.
  The join table `JOINTABLE` is created automatically; its columns default by
  convention to `<thisTable>Id` and `<targetTable>Id`.

\code
    class Photo : public QiModel {
        QI_MODEL
    public:
        QiField<QString> title;
        QI_MANY_TO_MANY(Tag, tags, "photo_tag")   // -> photoId / tagId
    };
    // photo.tags().add(tag);  photo.tags().all();  photo.tags() << tag;
\endcode

  Save the owner row (so it has a key) before linking. Use QI_MANY_TO_MANY_AS to
  name the join columns explicitly.
 */
// The accessor is a (defaulted) template so `TARGET`'s completeness is only
// required at the call site, not where the relation is declared — which lets two
// models reference each other bidirectionally over the same join table.
#define QI_MANY_TO_MANY(TARGET, NAME, JOINTABLE)                               \
    template <typename _QiTgt = TARGET>                                        \
    QiRelationSet<_QiTgt> NAME() {                                             \
        return QiRelationSet<_QiTgt>(connection(), qiKeyOf(*this),             \
                   QStringLiteral(JOINTABLE),                                  \
                   tableName() + QStringLiteral("Id"),                         \
                   _QiTgt::TableName() + QStringLiteral("Id"));                \
    }

/// Like QI_MANY_TO_MANY, but with explicit join-table column names.
#define QI_MANY_TO_MANY_AS(TARGET, NAME, JOINTABLE, OWNERCOL, TARGETCOL)       \
    template <typename _QiTgt = TARGET>                                        \
    QiRelationSet<_QiTgt> NAME() {                                             \
        return QiRelationSet<_QiTgt>(connection(), qiKeyOf(*this),             \
                   QStringLiteral(JOINTABLE), QStringLiteral(OWNERCOL),        \
                   QStringLiteral(TARGETCOL));                                 \
    }

#endif // QiRELATION_H
