/** Demonstration of JSON mapping and threaded HTTP loading.

    Two things are shown:

      1. QiJsonMapper   - synchronous JSON <-> model mapping (QtCore only).
      2. QiJsonRequest  - fetch JSON over HTTP on a worker thread, map it to
                          models, save them to the database, and deliver the
                          result on the main thread via a signal.

    So the example runs offline, it starts a tiny local HTTP server that serves
    a canned JSON array. In a real program you would point get() at a REST API.
 */

#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QSqlDatabase>
#include <QHostAddress>
#include <QJsonDocument>
#include <QDebug>

#include <qivot.h>
#include <qijsonrequest.h>

/// A blog post
class Post : public QiModel {
    QI_MODEL
public:
    QiField<int>       userId;
    QiField<QString>   title;
    QiField<QDateTime> published;
};

QI_DECLARE_MODEL(Post,
                 "post",
                 QI_FIELD(userId),
                 QI_FIELD(title),
                 QI_FIELD(published));

/// A minimal HTTP/1.0 server that returns a fixed JSON body (stands in for a REST API).
class MiniHttp : public QObject {
    Q_OBJECT
public:
    explicit MiniHttp(const QByteArray &body) : m_body(body) {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, &MiniHttp::onConnection);
    }
    quint16 port() const { return m_server.serverPort(); }
private slots:
    void onConnection() {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            socket->readAll(); // ignore the request
            QByteArray response = "HTTP/1.0 200 OK\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: " + QByteArray::number(m_body.size()) + "\r\n"
                                  "\r\n" + m_body;
            socket->write(response);
            socket->flush();
            socket->disconnectFromHost();
        });
    }
private:
    QTcpServer m_server;
    QByteArray m_body;
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("jsonhttp.db");
    db.open();

    QiConnection connection;
    if (!connection.open(db))
        return 1;
    connection.addModel<Post>();
    if (!connection.dropTables() || !connection.createTables())
        return 1;

    /* ---- 1. Synchronous mapping with QiJsonMapper ---- */

    QByteArray oneDoc = R"({"userId": 7, "title": "mapped locally"})";
    Post local = QiJsonMapper::map<Post>(QJsonDocument::fromJson(oneDoc).object());
    qDebug() << "QiJsonMapper:" << local.userId->toInt() << local.title->toString();

    /* ---- 2. Threaded HTTP loading with QiJsonRequest ---- */

    QByteArray feed = R"([
        {"userId": 1, "title": "hello world",  "published": "2020-01-01T09:00:00"},
        {"userId": 1, "title": "second post",  "published": "2020-02-01T09:00:00"},
        {"userId": 2, "title": "someone else"}
    ])";
    MiniHttp server(feed);

    QiJsonRequest *request = qiJsonRequest<Post>();
    request->setConnection(connection);                       // save the mapped posts
    request->setRawHeader("Authorization", "Bearer example"); // custom header

    // Pass a context object (here &app, which lives on the main thread) so the
    // slot runs on the main thread. A context-free lambda would run on the
    // worker thread instead.
    QObject::connect(request, &QiJsonRequest::loaded, &app, [&](QiSharedList records) {
        QiList<Post> posts = records;
        qDebug() << "loaded" << posts.size() << "posts over HTTP:";
        for (int i = 0; i < posts.size(); i++)
            qDebug() << "   " << posts.at(i)->title->toString();

        qDebug() << "persisted in DB:" << Post::objects().count();
        app.quit();
    });
    QObject::connect(request, &QiJsonRequest::failed, &app, [&](QString message) {
        qWarning() << "request failed:" << message;
        app.quit();
    });

    request->get(QUrl(QString("http://127.0.0.1:%1/posts").arg(server.port())));

    QTimer::singleShot(10000, &app, [&]() { qWarning() << "timeout"; app.quit(); });
    return app.exec();
}

#include "main.moc"
