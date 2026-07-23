#ifndef QiKEYSET_H
#define QiKEYSET_H

#include <QString>
#include <QVariant>
#include <qiquery.h>
#include <qiwhere.h>
#include <qimodelmetainfo.h>

/// Keyset (a.k.a. cursor / "seek") pagination.
/**
  Pages through an ordered result by remembering the last row's key and asking
  for rows *after* it — `WHERE key > :cursor ORDER BY key LIMIT n` — instead of
  `LIMIT n OFFSET n*page`. Each page is a single index seek, so it stays fast no
  matter how deep you scroll; `OFFSET` re-scans every skipped row and gets slower
  the further you go.

  The key column must be unique and match the sort order — typically the primary
  key. Pass an optional base filter to page through a subset.

\code
    QiKeyset<Event> pager("id", 100);          // key column, page size
    while (!pager.atEnd()) {
        QiList<Event> page = pager.next();
        for (int i = 0; i < page.size(); i++) { ... }
    }

    // Save pager.cursor() and later resume exactly where you left off:
    pager.seek(savedCursor);
\endcode
 */
template <typename T>
class QiKeyset {
public:
    /// @param keyField    unique column to seek on (e.g. "id")
    /// @param pageSize    rows per page
    /// @param ascending   sort/seek direction
    /// @param baseFilter  optional condition ANDed into every page
    explicit QiKeyset(const QString &keyField, int pageSize = 50,
                      bool ascending = true, QiWhere baseFilter = QiWhere())
        : m_key(keyField), m_size(pageSize > 0 ? pageSize : 50),
          m_asc(ascending), m_base(baseFilter) {}

    /// Fetch the next page (empty once exhausted).
    QiList<T> next() {
        if (m_atEnd)
            return QiList<T>();

        QiWhere cond = m_base;
        if (m_hasCursor) {
            QiWhere seek = m_asc ? (QiWhere(m_key) > m_cursor)
                                 : (QiWhere(m_key) < m_cursor);
            cond = m_base.isNull() ? seek : (m_base && seek);
        }

        QiQuery<T> q;
        if (!cond.isNull())
            q = q.filter(cond);
        q = q.orderBy(m_key + (m_asc ? QStringLiteral(" asc") : QStringLiteral(" desc")))
             .limit(m_size);

        QiList<T> page = q.all();
        if (page.size() < m_size)
            m_atEnd = true;
        if (page.size() > 0) {
            QiModelMetaInfo *info = qiMetaInfo<T>();
            m_cursor = info->value(page.at(page.size() - 1), m_key);
            m_hasCursor = true;
        }
        return page;
    }

    /// True once a short/empty page proved there's nothing more.
    bool atEnd() const { return m_atEnd; }

    /// The last key seen — a compact, stable cursor you can persist and resume.
    QVariant cursor() const { return m_cursor; }

    /// Resume paging from a saved cursor (the next page starts after `key`).
    void seek(const QVariant &key) { m_cursor = key; m_hasCursor = true; m_atEnd = false; }

    /// Start over from the beginning.
    void reset() { m_cursor = QVariant(); m_hasCursor = false; m_atEnd = false; }

private:
    QString  m_key;
    int      m_size;
    bool     m_asc;
    QiWhere  m_base;
    QVariant m_cursor;
    bool     m_hasCursor = false;
    bool     m_atEnd = false;
};

#endif // QiKEYSET_H
