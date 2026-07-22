#include "poststore.h"
#include "post.h"
#include <QDebug>

PostStore::PostStore(QObject *parent) : QObject(parent) {
}

QAbstractItemModel *PostStore::posts() {
    return &m_model;
}

QString PostStore::status() const {
    return m_status;
}

void PostStore::setStatus(const QString &status) {
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

void PostStore::reload() {
    m_model.setList( Post::objects().orderBy(Post::col().remoteId.desc()).all() );
    setStatus(QString("%1 posts").arg(m_model.count()));
    qDebug() << "[PostStore] reload ->" << m_model.count() << "posts";
}

void PostStore::add(const QString &title, const QString &author) {
    int nextId = Post::objects().call("max", "remoteId").toInt() + 1;   // next natural key
    Post post;
    post.remoteId = nextId;
    post.title    = title;
    post.author   = author.isEmpty() ? QStringLiteral("anon") : author;
    post.save();
    reload();
    qDebug() << "[PostStore] add ->" << nextId << title;
}

void PostStore::remove(int remoteId) {
    Post post;
    if (post.load(Post::col().remoteId == remoteId))
        (void) post.remove();
    reload();
    qDebug() << "[PostStore] remove ->" << remoteId;
}

void PostStore::search(const QString &text) {
    if (text.trimmed().isEmpty()) {
        reload();
        return;
    }
    m_model.setList( QiQuery<Post>().search("post_fts", text).all() );
    setStatus(QString("%1 matches for \"%2\"").arg(m_model.count()).arg(text));
    qDebug() << "[PostStore] search" << text << "->" << m_model.count();
}
