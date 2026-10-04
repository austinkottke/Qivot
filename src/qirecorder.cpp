#include <atomic>
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include "qirecorder.h"

namespace {

struct RecorderState {
    QMutex                 mutex;
    QFile                 *file = nullptr;
    int                    samples = 20;
    bool                   envChecked = false;
    QHash<QString, qint64> seen;        // key -> runs so far
    QHash<QString, QPair<QString, QString>> keys;   // key -> (driver, sql), for the counts
};

RecorderState &recorderState() {
    static RecorderState s;
    return s;
}

std::atomic<bool> recorderActive{false};
std::atomic<bool> recorderEnvChecked{false};

QString recorderDriverName(const QSqlQuery &q) {
    const QSqlDriver *d = q.driver();
    if (!d)
        return QString();
    switch (d->dbmsType()) {
    case QSqlDriver::SQLite:      return QStringLiteral("QSQLITE");
    case QSqlDriver::PostgreSQL:  return QStringLiteral("QPSQL");
    case QSqlDriver::MySqlServer: return QStringLiteral("QMYSQL");
    case QSqlDriver::MSSqlServer: return QStringLiteral("QODBC");
    case QSqlDriver::Oracle:      return QStringLiteral("QOCI");
    default:                      return QString();
    }
}

void recorderWrite(RecorderState &s, const QJsonObject &o) {
    if (!s.file)
        return;
    s.file->write(QJsonDocument(o).toJson(QJsonDocument::Compact));
    s.file->write("\n");
    s.file->flush();
}

// Write the runs beyond the samples, then close. Called with the mutex held.
void recorderFinish(RecorderState &s) {
    if (!s.file)
        return;
    for (auto it = s.seen.constBegin(); it != s.seen.constEnd(); ++it) {
        const qint64 more = it.value() - s.samples;
        if (more > 0) {
            const QPair<QString, QString> k = s.keys.value(it.key());
            recorderWrite(s, QJsonObject{ { "driver", k.first }, { "sql", k.second }, { "more", double(more) } });
        }
    }
    s.file->close();
    delete s.file;
    s.file = nullptr;
    s.seen.clear();
    s.keys.clear();
    recorderActive = false;
}

void recorderAtExit() {
    QiRecorder::stop();
}

bool recorderStartLocked(RecorderState &s, const QString &path, int samples) {
    if (s.file)
        recorderFinish(s);
    auto *f = new QFile(path);
    if (!f->open(QIODevice::WriteOnly | QIODevice::Append)) {
        delete f;
        return false;
    }
    s.file = f;
    s.samples = qMax(1, samples);
    recorderActive = true;
    if (QCoreApplication::instance())
        qAddPostRoutine(recorderAtExit);    // write the counts when the app quits
    return true;
}

// QIVOT_RECORD=path starts recording the first time a query runs.
void recorderCheckEnvironment() {
    if (recorderEnvChecked.load())
        return;
    RecorderState &s = recorderState();
    QMutexLocker lock(&s.mutex);
    if (s.envChecked)
        return;
    s.envChecked = true;
    recorderEnvChecked = true;
    const QString path = qEnvironmentVariable("QIVOT_RECORD");
    if (!path.isEmpty() && !s.file) {
        bool ok = false;
        const int samples = qEnvironmentVariableIntValue("QIVOT_RECORD_SAMPLES", &ok);
        recorderStartLocked(s, path, ok ? samples : 20);
    }
}

} // namespace

bool QiRecorder::start(const QString &path, int samples) {
    RecorderState &s = recorderState();
    QMutexLocker lock(&s.mutex);
    s.envChecked = true;            // an explicit start wins over QIVOT_RECORD
    recorderEnvChecked = true;
    return recorderStartLocked(s, path, samples);
}

void QiRecorder::stop() {
    RecorderState &s = recorderState();
    QMutexLocker lock(&s.mutex);
    recorderFinish(s);
}

bool QiRecorder::isRecording() {
    recorderCheckEnvironment();
    return recorderActive.load();
}

QString QiRecorder::path() {
    RecorderState &s = recorderState();
    QMutexLocker lock(&s.mutex);
    return s.file ? s.file->fileName() : QString();
}

