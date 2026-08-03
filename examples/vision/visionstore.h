#ifndef VISIONSTORE_H
#define VISIONSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QVariantMap>
#include <QVariantList>
#include <QHash>
#include <qilistmodel.h>
#include "models.h"

/// The controller QML talks to. Reports are recomputed with GROUP BY aggregate
/// queries and handed to QML as plain QVariantLists; the posted-labor ledger is a
/// live QiListModel that refreshes itself when the LD table changes. Terminology
/// follows Deltek Vision: Revenue (job-to-date labor value), Compensation (direct
/// labor cost), Profit, and the Effective Multiplier (Revenue ÷ Compensation).
class VisionStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap  metrics      READ metrics      NOTIFY reportsChanged)  // Key Financial Metrics
    Q_PROPERTY(QVariantList projects     READ projects     NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList orgs         READ orgs         NOTIFY reportsChanged)   // by-org rollup
    Q_PROPERTY(QVariantList treemap      READ treemap      NOTIFY reportsChanged)   // project visualization tiles
    Q_PROPERTY(QVariantList staff        READ staff        NOTIFY reportsChanged)
    Q_PROPERTY(QVariantList projectOptions READ projectOptions CONSTANT)
    Q_PROPERTY(QVariantList staffOptions   READ staffOptions   CONSTANT)
    Q_PROPERTY(QAbstractItemModel *ledger READ ledger CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
public:
    explicit VisionStore(QObject *parent = nullptr);

    QVariantMap  metrics() const   { return m_metrics; }
    QVariantList projects() const  { return m_projects; }
    QVariantList orgs() const      { return m_orgs; }
    QVariantList treemap() const   { return m_treemap; }
    QVariantList staff() const     { return m_staff; }
    QVariantList projectOptions() const { return m_projectOptions; }
    QVariantList staffOptions() const   { return m_staffOptions; }
    QAbstractItemModel *ledger()   { return &m_ledger; }
    QString lastError() const      { return m_lastError; }

    /// Post one labor row (PR/EM references, date, hours, rates). Returns false on error.
    Q_INVOKABLE bool postLabor(const QString &wbs1, const QString &employee,
                               const QString &date, double hours,
                               double billRate, double costRate);

    Q_INVOKABLE QString projectName(const QString &wbs1) const { return m_projName.value(wbs1, wbs1); }
    Q_INVOKABLE QString employeeName(const QString &emp) const { return m_empName.value(emp, emp); }
    Q_INVOKABLE QString money(double amount) const;   // "$1,240,000"

signals:
    void reportsChanged();
    void errorChanged();

private:
    void buildCaches();
    void buildOptions();
    void refreshReports();

    QiListModel m_ledger;
    QVariantMap  m_metrics;
    QVariantList m_projects, m_orgs, m_treemap, m_staff, m_projectOptions, m_staffOptions;
    QString      m_lastError;
    QHash<QString, QString> m_projName, m_empName;
};

#endif // VISIONSTORE_H
