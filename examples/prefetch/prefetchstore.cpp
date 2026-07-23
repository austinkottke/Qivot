#include "prefetchstore.h"
#include "models.h"
#include <QElapsedTimer>
#include <qirelation.h>
#include <qilog.h>

// Counts every SQL statement Qivot executes (installed once, below).
static int g_sqlCount = 0;

PrefetchStore::PrefetchStore(QObject *parent) : QObject(parent) {
    QiLog::setEnabled(true);
    QiLog::setCategories(QiLog::Sql);
    QiLog::setHandler([](QiLog::Level, int category, const QString &) {
        if (category & QiLog::Sql) g_sqlCount++;
    });
    loadPrefetch();   // start on the good path
}

void PrefetchStore::loadNaive() {
    QElapsedTimer t; t.start();
    g_sqlCount = 0;

    QiList<Author> authors = Author::objects().orderBy(Author::col().name.asc()).all();  // 1 query
    m_authors.clear();
    for (int i = 0; i < authors.size(); i++) {
        Author *a = authors.at(i);
        QiList<Book> books = qiHasMany<Book>(*a, "author").all();                        // +1 each
        QStringList titles;
        for (int j = 0; j < books.size(); j++) titles << books.at(j)->title->toString();
        QVariantMap m;
        m["name"] = a->name->toString();
        m["count"] = titles.size();
        m["titles"] = titles;
        m_authors << m;
    }

    m_elapsed = t.nsecsElapsed() / 1e6;
    m_queryCount = g_sqlCount;
    m_mode = QString("N+1  (1 + %1 queries)").arg(authors.size());
    emit changed();
}

void PrefetchStore::loadPrefetch() {
    QElapsedTimer t; t.start();
    g_sqlCount = 0;

    QiList<Author> authors = Author::objects().orderBy(Author::col().name.asc()).all();  // 1 query
    QiPrefetch<Book> books = qiPrefetchHasMany<Book>(authors, "author");                 // 1 query
    m_authors.clear();
    for (int i = 0; i < authors.size(); i++) {
        Author *a = authors.at(i);
        const QList<Book *> theirs = books.forKey(a->id());
        QStringList titles;
        for (Book *b : theirs) titles << b->title->toString();
        QVariantMap m;
        m["name"] = a->name->toString();
        m["count"] = titles.size();
        m["titles"] = titles;
        m_authors << m;
    }

    m_elapsed = t.nsecsElapsed() / 1e6;
    m_queryCount = g_sqlCount;
    m_mode = "Prefetch  (2 queries)";
    emit changed();
}
