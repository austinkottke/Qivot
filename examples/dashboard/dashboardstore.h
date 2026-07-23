#ifndef DASHBOARDSTORE_H
#define DASHBOARDSTORE_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>          // QML_ELEMENT
#include <QFutureWatcher>

/// A step-by-step "playground": each recipe is one small query shown with its
/// code and its live result. The last step runs a query off the UI thread.
class DashboardStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList recipes READ recipes CONSTANT)
    Q_PROPERTY(QString asyncResult READ asyncResult NOTIFY asyncChanged)
    Q_PROPERTY(bool    asyncBusy   READ asyncBusy   NOTIFY asyncChanged)
public:
    explicit DashboardStore(QObject *parent = nullptr);

    QVariantList recipes() const { return m_recipes; }   // [{step, title, code, result}]
    QString asyncResult() const { return m_asyncResult; }
    bool    asyncBusy()   const { return m_asyncBusy; }

    /// Step 8 — run a count on a background thread and show the result.
    Q_INVOKABLE void runAsync();

signals:
    void asyncChanged();

private:
    void buildRecipes();

    QVariantList m_recipes;
    QString m_asyncResult = "(tap Run)";
    bool    m_asyncBusy = false;
    QFutureWatcher<int> m_watcher;
};

#endif // DASHBOARDSTORE_H
