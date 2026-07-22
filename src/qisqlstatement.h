#ifndef QiSQLSTATEMENT_H
#define QiSQLSTATEMENT_H

#include <QString>

#include <qimodelmetainfo.h>
#include <qisharedquery.h>
#include <qiqueryrules.h>
#include <qiindex.h>
#include <qiftsindex.h>

class QiSharedQuery;
class QiQueryRules;

/// Sql Statement generator abstract interface

class QiSqlStatement
{
public:
    /// Default constructor
    QiSqlStatement();

    /// Get the supported driver name
    virtual QString driverName() = 0;

    /// "CREATE TABLE IF NOT EXISTS" statement
    template <typename T>
    QString createTableIfNotExists() {
        QiModelMetaInfo *info = qiMetaInfo<T>();
        return _createTableIfNotExists(info);
    }

    /// "CREATE TABLE IF NOT EXISTS" statement
    virtual QString createTableIfNotExists(QiModelMetaInfo *info);

    /// "DROP TABLE" statement
    template <typename T>
    QString dropTable() {
        QiModelMetaInfo *info = qiMetaInfo<T>();
        return dropTable(info);
    }

    /// Drop table statement
    virtual QString dropTable(QiModelMetaInfo *info);

    /// "ALTER TABLE ... ADD COLUMN" statement for a single field (migrations)
    virtual QString addColumn(QiModelMetaInfo *info, const QiModelMetaInfoField *field);

    /// Rename a column (SQLite 3.25+): ALTER TABLE ... RENAME COLUMN a TO b
    virtual QString renameColumn(QiModelMetaInfo *info, const QString &from, const QString &to);

    /// Drop a column (SQLite 3.35+): ALTER TABLE ... DROP COLUMN name
    virtual QString dropColumn(QiModelMetaInfo *info, const QString &name);

    /// Create index statement
    virtual QString createIndexIfNotExists(const QiBaseIndex& index);

    /// Drop the index
    virtual QString dropIndexIfExists(QString name);

    /// Statements that create an FTS5 index (virtual table + sync triggers + rebuild)
    virtual QStringList createFtsIndex(const QiBaseFtsIndex &index);

    /// Statements that drop an FTS5 index (its triggers + virtual table)
    virtual QStringList dropFtsIndex(QString name);

    /// Insert into statement
    /**
      @param with_id TRUE if the "id" field should be included.
     */
    virtual QString insertInto(QiModelMetaInfo *info,QStringList fields);

    /// Replace into statement
    /**
      @param with_id TRUE if the "id" field should be included.
     */
    virtual QString replaceInto(QiModelMetaInfo *info,QStringList fields);

    /// Upsert statement : "INSERT INTO ... ON CONFLICT(keys) DO UPDATE SET ..."
    /**
      A non-destructive upsert: insert the row, or if it collides on the given
      conflict column(s), update the other columns in place (keeping the row's
      primary key). Requires SQLite 3.24 or newer.

      @param fields The columns to insert.
      @param conflictColumns The unique/primary column(s) that identify an
             existing row (the ON CONFLICT target).
     */
    virtual QString upsertInto(QiModelMetaInfo *info,QStringList fields,QStringList conflictColumns);

    /// Select statement
    virtual QString select(QiSharedQuery query);

    /// Delete from statement
    virtual QString deleteFrom(QiSharedQuery query);

    /// Update statement: UPDATE <table> SET <field> = :set_<field> ... WHERE ...
    /** The value for each field is bound under the placeholder ":set_<field>". */
    virtual QString update(QiSharedQuery query, const QStringList &fields);

    /// Returns a string representation of the QVariant for SQL statement
    virtual QString formatValue(QVariant value,bool trimStrings = false);

protected:
    /// The real function for create table if not exists
    virtual QString _createTableIfNotExists(QiModelMetaInfo *info) = 0;

    /// The real function for "insert into / replace into" statement
    virtual QString _insertInto(QiModelMetaInfo *info ,QString type, QStringList fields);

    virtual QString selectCore(QiQueryRules rules);

    virtual QString selectResultColumn(QiQueryRules rules);

    /// Generate the JOIN clauses (e.g. "INNER JOIN friendship ON ...")
    virtual QString joinClause(QiQueryRules rules);

    virtual QString limitAndOffset(int limit, int offset = 0);

    virtual QString orderBy(QiQueryRules rules);

};


#endif // QiSQLSTATEMENT_H
