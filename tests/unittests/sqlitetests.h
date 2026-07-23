#ifndef SQLITETESTS_H
#define SQLITETESTS_H

#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>

#include <QSqlError>
#include <qiconnection.h>
#include <qisqlitestatement.h>
#include <qiquery.h>
#include <qisql.h>
#include <qilistwriter.h>

#include "model1.h"
#include "model2.h"
#include "model3.h"
#include "model4.h"
#include "model5.h"
#include "config.h"
#include "misc.h"

class SqliteTests : public QObject
{
    Q_OBJECT

public:
    SqliteTests(QObject* parent = 0);

    /// Verify the create table stmt
    void verifyCreateTable();

    /// Test the basic operation of QiForeignKey
    void foreignKey();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void insertInto();

    /// Test QiModel::save()
    void qiModelSave();

    /// Test QiQuery::deletFrom without filter
    /**
      It will clear all the record made by previous operation.
     */
    void deleteAll();

    /// Insert pre-defined records for each model. They may needed for following tests
    /**
      @todo Should be part of initTestCase
     */
    void prepareInitRecords();

    void select();

    void queryAll();
    void querySelect();

    /// test different combination of where
    void querySelectWhere();

    /// Test can it load model through foreign key
    void foreignKeyLoad();

    /// Test Model4 access
    void model4();

    /// Test date time access
    void datetime();

    /// Verify the save and load for specific type
    void checkTypeSaveAndLoad();

    void queryOrderBy();

    /// Test JOIN queries (INNER / LEFT , cross-table filter , count , aggregate)
    void join();

    /// Test JSON <-> model mapping (QiJsonMapper)
    void jsonMapper();

    /// Test QiModel::upsert (INSERT ... ON CONFLICT DO UPDATE)
    void upsert();

    /// Test full-text search (FTS5) index + QiQuery::search
    void fts();

    /// Test transactions (QiTransaction commit / rollback)
    void transaction();

    /// Test foreign-key enforcement (PRAGMA foreign_keys)
    void fkEnforcement();

    /// Test schema migration (createTables adds missing columns)
    void migration();

    /// Test batched QiList::save (prepared statement reuse + transaction)
    void batchSave();

    /// Test the error API (QiModel::lastError / QiConnection::lastError)
    void errorHandling();

    /// Test offset/limit ordering, GROUP BY / HAVING, WAL, and UNIQUE indexes
    void queryExtras();

    /// Test type-safe field references (qiField / pointer-to-member)
    void fieldRef();

    /// Test type-safe column descriptors (Model::col().field)
    void columns();

    /// Test the debug logger (QiLog): capture, level + category filtering
    void logging();

    /// Test a model with a string primary key and no auto-increment id column
    void noAutoId();

    /// Test bulk UPDATE ... SET via QiQuery::update()
    void bulkUpdate();

    /// Test composite (multi-column) primary keys
    void compositeKey();

    /// Test foreign-key referential actions + references to non-id / string keys
    void foreignKeyActions();

    /// Test CHECK column constraints (QiCheck)
    void checkConstraint();

    /// Test WITHOUT ROWID tables
    void withoutRowid();

    /// Test reverse (has-many) relations via qiHasMany()
    void hasMany();

    /// Test rename / drop column migrations
    void alterColumn();

    /// Test enum fields (stored as integers)
    void enumField();

    /// Test per-column SQL type overrides (QI_FIELD_AS), incl. a typeless column
    void fieldTypeOverride();

    /// Test complex/nested JSON serialization into structured model fields
    void jsonComplex();

    /// Test reactive change notifications (QiListModel live auto-refresh)
    void reactive();

    /// Test lazy paging model for infinite scroll (QiLazyListModel)
    void lazyScroll();

    // --- features added later -------------------------------------------
    /// Keyset (cursor) pagination — QiKeyset forward paging + resume
    void keysetPaging();
    /// Versioned schema migrations — QiMigrator ordering / idempotency / rollback
    void migrator();
    /// Declarative many-to-many — QI_MANY_TO_MANY / QiRelationSet, both directions
    void relationsManyToMany();
    /// One-to-many accessor + batched prefetch (N+1 avoidance)
    void relationsHasManyPrefetch();
    /// Custom field type converter — QI_DECLARE_CONVERTER round-trip
    void converterField();
    /// Lifecycle hooks — afterSave / beforeRemove
    void lifecycleHooks();
    /// Auto timestamps — createdAt / updatedAt
    void autoTimestamps();
    /// Soft delete — softRemove + qiAlive / qiTrashed
    void softDelete();
    /// Nested transactions via SAVEPOINT (QiTransaction nesting)
    void nestedTransaction();
    /// Raw typed query — qiRawQuery maps subquery/window SQL to models
    void rawTypedQuery();
    /// Per-thread connection pool (QiConnectionPool)
    void connectionPool();
    /// Async query + cancellation token (QiAsync)
    void asyncQuery();

private:
    QiConnection connect;
    QSqlDatabase db;
};

#endif // SQLITETESTS_H
