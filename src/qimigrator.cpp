#include <algorithm>
#include <QSqlQuery>
#include <QSqlError>
#include "qimigrator.h"
#include "qilog.h"

QiMigrator::QiMigrator(QiConnection connection)
    : m_conn(connection) {
}

void QiMigrator::add(int version, const QString &name, Step step) {
    m_steps.append({ version, name, std::move(step) });
}

int QiMigrator::currentVersion() const {
    QSqlQuery q = m_conn.query();
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return 0;
}

int QiMigrator::targetVersion() const {
    int t = 0;
    for (const Entry &e : m_steps)
        t = qMax(t, e.version);
    return t;
}

void QiMigrator::setUserVersion(int version) {
    QSqlQuery q = m_conn.query();
    // PRAGMA doesn't take a bound parameter; the value is our own int.
    q.exec(QStringLiteral("PRAGMA user_version = %1").arg(version));
}

int QiMigrator::migrate() {
    m_error.clear();

    QVector<Entry> ordered = m_steps;
    std::sort(ordered.begin(), ordered.end(),
              [](const Entry &a, const Entry &b) { return a.version < b.version; });

    const int from = currentVersion();
    int applied = 0;

    for (const Entry &e : ordered) {
        if (e.version <= from)
            continue;                       // already applied

        if (!m_conn.transaction()) {
            m_error = QStringLiteral("could not begin a transaction");
            return -1;
        }

        const bool ok = e.step ? e.step(m_conn) : true;
        if (!ok) {
            m_conn.rollback();
            QString reason = m_conn.lastError().text().trimmed();
            if (reason.isEmpty())
                reason = QStringLiteral("the migration step returned false");
            m_error = QStringLiteral("migration %1 (%2) failed: %3")
                          .arg(e.version).arg(e.name, reason);
            QiLog::write(QiLog::Sql, QiLog::Error, m_error);
            return -1;
        }

        setUserVersion(e.version);          // transactional in SQLite
        if (!m_conn.commit()) {
            m_error = QStringLiteral("migration %1 (%2): commit failed")
                          .arg(e.version).arg(e.name);
            return -1;
        }

        QiLog::write(QiLog::Sql, QiLog::Info,
                     QStringLiteral("migrated to v%1 (%2)").arg(e.version).arg(e.name));
        applied++;
    }

    return applied;
}

QString QiMigrator::lastError() const {
    return m_error;
}
