#ifndef QiLOG_H
#define QiLOG_H

#include <QString>
#include <QDebug>
#include <functional>

class QSqlQuery;

/// Qivot's debug logger — timestamped, filterable, colorized.
/**
  Logging is **off by default** (zero overhead). Turn it on when you want to see
  what Qivot is doing against the database:

\code
    QiLog::enableAll();                       // everything, at Debug level
    // or fine-grained:
    QiLog::setEnabled(true);
    QiLog::setLevel(QiLog::Debug);
    QiLog::setCategories(QiLog::Sql | QiLog::Connection);   // only these
\endcode

  Each line carries a timestamp, a category tag and a level, e.g.

\verbatim
    [2026-07-21 01:37:12.345] [SQL ] DEBUG  SELECT ALL user.* FROM user WHERE karma > :arg0  | args: [:arg0=50] | rows: 3 | 0.42ms
    [2026-07-21 01:37:12.361] [SQL ] ERROR  INSERT INTO user (...) ...  | error: UNIQUE constraint failed: user.userId
\endcode

  Redirect anywhere (a file, your app's logger, a test buffer) with setHandler().
  You can also log your own lines in the same format via qiLog() / QiLog::debug().
 */
class QiLog {
public:
    /// Severity, low to high. A message is shown only if its level >= the
    /// configured threshold (see setLevel()).
    enum Level { Trace, Debug, Info, Warning, Error };

    /// Message categories (a bitmask — enable any subset with setCategories()).
    enum Category {
        General    = 1 << 0,   ///< your own messages / uncategorised
        Connection = 1 << 1,   ///< open / close
        Sql        = 1 << 2,   ///< every statement executed, with params + timing
        Model      = 1 << 3,   ///< model-level events (validation failures, ...)
        Json       = 1 << 4    ///< JSON mapping
    };
    enum { AllCategories = General | Connection | Sql | Model | Json };

    // ---- configuration ----------------------------------------------------
    static void  setEnabled(bool on);
    static bool  isEnabled();
    static void  setLevel(Level level);        ///< threshold (default Debug)
    static Level level();
    static void  setCategories(int mask);      ///< default: AllCategories
    static int   categories();
    static void  setTimestamps(bool on);       ///< default: true
    static void  setColorized(bool on);        ///< default: true (built-in sink)

    /// One-liner: enable logging for all categories at a level (default Debug).
    static void  enableAll(Level level = Debug);

    /// Custom sink: receives (level, category, formatted-line-without-color).
    /// Pass nullptr to restore the built-in stderr sink.
    using Handler = std::function<void(Level, int, const QString &)>;
    static void  setHandler(Handler handler);

    // ---- cheap predicate (guard expensive message building) ---------------
    static bool  wants(int category, Level level);

    // ---- emit -------------------------------------------------------------
    static void  write(int category, Level level, const QString &message);
    static void  trace  (const QString &m, int c = General) { write(c, Trace,   m); }
    static void  debug  (const QString &m, int c = General) { write(c, Debug,   m); }
    static void  info   (const QString &m, int c = General) { write(c, Info,    m); }
    static void  warning(const QString &m, int c = General) { write(c, Warning, m); }
    static void  error  (const QString &m, int c = General) { write(c, Error,   m); }

    // ---- used by the ORM internals (safe to ignore) -----------------------
    /// Log a just-executed query: SQL text, bound params, row count, error and
    /// (if provided) elapsed time. Level is Error when the query failed.
    static void  logQuery(const QSqlQuery &query, qint64 elapsedNs = -1);
};

/// QDebug-style stream for your own log lines, flushed when it goes out of scope:
/// `qiLog(QiLog::General, QiLog::Info) << "loaded" << n << "rows";`
class QiLogStream {
public:
    QiLogStream(int category, QiLog::Level level)
        : m_cat(category), m_level(level), m_on(QiLog::wants(category, level)) {}
    ~QiLogStream() { if (m_on) QiLog::write(m_cat, m_level, m_buf.trimmed()); }

    template <typename T>
    QiLogStream &operator<<(const T &value) {
        if (m_on) QDebug(&m_buf).noquote() << value;
        return *this;
    }
private:
    int          m_cat;
    QiLog::Level m_level;
    bool         m_on;
    QString      m_buf;
};

/// Convenience factory: `qiLog(QiLog::Sql, QiLog::Debug) << ...;`
inline QiLogStream qiLog(int category = QiLog::General,
                         QiLog::Level level = QiLog::Debug) {
    return QiLogStream(category, level);
}

#endif // QiLOG_H
