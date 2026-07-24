#ifndef SEARCHSTORE_H
#define SEARCHSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QQmlEngine>        // QML_ELEMENT
#include <qilistmodel.h>

/// Runs full-text searches and exposes the ranked results (plus timing) to QML.
class SearchStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *results READ results CONSTANT)
    Q_PROPERTY(int     totalCount READ totalCount CONSTANT)          // rows in the corpus
    Q_PROPERTY(int     matchCount READ matchCount NOTIFY statsChanged)
    Q_PROPERTY(int     shown      READ shown      NOTIFY statsChanged)
    Q_PROPERTY(double  elapsedMs  READ elapsedMs  NOTIFY statsChanged)
    Q_PROPERTY(QString terms      READ terms      NOTIFY statsChanged) // for highlighting in QML
public:
    explicit SearchStore(QObject *parent = nullptr);

    QAbstractItemModel *results() { return &m_model; }
    int     totalCount() const { return m_total; }
    int     matchCount() const { return m_match; }
    int     shown()      const { return m_model.count(); }
    double  elapsedMs()  const { return m_ms; }
    QString terms()      const { return m_terms; }

    /// Run a search. Called from QML on every keystroke (debounced there).
    Q_INVOKABLE void search(const QString &text);

signals:
    void statsChanged();

private:
    static QString toFtsQuery(const QString &text);   // user text -> safe FTS5 prefix query

    QiListModel m_model;
    int    m_total = 0;
    int    m_match = 0;
    double m_ms    = 0.0;
    QString m_terms;

    static constexpr int kMaxRows = 200;   // how many ranked hits we display
};

#endif // SEARCHSTORE_H
