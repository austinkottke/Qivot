#include "searchstore.h"
#include "doc.h"

#include <QElapsedTimer>

SearchStore::SearchStore(QObject *parent) : QObject(parent) {
    m_total = Doc::objects().count();   // full corpus size, shown as "of N"
}

// Turn whatever the user typed into a safe FTS5 query. We keep only letters and
// digits, and append '*' to each word so it matches as you type ("oce" finds
// "ocean"). Words are space-separated, which FTS5 treats as AND.
QString SearchStore::toFtsQuery(const QString &text) {
    QStringList terms;
    QString cur;
    const QString lower = text.toLower();
    for (int i = 0; i <= lower.size(); i++) {
        const QChar c = i < lower.size() ? lower.at(i) : QChar(' ');
        if (c.isLetterOrNumber()) {
            cur += c;
        } else if (!cur.isEmpty()) {
            terms << cur + "*";     // prefix match
            cur.clear();
        }
    }
    return terms.join(' ');
}

void SearchStore::search(const QString &text) {
    m_terms = text.trimmed();
    const QString q = toFtsQuery(text);

    if (q.isEmpty()) {                       // empty box -> clear results
        m_model.setList(QiList<Doc>());
        m_match = 0;
        m_ms = 0.0;
        emit statsChanged();
        return;
    }

    QElapsedTimer timer;
    timer.start();

    // Ranked hits (best matches first), capped for display. search() joins the
    // FTS5 table, filters by MATCH, and orders by relevance (bm25 rank).
    QiList<Doc> hits = QiQuery<Doc>().search("doc_fts", q).limit(kMaxRows).all();

    // Total number of matches across the whole corpus (may be far more than we show).
    m_match = QiQuery<Doc>().search("doc_fts", q).count();

    m_ms = timer.nsecsElapsed() / 1.0e6;
    m_model.setList(hits);
    emit statsChanged();
}
