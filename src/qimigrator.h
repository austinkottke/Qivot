#ifndef QiMIGRATOR_H
#define QiMIGRATOR_H

#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QVector>
#include <QString>
#include <functional>
#include <qiconnection.h>

/// Versioned schema migrations, on every database Qivot supports.
/**
  Register migrations keyed by an increasing version number — as code, as SQL
  text, or as a folder of `.sql` files — then call migrate(): every migration
  not yet applied runs in version order, each inside its own transaction, and is
  recorded in a `qivot_migrations` table (version, name, checksum, applied_at,
  duration_ms). Re-running is a no-op, so migrate() is safe to call on every
  startup.

\code
    QiMigrator m(conn);
    m.add(1, "create tables", [](QiConnection &c){
        return c.query().exec("CREATE TABLE note (id INTEGER PRIMARY KEY, title TEXT)");
    });
    m.addSql(2, "add body", "ALTER TABLE note ADD COLUMN body TEXT",
                            "ALTER TABLE note DROP COLUMN body");      // optional down
    m.addDirectory(":/migrations");     // 0003_tags.sql, 0004_index.up.sql + 0004_index.down.sql …

    int applied = m.migrate();          // -> number of migrations run this time
    if (applied < 0) qWarning() << m.lastError();
\endcode

  **Files.** addDirectory() reads `NNNN_name.sql` (or `NNNN_name.up.sql`) and an
  optional `NNNN_name.down.sql`; the number is the version, the rest the name
  (underscores become spaces). A file holds any number of statements separated
  by `;` — quotes, comments, PostgreSQL `$$` bodies and trigger `BEGIN … END`
  blocks are respected, and SQL Server's `GO` lines also separate. A file that
  must reach the server in one piece can say so with a `-- qivot:no-split` line.

  **Checksums.** SQL migrations are recorded with a SHA-256 of their text. If a
  migration that has already run is edited afterwards, migrate() refuses to run
  (see changed()) until the edit is undone or accepted with acceptChecksums():
  the database no longer matches the file, and carrying on would hide that.
  Code migrations have no text, so they are never flagged.

  **Transactions.** Each migration runs in a transaction of its own, so `BEGIN` /
  `COMMIT` statements in a file are left out. SQLite, PostgreSQL, SQL Server and DuckDB run DDL inside the
  transaction, so a failed migration leaves nothing behind. MySQL/MariaDB commit
  each DDL statement on the spot: a migration that fails halfway there keeps the
  statements before the failure and is not recorded, so keep MySQL migrations to
  one DDL statement each where you can.

  **SQLite foreign keys.** SQLite changes a table by rebuilding it, and dropping
  the old copy of a parent table would delete (or block on) its children. So, as
  SQLite's documentation prescribes, foreign keys are switched off around each
  migration and `PRAGMA foreign_key_check` must come back clean before it
  commits; a migration that leaves dangling references fails and is rolled back.

  **Concurrency.** On PostgreSQL (`pg_advisory_lock`), MySQL (`GET_LOCK`) and SQL
  Server (`sp_getapplock`) migrate() and rollback() hold a lock for the whole run,
  so two app instances starting at once don't both migrate. SQLite serialises
  writers itself.

  **Upgrading from the SQLite-only migrator.** Earlier versions tracked the version
  in SQLite's `PRAGMA user_version` alone. The first run on such a database records
  the registered migrations up to that version as applied, without running them
  again; SQLite databases keep `user_version` in step from then on. (Only the
  default table does this: a table named with setTable() is a history of its own.)
 */
class QiMigrator {
public:
    /// A single migration step: change the schema, return true on success.
    using Step = std::function<bool(QiConnection &)>;

    /// One migration, as status() reports it.
    struct Migration {
        int       version = 0;
        QString   name;
        QString   checksum;           ///< SHA-256 of the up SQL (empty for code migrations)
        QString   sql;                ///< the up SQL (empty for code migrations)
        QString   downSql;            ///< the down SQL, if any
        bool      known = false;      ///< registered with this migrator
        bool      applied = false;    ///< recorded in the database
        bool      reversible = false; ///< has a down step
        bool      changed = false;    ///< applied, but its SQL has changed since
        QDateTime appliedAt;          ///< when it was applied (UTC)
        int       durationMs = -1;    ///< how long it took (-1 if unknown)
        QString   appliedChecksum;    ///< the checksum recorded when it ran
    };

