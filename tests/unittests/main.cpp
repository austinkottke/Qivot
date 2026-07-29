#include <QCoreApplication>
#include "testobjectrunner.h"
#include "coretests.h"
#include "sqlitetests.h"
#include "dialecttests.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TestObjectRunner runner;

    runner.add<CoreTests>();
    runner.add<SqliteTests>();
    runner.add<DialectTests>();

    return runner.exec(a.arguments());
}
