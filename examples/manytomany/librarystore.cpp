#include "librarystore.h"
#include "models.h"
#include <QSet>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LibraryStore::LibraryStore(QObject *parent) : QObject(parent) {
    // Live lists: adding/removing a song, playlist or artist refreshes the views.
    m_songs.setLive<Song>(QiConnection::defaultConnection(), [] {
        return Song::objects().orderBy(Song::col().title.asc()).all();
    });
    m_playlists.setLive<Playlist>(QiConnection::defaultConnection(), [] {
        return Playlist::objects().orderBy(Playlist::col().name.asc()).all();
    });
    m_artists.setLive<Artist>(QiConnection::defaultConnection(), [] {
        return Artist::objects().orderBy(Artist::col().name.asc()).all();
    });

    backfillArtwork();   // fetch real album covers from the iTunes API
}

// --- Album art: found on the internet at runtime --------------------------

void LibraryStore::backfillArtwork() {
    QiList<Song> all = Song::objects().all();
    for (int i = 0; i < all.size(); i++) {
        Song *s = all.at(i);
        if (!s->artworkUrl->toString().isEmpty()) continue;
        const int aid = s->metaInfo()->value(s, "artist").toInt();
        fetchArtwork(s->id->toInt(), s->title->toString(), artistName(aid));
    }
}

void LibraryStore::fetchArtwork(int songId, const QString &title, const QString &artist) {
    QUrl url(QStringLiteral("https://itunes.apple.com/search"));
    QUrlQuery q;
    q.addQueryItem("term", title + " " + artist);
    q.addQueryItem("entity", "song");
    q.addQueryItem("limit", "1");
    url.setQuery(q);

    QNetworkReply *reply = m_net.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, songId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const QJsonArray results =
            QJsonDocument::fromJson(reply->readAll()).object().value("results").toArray();
        if (results.isEmpty()) return;
        QString art = results.at(0).toObject().value("artworkUrl100").toString();
        if (art.isEmpty()) return;
        art.replace(QStringLiteral("100x100bb"), QStringLiteral("300x300bb")); // crisper

        Song s;
        if (s.load(Song::col().id == songId)) {
            s.artworkUrl = art;
            s.save();          // live model refreshes -> QML Image loads the cover
            bump();
        }
    });
}

QAbstractItemModel *LibraryStore::songs()     { return &m_songs; }
QAbstractItemModel *LibraryStore::playlists() { return &m_playlists; }
QAbstractItemModel *LibraryStore::artists()   { return &m_artists; }

// --- one-to-many: Artist --< Song -----------------------------------------

QString LibraryStore::artistName(int artistId) {
    Artist a;
    return a.load(Artist::col().id == artistId) ? a.name->toString() : QString();
}

int LibraryStore::artistSongCount(int artistId) {
    Artist a;
    if (!a.load(Artist::col().id == artistId)) return 0;
    return a.songs().count();               // the one-to-many accessor
}

QVariantList LibraryStore::songsByArtist(int artistId) {
    QVariantList out;
    Artist a;
    if (!a.load(Artist::col().id == artistId)) return out;
    QiList<Song> list = a.songs().orderBy(Song::col().title.asc()).all();
    for (int i = 0; i < list.size(); i++) {
        Song *s = list.at(i);
        QVariantMap m;
        m["id"]    = s->id->toInt();
        m["title"] = s->title->toString();
        out << m;
    }
    return out;
}

void LibraryStore::bump() { m_revision++; emit revisionChanged(); }

void LibraryStore::toggle(int songId, int playlistId) {
    Song s; Playlist pl;
    if (!s.load(Song::col().id == songId))         return;
    if (!pl.load(Playlist::col().id == playlistId)) return;

    if (s.playlists().contains(pl))   // the declarative relation, both ways below
        s.playlists().remove(pl);
    else
        s.playlists().add(pl);
    bump();
}

bool LibraryStore::isLinked(int songId, int playlistId) {
    Song s; Playlist pl;
    if (!s.load(Song::col().id == songId))         return false;
    if (!pl.load(Playlist::col().id == playlistId)) return false;
    return s.playlists().contains(pl);
}

int LibraryStore::songCount(int playlistId) {
    Playlist pl;
    if (!pl.load(Playlist::col().id == playlistId)) return 0;
    return pl.songs().count();          // reverse direction
}

int LibraryStore::playlistCount(int songId) {
    Song s;
    if (!s.load(Song::col().id == songId)) return 0;
    return s.playlists().count();
}

QVariantList LibraryStore::playlistChips(int songId) {
    Song s;
    QSet<int> member;
    if (s.load(Song::col().id == songId)) {
        QiList<Playlist> mine = s.playlists().all();
        for (int i = 0; i < mine.size(); i++)
            member.insert(mine.at(i)->id->toInt());
    }
    QVariantList out;
    QiList<Playlist> all = Playlist::objects().orderBy(Playlist::col().name.asc()).all();
    for (int i = 0; i < all.size(); i++) {
        Playlist *p = all.at(i);
        const int pid = p->id->toInt();
        QVariantMap m;
        m["id"]     = pid;
        m["name"]   = p->name->toString();
        m["color"]  = p->color->toString();
        m["member"] = member.contains(pid);
        out << m;
    }
    return out;
}

QVariantList LibraryStore::songChips(int playlistId) {
    Playlist pl;
    QSet<int> member;
    if (pl.load(Playlist::col().id == playlistId)) {
        QiList<Song> mine = pl.songs().all();   // reverse direction of the relation
        for (int i = 0; i < mine.size(); i++)
            member.insert(mine.at(i)->id->toInt());
    }
    QVariantList out;
    QiList<Song> all = Song::objects().orderBy(Song::col().title.asc()).all();
    for (int i = 0; i < all.size(); i++) {
        Song *s = all.at(i);
        const int sid = s->id->toInt();
        QVariantMap m;
        m["id"]     = sid;
        m["title"]  = s->title->toString();
        m["artist"] = artistName(s->metaInfo()->value(s, "artist").toInt()); // FK -> name
        m["member"] = member.contains(sid);
        out << m;
    }
    return out;
}

void LibraryStore::addSong(const QString &title, const QString &artistName) {
    if (title.trimmed().isEmpty()) return;
    const QString aName = artistName.trimmed().isEmpty()
                              ? QStringLiteral("Unknown") : artistName.trimmed();

    // Find-or-create the artist (the "one" side of the one-to-many).
    Artist a;
    if (!a.load(Artist::col().name == aName)) {
        a.name = aName;
        a.save();
    }

    Song s;
    s.title  = title.trimmed();
    s.artist = a.id();          // set the foreign key
    s.save();
    fetchArtwork(s.id->toInt(), s.title->toString(), aName);   // find its cover online
    bump();
}

void LibraryStore::addPlaylist(const QString &name, const QString &color) {
    if (name.trimmed().isEmpty()) return;
    Playlist p; p.name = name.trimmed();
    p.color = color.isEmpty() ? QStringLiteral("#5E5CE6") : color;
    p.save();
    bump();
}

void LibraryStore::removeSong(int songId) {
    Song s;
    if (s.load(Song::col().id == songId)) {
        s.playlists().clear();          // drop its links first
        (void)s.remove();
        bump();
    }
}

void LibraryStore::removePlaylist(int playlistId) {
    Playlist p;
    if (p.load(Playlist::col().id == playlistId)) {
        p.songs().clear();
        (void)p.remove();
        bump();
    }
}
