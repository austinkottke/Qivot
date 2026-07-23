#ifndef PREFETCHSTORE_H
#define PREFETCHSTORE_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>          // QML_ELEMENT

/// Controller that loads authors + their books two ways, counting real SQL.
/**
  `loadNaive()` walks the relation per author (the N+1 problem); `loadPrefetch()`
  uses qiPrefetchHasMany to load everything in 2 queries. A QiLog handler counts
  the actual statements executed so the difference is visible.
 */
class PrefetchStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList authors READ authors  NOTIFY changed)
    Q_PROPERTY(int     queryCount    READ queryCount NOTIFY changed)
    Q_PROPERTY(QString mode          READ mode       NOTIFY changed)
    Q_PROPERTY(double  elapsedMs     READ elapsedMs  NOTIFY changed)
public:
    explicit PrefetchStore(QObject *parent = nullptr);

    QVariantList authors() const { return m_authors; }
    int     queryCount() const { return m_queryCount; }
    QString mode() const { return m_mode; }
    double  elapsedMs() const { return m_elapsed; }

    /// One query for the authors, then one per author for their books (N+1).
    Q_INVOKABLE void loadNaive();
    /// Two queries total: authors, then all books via qiPrefetchHasMany.
    Q_INVOKABLE void loadPrefetch();

signals:
    void changed();

private:
    QVariantList m_authors;
    int     m_queryCount = 0;
    QString m_mode = "—";
    double  m_elapsed = 0;
};

#endif // PREFETCHSTORE_H
