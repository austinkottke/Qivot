#ifndef QiMIGRATOR_H
#define QiMIGRATOR_H

#include <QVector>
#include <QString>
#include <functional>
#include <qiconnection.h>

/// Versioned schema migrations, tracked by SQLite's `PRAGMA user_version`.
/**
  Register migrations keyed by an increasing version number, then call
  migrate(): every migration whose version is greater than the database's
  current `user_version` runs in order, each inside its own transaction, and
  `user_version` is advanced to match. Re-running is a no-op — already-applied
  migrations are skipped — so migrate() is safe to call on every startup.

  Each step is a callable `bool(QiConnection&)`; return false (or let a Qivot
  call fail) to abort — the step's transaction is rolled back and the version is
  left unchanged.

\code
    QiMigrator m(conn);
    m.add(1, "create tables", [](QiConnection &c){
        return c.query().exec("CREATE TABLE note (id INTEGER PRIMARY KEY, title TEXT)");
    });
    m.add(2, "add body", [](QiConnection &c){
        return c.query().exec("ALTER TABLE note ADD COLUMN body TEXT");
    });

    int applied = m.migrate();          // -> number of migrations run this time
    if (applied < 0) qWarning() << m.lastError();
\endcode
 */
class QiMigrator {
public:
    /// A single migration: mutate the schema, return true on success.
    using Step = std::function<bool(QiConnection &)>;

    explicit QiMigrator(QiConnection connection = QiConnection::defaultConnection());

    /// Register a migration to `version` (must be > 0). add() order is irrelevant;
    /// migrations always run in ascending version order.
    void add(int version, const QString &name, Step step);

    /// The database's current schema version (`PRAGMA user_version`; 0 if fresh).
    int currentVersion() const;

    /// The highest registered migration version.
    int targetVersion() const;

    /// Run every pending migration in order.
    /**
      @return the number of migrations applied this call (0 if already
              up to date), or -1 on failure (see lastError()).
     */
    int migrate();

    /// Human-readable reason migrate() returned -1 (empty on success).
    QString lastError() const;

private:
    struct Entry { int version; QString name; Step step; };
    void setUserVersion(int version);

    mutable QiConnection m_conn;   // query() is non-const; currentVersion() is logically const
    QVector<Entry> m_steps;
    QString        m_error;
};

#endif // QiMIGRATOR_H
