#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>

#include "qijsonrequest.h"
#include "qijsonmapper.h"
#include "qimodel.h"
#include "qisql.h"

QAtomicInt QiJsonRequest::s_counter(0);

QiJsonRequest::QiJsonRequest(QiModelMetaInfo *metaInfo, QObject *parent)
    : QThread(parent) , m_metaInfo(metaInfo) , m_save(false) , m_writeMode(Replace) {
    // Allow QiSharedList to travel across the queued signal to the caller thread.
    qRegisterMetaType<QiSharedList>("QiSharedList");

    // One-shot: clean up after the thread has finished and its signals delivered.
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

QiJsonRequest::~QiJsonRequest() {
    // If still running (unusual), make sure the thread has stopped.
    if (isRunning()) {
        wait();
    }
}

void QiJsonRequest::setRawHeader(const QByteArray &name, const QByteArray &value) {
    m_headers.append(qMakePair(name, value));
}

void QiJsonRequest::setConnection(QiConnection connection) {
    // Capture the database parameters now, on the caller's thread, so the worker
    // never has to touch the caller's QSqlDatabase object.
    QSqlDatabase db = connection.sql().database();
    m_dbDriver = db.driverName();
    m_dbName = db.databaseName();
    m_save = true;
}

void QiJsonRequest::setSaveEnabled(bool enabled) {
    m_save = enabled;
}

void QiJsonRequest::setWriteMode(WriteMode mode) {
    m_writeMode = mode;
}

void QiJsonRequest::setUpsertKeys(const QStringList &keys) {
    m_upsertKeys = keys;
    if (!keys.isEmpty())
        m_writeMode = Upsert;
}

void QiJsonRequest::get(const QUrl &url) {
    m_url = url;
    start();
}

void QiJsonRequest::run() {
    // --- 1. HTTP GET (blocking on this worker thread via a local event loop) ---
    QNetworkAccessManager nam;
    QNetworkRequest request(m_url);
    for (int i = 0 ; i < m_headers.size() ; i++) {
        request.setRawHeader(m_headers.at(i).first, m_headers.at(i).second);
    }

    QNetworkReply *reply = nam.get(request);   // owned by nam; freed with it

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        emit failed(QString("HTTP error: %1").arg(reply->errorString()));
        return;
    }

    QByteArray body = reply->readAll();

    // --- 2. Parse JSON ---
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit failed(QString("JSON parse error: %1").arg(parseError.errorString()));
        return;
    }

    // --- 3. Map to models ---
    QiSharedList records;
    if (doc.isArray()) {
        records = QiJsonMapper::fromJson(m_metaInfo, doc.array());
    } else if (doc.isObject()) {
        QiAbstractModel *model = QiJsonMapper::fromJson(m_metaInfo, doc.object());
        if (model)
            records.append(model);
    } else {
        emit failed(QStringLiteral("JSON root is neither an array nor an object"));
        return;
    }

    // --- 4. Optionally persist on a per-thread connection ---
    if (m_save && !m_dbName.isEmpty()) {
        const QString connName = QStringLiteral("qivot_json_%1")
                .arg(s_counter.fetchAndAddOrdered(1));
        bool ok = true;
        QString saveError;

        {
            QSqlDatabase wdb = QSqlDatabase::addDatabase(m_dbDriver, connName);
            wdb.setDatabaseName(m_dbName);
            // Wait for the lock instead of failing immediately when the primary
            // connection is touching the same SQLite file concurrently.
            wdb.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=10000"));

            if (!wdb.open()) {
                ok = false;
                saveError = wdb.lastError().text();
            } else {
                QiConnection wconn;
                // asDefault = false : never touch the caller thread's default connection.
                if (wconn.open(wdb, false)) {
                    wconn.addModel(m_metaInfo);
                    const bool upsert = (m_writeMode == Upsert) && !m_upsertKeys.isEmpty();

                    // One transaction for the whole batch: atomic, and far faster
                    // than an implicit commit (fsync) per record.
                    bool tx = wconn.transaction();
                    for (int i = 0 ; i < records.size() ; i++) {
                        QiModel *m = static_cast<QiModel *>(records.at(i));
                        m->setConnection(wconn);
                        bool wrote = upsert ? m->upsert(m_upsertKeys) : m->save();
                        if (!wrote) {
                            ok = false;
                            saveError = QStringLiteral("write failed for record %1").arg(i);
                            break;
                        }
                    }
                    if (tx) {
                        if (ok) {
                            if (!wconn.commit())
                                ok = false;
                        } else {
                            wconn.rollback();
                        }
                    }
                    wconn.close();
                } else {
                    ok = false;
                    saveError = QStringLiteral("could not open a QiConnection on the worker thread");
                }
            }
        }
        // The QSqlDatabase must be removed after all its objects are out of scope.
        QSqlDatabase::removeDatabase(connName);

        if (!ok) {
            emit failed(QString("DB save error: %1").arg(saveError));
            return;
        }
    }

    emit loaded(records);
}
