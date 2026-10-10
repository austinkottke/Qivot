#ifndef LIVELISTTESTS_H
#define LIVELISTTESTS_H

#include <QObject>

/// QiLiveListModel on 10,000 rows: one change, one row signal, one small
/// query; and QiListModel's live mode comparing by id instead of resetting.
class LiveListTests : public QObject {
    Q_OBJECT
public:
    explicit LiveListTests(QObject *parent = nullptr) : QObject(parent) {}
private slots:
    void initTestCase();
    void listModelDiffs();
    void tenThousand();
    void editInPlace();
    void moveInsertRemove();
    void leavesTheFilter();
    void severalAtOnce();
    void bulkWrite();
    void cacheIsBounded();

    /// save() never deletes a row (no REPLACE): the integration suite, on SQLite
    void saveNeverDeletes();
};

#endif // LIVELISTTESTS_H
