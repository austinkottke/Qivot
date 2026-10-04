#ifndef QiRECORDER_H
#define QiRECORDER_H

#include <QDateTime>
#include <QString>
#include <QVariantList>
#include <QVector>

class QSqlQuery;

/// Records the queries an app runs through Qivot, to replay them later.
/**
  Every statement Qivot executes (the ones QiLog sees) is appended to a file
  as one JSON object per line: the SQL, its bound values, how long it took
  and whether it failed. Nothing else is needed in the app: set
  `QIVOT_RECORD=path/to/app.qrec` in its environment, or call start().

\code
    QiRecorder::start("app.qrec");     // or: QIVOT_RECORD=app.qrec ./myapp
    ...                                // use the app
    QiRecorder::stop();

    for (const QiRecorder::Query &q : QiRecorder::read("app.qrec"))
        qDebug() << q.runs << q.sql;
\endcode

  Each distinct SQL text is written at most `samples` times (default 20, or
  QIVOT_RECORD_SAMPLES), each with its own bound values; later runs are only
  counted, and the counts are written when recording stops (or the app quits).
  So a loop that runs one query a million times costs a handful of lines.

  Qt Sql's own drivers can't be watched, so a QSqlQuery an app runs itself
  (`connection.query().exec(...)`) isn't recorded; everything through Qivot is.

  The recording is what `qivot-cli replay` reads to test migrations against
  the queries an app really runs.
 */
class QiRecorder {
public:
    /// One recorded run of a statement.
    struct Run {
        QVariantList values;     ///< bound values, in order
        double       ms = -1;    ///< how long it took (-1: unknown)
        bool         failed = false;
        QString      error;
    };

    /// A distinct statement, with the runs recorded for it.
    struct Query {
        QString      sql;
        QString      driver;     ///< the Qt driver it ran on ("QSQLITE", "QPSQL", ...)
        qint64       runs = 0;   ///< how many times it ran (recorded or only counted)
        QVector<Run> samples;
    };

    /// Start appending to `path` (created if missing). False if it can't be opened.
    static bool start(const QString &path, int samples = 20);
    /// Stop, writing the counts of runs beyond the samples.
    static void stop();
    static bool isRecording();
    /// The file being recorded to ("" if none).
    static QString path();

    /// Called by QiLog for every statement Qivot runs.
    static void record(const QSqlQuery &query, qint64 elapsedNs);

    /// Read a recording: one entry per distinct (driver, SQL), in first-seen order.
    static QVector<Query> read(const QString &path, QString *error = nullptr);

    /// A bound value as JSON and back (bytes, dates and times keep their type).
    static QVariant encode(const QVariant &value);
    static QVariant decode(const QVariant &json);
};

#endif // QiRECORDER_H