void QiRecorder::record(const QSqlQuery &query, qint64 elapsedNs) {
    recorderCheckEnvironment();
    if (!recorderActive.load())
        return;
    const QString sql = query.lastQuery().trimmed();
    if (sql.isEmpty())
        return;
    const QString driver = recorderDriverName(query);

    RecorderState &s = recorderState();
    QMutexLocker lock(&s.mutex);
    if (!s.file)
        return;
    const QString key = driver + QChar(0x1f) + sql;
    const qint64 runs = ++s.seen[key];
    if (runs == 1)
        s.keys.insert(key, qMakePair(driver, sql));
    if (runs > s.samples)
        return;                     // counted; written as "more" at the end

    QJsonArray values;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const int n = query.boundValues().size();
#else
    const int n = int(query.boundValues().size());
#endif
    for (int i = 0; i < n; ++i)
        values << QJsonValue::fromVariant(encode(query.boundValue(i)));

    QJsonObject o{ { "driver", driver }, { "sql", sql }, { "values", values },
                   { "at", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) } };
    if (elapsedNs >= 0)
        o.insert("ms", elapsedNs / 1.0e6);
    if (query.lastError().type() != QSqlError::NoError) {
        o.insert("failed", true);
        o.insert("error", query.lastError().text().simplified());
    }
    recorderWrite(s, o);
}

QVector<QiRecorder::Query> QiRecorder::read(const QString &path, QString *error) {
    QVector<Query> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("%1: %2").arg(path, f.errorString());
        return out;
    }
    QHash<QString, int> index;
    int line = 0;
    while (!f.atEnd()) {
        const QByteArray text = f.readLine().trimmed();
        ++line;
        if (text.isEmpty())
            continue;
        QJsonParseError pe;
        const QJsonObject o = QJsonDocument::fromJson(text, &pe).object();
        if (pe.error != QJsonParseError::NoError || !o.contains("sql")) {
            if (error) *error = QStringLiteral("%1:%2: not a recorded query").arg(path).arg(line);
            return {};
        }
        const QString driver = o.value("driver").toString();
        const QString sql = o.value("sql").toString();
        const QString key = driver + QChar(0x1f) + sql;
        int at = index.value(key, -1);
        if (at < 0) {
            at = out.size();
            index.insert(key, at);
            Query q;
            q.sql = sql;
            q.driver = driver;
            out << q;
        }
        Query &q = out[at];
        if (o.contains("more")) {
            q.runs += qint64(o.value("more").toDouble());
            continue;
        }
        Run r;
        for (const QJsonValue &v : o.value("values").toArray())
            r.values << decode(v.toVariant());
        r.ms = o.value("ms").toDouble(-1);
        r.failed = o.value("failed").toBool();
        r.error = o.value("error").toString();
        q.samples << r;
        q.runs += 1;
    }
    return out;
}

QVariant QiRecorder::encode(const QVariant &value) {
    if (value.isNull())
        return QVariant();
    switch (value.userType()) {
    case QMetaType::Bool:
    case QMetaType::Int: case QMetaType::UInt:
    case QMetaType::LongLong: case QMetaType::ULongLong:
    case QMetaType::Double: case QMetaType::Float:
    case QMetaType::QString:
        return value;
    case QMetaType::QByteArray:
        return QVariantMap{ { QStringLiteral("bytes"), QString::fromLatin1(value.toByteArray().toBase64()) } };
    case QMetaType::QDate:
        return QVariantMap{ { QStringLiteral("date"), value.toDate().toString(Qt::ISODate) } };
    case QMetaType::QTime:
        return QVariantMap{ { QStringLiteral("time"), value.toTime().toString(Qt::ISODateWithMs) } };
    case QMetaType::QDateTime:
        return QVariantMap{ { QStringLiteral("datetime"), value.toDateTime().toString(Qt::ISODateWithMs) } };
    default:
        return value.toString();
    }
}

QVariant QiRecorder::decode(const QVariant &json) {
    if (json.isNull())
        return QVariant();
    if (json.userType() == QMetaType::QVariantMap) {
        const QVariantMap m = json.toMap();
        if (m.contains(QStringLiteral("bytes")))
            return QByteArray::fromBase64(m.value(QStringLiteral("bytes")).toString().toLatin1());
        if (m.contains(QStringLiteral("date")))
            return QDate::fromString(m.value(QStringLiteral("date")).toString(), Qt::ISODate);
        if (m.contains(QStringLiteral("time")))
            return QTime::fromString(m.value(QStringLiteral("time")).toString(), Qt::ISODateWithMs);
        if (m.contains(QStringLiteral("datetime")))
            return QDateTime::fromString(m.value(QStringLiteral("datetime")).toString(), Qt::ISODateWithMs);
        return QVariant();
    }
    // JSON numbers are doubles: whole ones go back as integers, as they were bound.
    if (json.userType() == QMetaType::Double) {
        const double d = json.toDouble();
        if (d == double(qint64(d)) && qAbs(d) < 9.0e15)
            return qint64(d);
    }
    return json;
}
