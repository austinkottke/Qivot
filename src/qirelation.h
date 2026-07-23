#ifndef QiRELATION_H
#define QiRELATION_H

#include <QSqlQuery>
#include <QVariant>
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

#endif // QiRELATION_H
