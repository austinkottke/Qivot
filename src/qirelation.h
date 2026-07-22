#ifndef QiRELATION_H
#define QiRELATION_H

#include <qiquery.h>
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

/// A query for every `Child` whose `foreignKeyField` references `parent`.
template <typename Child, typename Parent>
inline QiQuery<Child> qiHasMany(Parent &parent, const QString &foreignKeyField) {
    QiModelMetaInfo *pinfo = qiMetaInfo<Parent>();
    QString pk = pinfo->primaryKeyName();
    QVariant key = (pk.isEmpty() || pk == QLatin1String("id"))
                   ? parent.id()
                   : pinfo->value(&parent, pk);
    return QiQuery<Child>().filter( QiWhere(foreignKeyField) == key );
}

#endif // QiRELATION_H
