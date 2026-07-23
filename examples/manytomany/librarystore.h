#ifndef LIBRARYSTORE_H
#define LIBRARYSTORE_H

#include <QObject>
#include <QAbstractItemModel>
#include <QVariantList>
#include <QQmlEngine>          // QML_ELEMENT
#include <QNetworkAccessManager>
#include <qilistmodel.h>

/// QML controller for the many-to-many music library.
/**
  Exposes live `songs` and `playlists` models plus invokables that drive the
  Song<->Playlist relation. `revision` bumps on every link change so QML
  membership bindings re-evaluate. All linking goes through the declarative
  QiRelationSet API (song.playlists() / playlist.songs()).
 */
class LibraryStore : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *songs     READ songs     CONSTANT)
    Q_PROPERTY(QAbstractItemModel *playlists READ playlists CONSTANT)
    Q_PROPERTY(QAbstractItemModel *artists   READ artists   CONSTANT)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
public:
    explicit LibraryStore(QObject *parent = nullptr);

    QAbstractItemModel *songs();
    QAbstractItemModel *playlists();
    QAbstractItemModel *artists();
    int revision() const { return m_revision; }

    // --- one-to-many: Artist --< Song ---
    Q_INVOKABLE QString artistName(int artistId);        // resolve a song's artist
    Q_INVOKABLE int     artistSongCount(int artistId);   // artist.songs().count()
    Q_INVOKABLE QVariantList songsByArtist(int artistId);// [{id, title}]

    /// Toggle whether `songId` is in `playlistId` (add if absent, else remove).
    Q_INVOKABLE void toggle(int songId, int playlistId);
    Q_INVOKABLE bool isLinked(int songId, int playlistId);

    Q_INVOKABLE int songCount(int playlistId);       // # songs in a playlist
    Q_INVOKABLE int playlistCount(int songId);       // # playlists a song is in

    /// [{id, name, color, member}] for every playlist, w.r.t. a song.
    Q_INVOKABLE QVariantList playlistChips(int songId);
    /// [{id, title, artist, member}] for every song, w.r.t. a playlist.
    Q_INVOKABLE QVariantList songChips(int playlistId);

    Q_INVOKABLE void addSong(const QString &title, const QString &artist);
    Q_INVOKABLE void addPlaylist(const QString &name, const QString &color);
    Q_INVOKABLE void removeSong(int songId);
    Q_INVOKABLE void removePlaylist(int playlistId);

signals:
    void revisionChanged();

private:
    void bump();
    // Look up album art on the iTunes Search API and store it on the song.
    void fetchArtwork(int songId, const QString &title, const QString &artist);
    void backfillArtwork();

    QiListModel m_songs;
    QiListModel m_playlists;
    QiListModel m_artists;
    QNetworkAccessManager m_net;
    int m_revision = 0;
};

#endif // LIBRARYSTORE_H
