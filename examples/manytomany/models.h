#ifndef MODELS_H
#define MODELS_H
#include <qivot.h>

// A music library with two kinds of relation:
//
//   one-to-many :  Artist  --<  Song      (a Song has one Artist; an Artist has
//                                           many Songs — via a foreign key)
//   many-to-many:  Song  >--<  Playlist   (over the auto-created "song_playlist"
//                                           join table)

class Song;       // forward declarations for the relation accessors
class Playlist;

class Artist : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;

    QI_HAS_MANY(Song, songs, "artist")          // one-to-many: artist.songs()
};
QI_DECLARE_MODEL(Artist, "artist", QI_FIELD(name));

class Song : public QiModel {
    QI_MODEL
public:
    QiField<QString>     title;
    QiForeignKey<Artist> artist;                // foreign key -> artist(id)
    QiField<QString>     artworkUrl;            // album art, fetched at runtime

    QI_MANY_TO_MANY(Playlist, playlists, "song_playlist")   // song.playlists()
};
QI_DECLARE_MODEL(Song, "song",
                 QI_FIELD(title), QI_FIELD(artist), QI_FIELD(artworkUrl));

class Playlist : public QiModel {
    QI_MODEL
public:
    QiField<QString> name;
    QiField<QString> color;                     // hex, for the UI

    QI_MANY_TO_MANY(Song, songs, "song_playlist")           // playlist.songs()
};
QI_DECLARE_MODEL(Playlist, "playlist", QI_FIELD(name), QI_FIELD(color));

#endif // MODELS_H
