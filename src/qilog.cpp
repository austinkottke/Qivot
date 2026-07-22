#include <cstdio>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QStringList>

#include "qilog.h"

// --- configuration state (off by default) ---------------------------------
static bool           s_enabled    = false;
static QiLog::Level   s_level      = QiLog::Debug;
static int            s_categories = QiLog::AllCategories;
static bool           s_timestamps = true;
static bool           s_colorized  = true;
static QiLog::Handler s_handler    = nullptr;

static QMutex &qiLogMutex() {
    static QMutex mutex;
    return mutex;
}

// --- small helpers ---------------------------------------------------------
static const char *qiLevelLabel(QiLog::Level level) {
    switch (level) {
    case QiLog::Trace:   return "TRACE";
    case QiLog::Debug:   return "DEBUG";
    case QiLog::Info:    return "INFO ";
    case QiLog::Warning: return "WARN ";
    case QiLog::Error:   return "ERROR";
    }
    return "     ";
}

static const char *qiCategoryLabel(int category) {
    // A message carries one category; report the lowest set bit.
    if (category & QiLog::Sql)        return "SQL ";
    if (category & QiLog::Connection) return "CONN";
    if (category & QiLog::Model)      return "MODL";
    if (category & QiLog::Json)       return "JSON";
    return "GEN ";
}

static const char *qiLevelColor(QiLog::Level level) {
    switch (level) {
    case QiLog::Trace:   return "\033[90m"; // bright black / grey
    case QiLog::Debug:   return "\033[36m"; // cyan
    case QiLog::Info:    return "\033[32m"; // green
    case QiLog::Warning: return "\033[33m"; // yellow
    case QiLog::Error:   return "\033[31m"; // red
    }
    return "";
}

static QString qiFormatLine(int category, QiLog::Level level, const QString &message) {
    QString line;
    if (s_timestamps)
        line += QLatin1Char('[')
              + QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
              + QLatin1String("] ");
    line += QLatin1Char('[') + QLatin1String(qiCategoryLabel(category)) + QLatin1String("] ");
    line += QLatin1String(qiLevelLabel(level)) + QLatin1Char(' ') + message;
    return line;
}

// --- configuration API -----------------------------------------------------
void QiLog::setEnabled(bool on)      { QMutexLocker l(&qiLogMutex()); s_enabled = on; }
bool QiLog::isEnabled()              { QMutexLocker l(&qiLogMutex()); return s_enabled; }
void QiLog::setLevel(Level level)    { QMutexLocker l(&qiLogMutex()); s_level = level; }
QiLog::Level QiLog::level()          { QMutexLocker l(&qiLogMutex()); return s_level; }
void QiLog::setCategories(int mask)  { QMutexLocker l(&qiLogMutex()); s_categories = mask; }
int  QiLog::categories()             { QMutexLocker l(&qiLogMutex()); return s_categories; }
void QiLog::setTimestamps(bool on)   { QMutexLocker l(&qiLogMutex()); s_timestamps = on; }
void QiLog::setColorized(bool on)    { QMutexLocker l(&qiLogMutex()); s_colorized = on; }
void QiLog::setHandler(Handler h)    { QMutexLocker l(&qiLogMutex()); s_handler = std::move(h); }

void QiLog::enableAll(Level level) {
    QMutexLocker l(&qiLogMutex());
    s_enabled = true;
    s_level = level;
    s_categories = AllCategories;
}

bool QiLog::wants(int category, Level level) {
    QMutexLocker l(&qiLogMutex());
    return s_enabled && level >= s_level && (category & s_categories);
}

// --- emit ------------------------------------------------------------------
void QiLog::write(int category, Level level, const QString &message) {
    QMutexLocker l(&qiLogMutex());
    if (!s_enabled || level < s_level || !(category & s_categories))
        return;

    const QString line = qiFormatLine(category, level, message);

    if (s_handler) {
        s_handler(level, category, line);
        return;
    }

    if (s_colorized) {
        std::fprintf(stderr, "%s%s\033[0m\n",
                     qiLevelColor(level), line.toLocal8Bit().constData());
    } else {
        std::fprintf(stderr, "%s\n", line.toLocal8Bit().constData());
    }
    std::fflush(stderr);
}

void QiLog::logQuery(const QSqlQuery &query, qint64 elapsedNs) {
    const bool failed = query.lastError().type() != QSqlError::NoError;
    const Level level = failed ? Error : Debug;
    if (!wants(Sql, level))
        return;

    QString msg = query.lastQuery().simplified();

    // Bound parameters (the API shape differs between Qt 5 and Qt 6).
    QStringList params;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const QMap<QString, QVariant> bound = query.boundValues();
    for (auto it = bound.constBegin(); it != bound.constEnd(); ++it)
        params << it.key() + QLatin1Char('=') + it.value().toString();
#else
    const QVariantList bound = query.boundValues();
    for (int i = 0; i < bound.size(); ++i)
        params << QStringLiteral(":%1=%2").arg(i).arg(bound.at(i).toString());
#endif
    if (!params.isEmpty())
        msg += QLatin1String("  | args: [") + params.join(QLatin1String(", ")) + QLatin1Char(']');

    if (failed) {
        msg += QLatin1String("  | error: ") + query.lastError().text().simplified();
    } else {
        const int rows = query.numRowsAffected();
        if (rows >= 0)
            msg += QLatin1String("  | rows: ") + QString::number(rows);
    }

    if (elapsedNs >= 0)
        msg += QLatin1String("  | ") + QString::number(elapsedNs / 1.0e6, 'f', 2) + QLatin1String("ms");

    write(Sql, level, msg);
}
