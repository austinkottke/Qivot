#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "store.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Create and register store
    Store store;
    engine.rootContext()->setContextProperty("store", &store);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
