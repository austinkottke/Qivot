#ifndef SHAREDCHANGETESTS_H
#define SHAREDCHANGETESTS_H

#include <QObject>
#include <QTemporaryDir>

/// Changes reach every connection to the same database: a QiAsync worker's
/// save updates a live list on the main thread, on the main thread; changes in
/// a transaction arrive when it commits, and never if it rolls back.
class SharedChangeTests : public QObject {
    Q_OBJECT
public:
    explicit SharedChangeTests(QObject *parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void workerSaveReachesLiveList();
    void workerEditAndRemove();
    void hookRunsOnItsContextThread();
    void transactionWaitsForCommit();
    void rollbackSaysNothing();
    void savepointRollbackForgetsItsChanges();
    void otherDatabasesStayQuiet();
    void cleanupTestCase();
private:
    QTemporaryDir m_dir;
};

#endif // SHAREDCHANGETESTS_H
