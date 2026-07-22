#ifndef QiFIELDREF_H
#define QiFIELDREF_H

#include <cstddef>
#include <qiwhere.h>
#include <qimodelmetainfo.h>

/// A type-safe, refactor-proof reference to a model field, as a QiWhere.
/**
  Instead of hardcoding a column-name string, pass a pointer-to-member. The
  compiler checks the member exists, and renaming the field is caught at compile
  time instead of silently breaking a query at runtime.

\code
    User user;
    user.load( qiField(&User::userId) == "anonymous" );          // no "userId" string

    auto top = User::objects()
                 .filter( qiField(&User::karma) > 100 )
                 .orderBy("karma desc")
                 .all();
\endcode

  Works for any field you declare on the model. For the built-in primary key,
  use `QiWhere("id")` (or the explicit form `qiField<User>(&User::id)`), because
  `id` is inherited from QiModel.

  @param member A pointer to a QiField member of model T (e.g. `&User::userId`).
  @return A QiWhere field for use with the comparison operators, or a null
          QiWhere if the member is not a registered field.
 */
template <typename T, typename M>
inline QiWhere qiField(M T::* member) {
    T probe;
    const size_t offset = reinterpret_cast<size_t>(&(probe.*member))
                        - reinterpret_cast<size_t>(&probe);

    QiModelMetaInfo *info = qiMetaInfo<T>();
    const int n = info->size();
    for (int i = 0; i < n; i++) {
        if (static_cast<size_t>(info->at(i)->offset) == offset)
            return QiWhere(info->at(i)->name);
    }
    return QiWhere();
}

#endif // QiFIELDREF_H
