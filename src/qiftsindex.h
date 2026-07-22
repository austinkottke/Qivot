#ifndef QiFTSINDEX_H
#define QiFTSINDEX_H

#include <QString>
#include <QStringList>
#include <qimodelmetainfo.h>

/// Full-text search (FTS5) index declaration for a model
/**
  QiFtsIndex declares a full-text search index over one or more text columns of
  a model. Creating it (via QiConnection::createFtsIndex) builds an FTS5
  external-content virtual table plus triggers that keep it in sync with the
  model's table automatically, so ordinary save() / upsert() / remove() calls
  update the search index.

\code
    QiFtsIndex<Article> fts("article_fts");
    fts << "title" << "body";          // the searchable columns
    connection.createFtsIndex(fts);    // creates the FTS5 table + sync triggers

    // later:
    QiList<Article> hits = QiQuery<Article>().search("article_fts", "sqlite AND orm").all();
\endcode

  @remarks Requires SQLite built with FTS5 (SQLite 3.9+; Qt's bundled SQLite has it).
 */
class QiBaseFtsIndex {
public:
    /// Construct an FTS index
    /**
      @param metaInfo The meta info of the indexed model.
      @param name The name of the FTS5 virtual table to create.
     */
    QiBaseFtsIndex(QiModelMetaInfo *metaInfo, QString name)
        : m_metaInfo(metaInfo) , m_name(name) {
    }

    /// The meta info of the indexed model
    QiModelMetaInfo *metaInfo() const { return m_metaInfo; }

    /// The name of the FTS5 virtual table
    QString name() const { return m_name; }

    /// The list of indexed (searchable) columns
    QStringList columns() const { return m_columns; }

    /// Set the indexed columns
    void setColumns(QStringList columns) { m_columns = columns; }

    /// Append an indexed column
    QiBaseFtsIndex &operator<<(QString column) {
        m_columns << column;
        return *this;
    }

private:
    QiModelMetaInfo *m_metaInfo;
    QString m_name;
    QStringList m_columns;
};

/// A typed full-text search index over model T
template <typename T>
class QiFtsIndex : public QiBaseFtsIndex {
public:
    /// Construct an FTS index named @p name over model T
    QiFtsIndex(QString name) : QiBaseFtsIndex(qiMetaInfo<T>() , name) {
    }
};

#endif // QiFTSINDEX_H
