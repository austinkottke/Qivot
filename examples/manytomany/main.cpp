/** A comprehensive many-to-many QML app: a music library.

    Songs and Playlists are linked many-to-many (a song is in many playlists; a
    playlist has many songs) through Qivot's declarative relation — declared once
    on each model with QI_MANY_TO_MANY, backed by an auto-created join table.

    The UI lets you browse songs, toggle which playlists each belongs to, browse
    playlists and see/remove their songs, and add new songs and playlists — all
    updating live.

    QIVOT_SELFTEST=1 drives a few relation operations then quits (headless check).
 */
#include "models.h"
#include "librarystore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QTimer>
#include <QDebug>

static Artist mkArtist(const QString &n) {
    Artist a; a.name = n; a.save(); return a;
}
static Song mkSong(const QString &t, Artist &a) {
    Song s; s.title = t; s.artist = a.id(); s.save(); return s;   // set the FK
}
static Playlist mkPlaylist(const QString &n, const QString &c) {
    Playlist p; p.name = n; p.color = c; p.save(); return p;
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    // In-memory DB: a fresh, self-contained library on every launch.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    db.open();

    QiConnection connection;
    if (!connection.open(db)) return 1;
    connection.addModel<Artist>();
    connection.addModel<Song>();
    connection.addModel<Playlist>();
    if (!connection.createTables()) return 1;
    // The song_playlist join table is created automatically on the first link.

    // --- Seed playlists ---
    Playlist chill   = mkPlaylist("Chill",    "#34C759");
    Playlist workout = mkPlaylist("Workout",  "#FF375F");
    Playlist focus   = mkPlaylist("Focus",    "#0A84FF");
    Playlist party   = mkPlaylist("Party",    "#BF5AF2");
    Playlist road    = mkPlaylist("Roadtrip", "#FF9F0A");

    // --- Seed artists (the "one" side of the one-to-many) ---
    Artist kavinsky = mkArtist("Kavinsky");
    Artist m83      = mkArtist("M83");
    Artist daftpunk = mkArtist("Daft Punk");
    Artist tame     = mkArtist("Tame Impala");
    Artist gambino  = mkArtist("Childish Gambino");
    Artist marconi  = mkArtist("Marconi Union");
    Artist weeknd   = mkArtist("The Weeknd");
    Artist biscuit  = mkArtist("Petit Biscuit");
    Artist dualipa  = mkArtist("Dua Lipa");
    Artist zimmer   = mkArtist("Hans Zimmer");

    // --- Seed songs, each linked to its artist by foreign key ---
    Song s1  = mkSong("Nightcall",        kavinsky);
    Song s2  = mkSong("Midnight City",    m83);
    Song s3  = mkSong("Instant Crush",    daftpunk);   // Daft Punk again below
    Song s4  = mkSong("The Less I Know",  tame);
    Song s5  = mkSong("Redbone",          gambino);
    Song s6  = mkSong("Weightless",       marconi);
    Song s7  = mkSong("Blinding Lights",  weeknd);
    Song s8  = mkSong("Sunset Lover",     biscuit);
    Song s9  = mkSong("Levitating",       dualipa);
    Song s10 = mkSong("Time",             zimmer);
    Song s11 = mkSong("Get Lucky",        daftpunk);   // second Daft Punk song
    Song s12 = mkSong("Save Your Tears",  weeknd);      // second Weeknd song

    // --- Seed some links (the declarative relation) ---
    s1.playlists().add(chill);  s1.playlists().add(road);
    s2.playlists().add(chill);  s2.playlists().add(party);   s2.playlists().add(road);
    s3.playlists().add(party);  s3.playlists().add(workout);
    s4.playlists().add(chill);  s4.playlists().add(focus);
    s5.playlists().add(chill);
    s6.playlists().add(focus);
    s7.playlists().add(workout); s7.playlists().add(party);
    s8.playlists().add(chill);  s8.playlists().add(road);
    s9.playlists().add(workout); s9.playlists().add(party);  s9.playlists().add(road);
    s10.playlists().add(focus);
    s11.playlists().add(party);  s11.playlists().add(workout);
    s12.playlists().add(chill);

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (qEnvironmentVariableIsSet("QIVOT_SELFTEST")) {
        QObject *root = engine.rootObjects().first();
        LibraryStore *store = root->findChild<LibraryStore *>();
        const int songId = s1.id->toInt();
        const int chillId = chill.id->toInt();
        const int focusId = focus.id->toInt();
        const int daftId = daftpunk.id->toInt();
        const int weekndId = weeknd.id->toInt();
        const int kavinskyId = kavinsky.id->toInt();
        QTimer::singleShot(300, &app, [=] {
            if (!store) return;
            // many-to-many
            qInfo() << "s1 playlists:" << store->playlistCount(songId);      // 2 (chill, road)
            store->toggle(songId, focusId);                                  // add focus -> 3
            qInfo() << "after +focus:" << store->playlistCount(songId);
            store->toggle(songId, chillId);                                  // remove chill -> 2
            qInfo() << "after -chill:" << store->playlistCount(songId);
            qInfo() << "chill songs:" << store->songCount(chillId);
            // one-to-many (Artist --< Song)
            qInfo() << "Daft Punk songs:" << store->artistSongCount(daftId); // 2
            qInfo() << "Weeknd songs:" << store->artistSongCount(weekndId);  // 2
            qInfo() << "s1 artist:" << store->artistName(kavinskyId);        // Kavinsky
            store->addSong("Something", "Daft Punk");                        // find-or-create -> Daft Punk now 3
            qInfo() << "Daft Punk after add:" << store->artistSongCount(daftId);
        });
        // Give the album-art network fetches time to land, then report one.
        QTimer::singleShot(3500, &app, [=] {
            Song s;
            if (s.load(Song::col().title == "Nightcall"))
                qInfo() << "Nightcall artwork:" << s.artworkUrl->toString();
        });
        QTimer::singleShot(4000, &app, &QCoreApplication::quit);
    }
    return app.exec();
}