    explicit QiMigrator(QiConnection connection = QiConnection::defaultConnection());

    // --- Registering ---------------------------------------------------------

    /// Register a code migration to `version` (must be > 0). Registration order
    /// is irrelevant; migrations always run in ascending version order.
    void add(int version, const QString &name, Step up, Step down = Step());

    /// Register a migration given as SQL text (any number of statements).
    void addSql(int version, const QString &name, const QString &upSql, const QString &downSql = QString());

    /// Register every `NNNN_name.sql` / `.up.sql` (+ `.down.sql`) file in `dir`.
    /**
      `dir` may be a Qt resource path (":/migrations").
      @return the number of migrations added, or -1 if the folder can't be read,
              a file name doesn't start with a version, or two files share one.
     */
    int addDirectory(const QString &dir);

    // --- Status --------------------------------------------------------------

    /// The highest applied version (0 for a fresh database).
    int currentVersion() const;

    /// The highest registered version.
    int targetVersion() const;

    /// Every migration, registered or recorded, in version order.
    /** Rows recorded in the database but no longer registered have `known == false`. */
    QVector<Migration> status() const;

    /// The registered migrations that haven't run yet, in the order migrate() runs them.
    QVector<Migration> pending() const;

    /// Applied migrations whose SQL no longer matches what ran.
    QVector<Migration> changed() const;

    // --- Running -------------------------------------------------------------

    /// Run every pending migration in order.
    /**
      @return the number of migrations applied this call (0 if already up to
              date), or -1 on failure (see lastError()).
     */
    int migrate();

    /// Run the pending migrations up to and including `version`.
    int migrateTo(int version);

    /// Undo applied migrations newer than `version`, newest first, using their down steps.
    /**
      @return the number of migrations rolled back, or -1 on failure. If any of
              them has no down step, nothing is undone.
     */
    int rollback(int version);

    /// Record the current checksums for applied migrations that have changed.
    /** Use after deliberately editing a migration that has already run. */
    bool acceptChecksums();

    /// Human-readable reason the last call failed (empty on success).
    QString lastError() const;

    // --- Options -------------------------------------------------------------

    /// The table migrations are recorded in (default "qivot_migrations").
    /** Several migrators can share a database, each with its own table. */
    void setTable(const QString &table);
    QString table() const;

    /// Split SQL into statements the way SQL migrations are run.
    /** `driver` is the Qt driver name ("QPSQL", "QODBC", …): it decides
        whether `$$` bodies, `[identifiers]` and `GO` lines apply. */
    static QStringList splitStatements(const QString &sql, const QString &driver = QString());

    /// The checksum recorded for a migration's SQL.
    static QString checksum(const QString &sql);

private:
    struct Entry {
        int     version;
        QString name;
        Step    up;
        Step    down;
        QString sql;
        QString downSql;
        QString checksum;
    };
    struct Row { QString name; QString checksum; QDateTime appliedAt; int durationMs; };

    QString driver() const;
    bool isSqlite() const;
    bool usesUserVersion() const;
    bool tableExists() const;
    QString literal(const QString &value) const;
    bool ensureTable();
    bool begin();
    bool foreignKeysHold();
    bool end(bool commit);
    QMap<int, Row> appliedRows(bool *ok = nullptr) const;
    QVector<Entry> ordered() const;
    bool runStep(const Entry &e, bool up);
    bool runSql(const QString &sql);
    bool record(const Entry &e, int durationMs);
    bool unrecord(int version);
    void syncUserVersion();
    bool lock();
    void unlock();
    int  failed(const QString &message);
    int  run(int upTo);

    mutable QiConnection m_conn;   // query() is non-const; status() is logically const
    QVector<Entry> m_steps;
    QString        m_table = QStringLiteral("qivot_migrations");
    QString        m_error;
    bool           m_locked = false;
    bool           m_inTransaction = false;
    bool           m_restoreForeignKeys = false;
};

#endif // QiMIGRATOR_H
