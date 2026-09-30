#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include "testobjectrunner.h"
#include "coretests.h"
#include "sqlitetests.h"
#include "dialecttests.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
// A phone app is launched through Qt's platform plugin, which only starts
// under a QGuiApplication (unittests.pro adds gui for these platforms).
#include <QGuiApplication>
using TestApplication = QGuiApplication;
#else
using TestApplication = QCoreApplication;
#endif

int main(int argc, char *argv[])
{
    TestApplication a(argc, argv);

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // A phone app starts in "/", which it can't write to, and the SQLite tests
    // open "tests.db" relative to the working directory. Move into the app's
    // own data directory first.
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QDir::setCurrent(dataDir);
#endif

    TestObjectRunner runner;

    runner.add<CoreTests>();
    runner.add<SqliteTests>();
    runner.add<DialectTests>();

    return runner.exec(a.arguments());
}
