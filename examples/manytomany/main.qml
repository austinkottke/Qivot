import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 430; height: 860
    color: "#0C0C10"
    title: "Library — Qivot many-to-many"

    LibraryStore { id: store }

    readonly property var colorChoices: ["#34C759","#FF375F","#0A84FF","#BF5AF2",
                                          "#FF9F0A","#5E5CE6","#FF2D55","#30D158"]
    function initial(s) { return s && s.length ? s.charAt(0).toUpperCase() : "?" }

    // A stable two-tone "album cover" gradient derived from the title.
    function cover(seed) {
        var h = 0;
        for (var i = 0; i < seed.length; i++) h = (h * 31 + seed.charCodeAt(i)) >>> 0;
        var hue = (h % 360) / 360;
        return [ Qt.hsla(hue, 0.68, 0.56, 1),
                 Qt.hsla(((h % 360 + 45) % 360) / 360, 0.72, 0.40, 1) ];
    }

    // ---- Header ----
    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 96; color: "#0C0C10"
        Column {
            anchors { left: parent.left; leftMargin: 18; bottom: parent.bottom; bottomMargin: 12 }
            spacing: 2
            Text { text: "🎵  Library"; color: "white"; font.pixelSize: 30; font.bold: true }
            Text { text: "Songs × Playlists — a many-to-many relation"
                   color: "#8A8A93"; font.pixelSize: 13 }
        }
    }

    // ---- Tabs ----
    TabBar {
        id: tabs
        anchors { left: parent.left; right: parent.right; top: header.bottom }
        background: Rectangle { color: "#0C0C10" }
        TabButton {
            contentItem: Text { text: "Songs"; color: tabs.currentIndex === 0 ? "white" : "#8A8A93"
                                font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            background: Rectangle { color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2
                            color: tabs.currentIndex === 0 ? "#0A84FF" : "transparent" } }
        }
        TabButton {
            contentItem: Text { text: "Playlists"; color: tabs.currentIndex === 1 ? "white" : "#8A8A93"
                                font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            background: Rectangle { color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2
                            color: tabs.currentIndex === 1 ? "#0A84FF" : "transparent" } }
        }
        TabButton {
            contentItem: Text { text: "Artists"; color: tabs.currentIndex === 2 ? "white" : "#8A8A93"
                                font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            background: Rectangle { color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2
                            color: tabs.currentIndex === 2 ? "#0A84FF" : "transparent" } }
        }
    }
    Rectangle { anchors.top: tabs.bottom; width: parent.width; height: 1; color: "#1E1E24" }

    StackLayout {
        anchors { top: tabs.bottom; left: parent.left; right: parent.right; bottom: parent.bottom
                  topMargin: 1 }
        currentIndex: tabs.currentIndex

        // ============================ SONGS ============================
        Item {
            ListView {
                id: songList
                anchors.fill: parent; clip: true
                model: store.songs
                spacing: 0
                topMargin: 6; bottomMargin: 90

                delegate: Item {
                    width: songList.width
                    height: Math.max(54, songInfo.implicitHeight) + 22
                    // membership chips re-evaluate whenever a link changes:
                    property var chips: (store.revision, store.playlistChips(model.id))
                    property var memberChips: chips.filter(function(c){ return c.member })

                    Rectangle { anchors.fill: parent
                        color: rowMouse.pressed ? "#17171C" : "transparent" }

                    Row {
                        anchors { left: parent.left; right: parent.right; top: parent.top
                                  leftMargin: 16; rightMargin: 16; topMargin: 11 }
                        spacing: 12
                        // "album art" cover
                        Rectangle {
                            id: songCover
                            width: 54; height: 54; radius: 11; clip: true
                            property var cc: win.cover(title)
                            // gradient + initial: the fallback while art loads (or offline)
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: songCover.cc[0] }
                                GradientStop { position: 1.0; color: songCover.cc[1] }
                            }
                            Text { anchors.centerIn: parent; text: win.initial(title)
                                   color: "white"; font.pixelSize: 22; font.bold: true
                                   visible: albumArt.status !== Image.Ready
                                   style: Text.Raised; styleColor: "#40000000" }
                            // real album cover, fetched from the internet
                            Image {
                                id: albumArt
                                anchors.fill: parent
                                source: model.artworkUrl ? model.artworkUrl : ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true; cache: true; smooth: true
                                visible: status === Image.Ready
                            }
                        }
                        Column {
                            id: songInfo
                            width: parent.width - 54 - 12 - 22
                            spacing: 4
                            Text { text: title; color: "white"; font.pixelSize: 16; font.bold: true
                                   width: parent.width; elide: Text.ElideRight }
                            // artist name resolved through the one-to-many foreign key:
                            Text { text: (store.revision, store.artistName(model.artist))
                                   color: "#8A8A93"; font.pixelSize: 13
                                   width: parent.width; elide: Text.ElideRight }
                            // Playlists this song is in:
                            Flow {
                                width: parent.width; spacing: 6; topPadding: 2
                                Repeater {
                                    model: memberChips
                                    Rectangle {
                                        height: 22; radius: 11
                                        width: cText.width + 20
                                        color: Qt.rgba(0,0,0,0); border.width: 1.5
                                        border.color: modelData.color
                                        Text { id: cText; anchors.centerIn: parent; text: modelData.name
                                               color: modelData.color; font.pixelSize: 11; font.bold: true }
                                    }
                                }
                                Text {
                                    visible: memberChips.length === 0
                                    text: "＋ add to playlists"; color: "#5E5E66"; font.pixelSize: 12
                                }
                            }
                        }
                    }
                    Text { text: "›"; color: "#48484E"; font.pixelSize: 22
                           anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter } }
                    Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left
                                anchors.leftMargin: 82; width: parent.width - 82; height: 1; color: "#17171C" }
                    MouseArea { id: rowMouse; anchors.fill: parent
                        onClicked: songEditor.open(model.id, title, model.artworkUrl) }
                }
            }
            RoundButton {
                text: "＋"; anchors { right: parent.right; bottom: parent.bottom; margins: 22 }
                width: 58; height: 58; font.pixelSize: 26
                background: Rectangle { radius: 29; color: "#0A84FF" }
                contentItem: Text { text: "＋"; color: "white"; font.pixelSize: 26
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: addSongDialog.open()
            }
        }

        // ========================== PLAYLISTS ==========================
        Item {
            ListView {
                id: plList
                anchors.fill: parent; clip: true
                model: store.playlists
                spacing: 10
                topMargin: 12; bottomMargin: 90; leftMargin: 16; rightMargin: 16

                delegate: Rectangle {
                    width: plList.width - 32; height: 72; radius: 16
                    color: "#16161C"
                    property int songs: (store.revision, store.songCount(model.id))

                    Row {
                        anchors.fill: parent; anchors.leftMargin: 14; spacing: 14
                        Rectangle {
                            id: plCover
                            width: 52; height: 52; radius: 13; clip: true
                            anchors.verticalCenter: parent.verticalCenter
                            property color base: model.color ? model.color : "#5E5CE6"
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: Qt.lighter(plCover.base, 1.15) }
                                GradientStop { position: 1.0; color: Qt.darker(plCover.base, 1.35) }
                            }
                            Text { anchors.centerIn: parent; text: "♪"; color: "white"; font.pixelSize: 24
                                   style: Text.Raised; styleColor: "#40000000" }
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter; spacing: 4
                            Text { text: name; color: "white"; font.pixelSize: 17; font.bold: true }
                            Text { text: songs + (songs === 1 ? " song" : " songs")
                                   color: model.color; font.pixelSize: 13; font.bold: true }
                        }
                    }
                    Text { text: "›"; color: "#48484E"; font.pixelSize: 22
                           anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter } }
                    MouseArea { anchors.fill: parent
                        onClicked: plEditor.open(model.id, name, model.color) }
                }
            }
            RoundButton {
                text: "＋"; anchors { right: parent.right; bottom: parent.bottom; margins: 22 }
                width: 58; height: 58
                background: Rectangle { radius: 29; color: "#BF5AF2" }
                contentItem: Text { text: "＋"; color: "white"; font.pixelSize: 26
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: addPlaylistDialog.open()
            }
        }

        // =========================== ARTISTS ===========================
        Item {
            ListView {
                id: artistList
                anchors.fill: parent; clip: true
                model: store.artists
                spacing: 10
                topMargin: 12; bottomMargin: 20; leftMargin: 16; rightMargin: 16

                delegate: Rectangle {
                    width: artistList.width - 32; height: 68; radius: 16
                    color: "#16161C"
                    property int nSongs: (store.revision, store.artistSongCount(model.id))

                    Row {
                        anchors.fill: parent; anchors.leftMargin: 14; spacing: 14
                        Rectangle {
                            id: aCover
                            width: 48; height: 48; radius: 24; clip: true
                            anchors.verticalCenter: parent.verticalCenter
                            property var cc: win.cover(name)
                            gradient: Gradient {
                                GradientStop { position: 0.0; color: aCover.cc[0] }
                                GradientStop { position: 1.0; color: aCover.cc[1] }
                            }
                            Text { anchors.centerIn: parent; text: win.initial(name)
                                   color: "white"; font.pixelSize: 20; font.bold: true }
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter; spacing: 4
                            Text { text: name; color: "white"; font.pixelSize: 17; font.bold: true }
                            Text { text: nSongs + (nSongs === 1 ? " song" : " songs")
                                   color: "#0A84FF"; font.pixelSize: 13; font.bold: true }
                        }
                    }
                    Text { text: "›"; color: "#48484E"; font.pixelSize: 22
                           anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter } }
                    MouseArea { anchors.fill: parent
                        onClicked: artistDetail.open(model.id, name) }
                }
            }
        }
    }

    // ---- Song editor: toggle which playlists this song is in ----
    Popup {
        id: songEditor
        property int songId: -1
        property string songTitle: ""
        property string songArt: ""
        function open(id, t, art) { songId = id; songTitle = t; songArt = art || ""; visible = true }

        width: parent.width; height: parent.height * 0.62
        y: parent.height - height
        modal: true; dim: true
        background: Rectangle { color: "#161620"; radius: 22 }
        Overlay.modal: Rectangle { color: "#AA000000" }

        property var chips: (store.revision, songEditor.visible ? store.playlistChips(songId) : [])

        contentItem: Column {
            spacing: 4
            Rectangle { width: 40; height: 4; radius: 2; color: "#3A3A44"; anchors.horizontalCenter: parent.horizontalCenter }
            Row {
                spacing: 12; topPadding: 8; leftPadding: 2
                Rectangle {
                    id: edCover
                    width: 52; height: 52; radius: 11; clip: true
                    property var cc: win.cover(songEditor.songTitle)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: edCover.cc[0] }
                        GradientStop { position: 1.0; color: edCover.cc[1] }
                    }
                    Text { anchors.centerIn: parent; text: win.initial(songEditor.songTitle)
                           color: "white"; font.pixelSize: 22; font.bold: true
                           visible: edArt.status !== Image.Ready }
                    Image {
                        id: edArt; anchors.fill: parent
                        source: songEditor.songArt
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true; cache: true; smooth: true
                        visible: status === Image.Ready
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Text { text: songEditor.songTitle; color: "white"; font.pixelSize: 20; font.bold: true }
                    Text { text: "Tap a playlist to add or remove"; color: "#8A8A93"; font.pixelSize: 13 }
                }
            }
            Item { width: 1; height: 8 }
            Flow {
                width: parent.width; spacing: 10
                Repeater {
                    model: songEditor.chips
                    Rectangle {
                        height: 40; radius: 20; width: pText.width + 40
                        color: modelData.member ? modelData.color : "#00000000"
                        border.width: modelData.member ? 0 : 1.5
                        border.color: modelData.color
                        Row {
                            anchors.centerIn: parent; spacing: 6
                            Text { visible: modelData.member; text: "✓"; color: "white"; font.pixelSize: 14; font.bold: true }
                            Text { id: pText; text: modelData.name
                                   color: modelData.member ? "white" : modelData.color
                                   font.pixelSize: 14; font.bold: true }
                        }
                        MouseArea { anchors.fill: parent
                            onClicked: store.toggle(songEditor.songId, modelData.id) }
                    }
                }
            }
        }
    }

    // ---- Playlist editor: toggle which songs are in this playlist ----
    Popup {
        id: plEditor
        property int plId: -1
        property string plName: ""
        property string plColor: "#5E5CE6"
        function open(id, n, c) { plId = id; plName = n; plColor = c || "#5E5CE6"; visible = true }

        width: parent.width; height: parent.height * 0.72
        y: parent.height - height
        modal: true; dim: true
        background: Rectangle { color: "#161620"; radius: 22 }
        Overlay.modal: Rectangle { color: "#AA000000" }

        property var songs: (store.revision, plEditor.visible ? store.songChips(plId) : [])

        contentItem: Column {
            spacing: 4
            Rectangle { width: 40; height: 4; radius: 2; color: "#3A3A44"; anchors.horizontalCenter: parent.horizontalCenter }
            Row {
                spacing: 10; topPadding: 8; leftPadding: 4
                Rectangle { width: 30; height: 30; radius: 8; color: plEditor.plColor
                            Text { anchors.centerIn: parent; text: "♪"; color: "white"; font.pixelSize: 15 } }
                Text { text: plEditor.plName; color: "white"; font.pixelSize: 20; font.bold: true
                       anchors.verticalCenter: parent.verticalCenter }
            }
            Text { text: "Tap a song to add or remove it"; color: "#8A8A93"; font.pixelSize: 13
                   bottomPadding: 6; leftPadding: 4 }

            ListView {
                width: parent.width; height: plEditor.height - 130
                clip: true; model: plEditor.songs; spacing: 2
                delegate: Rectangle {
                    width: ListView.view.width; height: 52; radius: 10
                    color: modelData.member ? Qt.rgba(1,1,1,0.06) : "transparent"
                    Row {
                        anchors.fill: parent; anchors.leftMargin: 8; spacing: 12
                        Rectangle {
                            width: 34; height: 34; radius: 17; anchors.verticalCenter: parent.verticalCenter
                            color: modelData.member ? plEditor.plColor : "#2A2A32"
                            Text { anchors.centerIn: parent; text: modelData.member ? "✓" : "＋"
                                   color: "white"; font.pixelSize: 15; font.bold: true }
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            Text { text: modelData.title; color: "white"; font.pixelSize: 15
                                   font.bold: modelData.member }
                            Text { text: modelData.artist; color: "#8A8A93"; font.pixelSize: 12 }
                        }
                    }
                    MouseArea { anchors.fill: parent
                        onClicked: store.toggle(modelData.id, plEditor.plId) }
                }
            }
        }
    }

    // ---- Artist detail: the songs by this artist (one-to-many) ----
    Popup {
        id: artistDetail
        property int aId: -1
        property string aName: ""
        function open(id, n) { aId = id; aName = n; visible = true }

        width: parent.width; height: parent.height * 0.60
        y: parent.height - height
        modal: true; dim: true
        background: Rectangle { color: "#161620"; radius: 22 }
        Overlay.modal: Rectangle { color: "#AA000000" }

        property var list: (store.revision, artistDetail.visible ? store.songsByArtist(aId) : [])

        contentItem: Column {
            spacing: 4
            Rectangle { width: 40; height: 4; radius: 2; color: "#3A3A44"; anchors.horizontalCenter: parent.horizontalCenter }
            Row {
                spacing: 12; topPadding: 8; leftPadding: 2
                Rectangle {
                    id: adCover
                    width: 52; height: 52; radius: 26; clip: true
                    property var cc: win.cover(artistDetail.aName)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: adCover.cc[0] }
                        GradientStop { position: 1.0; color: adCover.cc[1] }
                    }
                    Text { anchors.centerIn: parent; text: win.initial(artistDetail.aName)
                           color: "white"; font.pixelSize: 22; font.bold: true }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter; spacing: 2
                    Text { text: artistDetail.aName; color: "white"; font.pixelSize: 20; font.bold: true }
                    Text { text: artistDetail.list.length + (artistDetail.list.length === 1 ? " song" : " songs")
                           color: "#8A8A93"; font.pixelSize: 13 }
                }
            }
            Item { width: 1; height: 8 }
            ListView {
                width: parent.width; height: artistDetail.height - 150; clip: true
                model: artistDetail.list; spacing: 2
                delegate: Rectangle {
                    width: ListView.view.width; height: 46; radius: 10; color: "transparent"
                    Row {
                        anchors.fill: parent; anchors.leftMargin: 6; spacing: 12
                        Text { text: "♪"; color: "#0A84FF"; font.pixelSize: 16
                               anchors.verticalCenter: parent.verticalCenter }
                        Text { text: modelData.title; color: "white"; font.pixelSize: 15
                               anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }
    }

    // ---- Add song ----
    Dialog {
        id: addSongDialog
        anchors.centerIn: parent; width: 320; modal: true
        title: "New Song"
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: { store.addSong(sTitle.text, sArtist.text); sTitle.clear(); sArtist.clear() }
        onRejected: { sTitle.clear(); sArtist.clear() }
        contentItem: ColumnLayout {
            spacing: 8
            TextField { id: sTitle; Layout.fillWidth: true; placeholderText: "Title" }
            TextField { id: sArtist; Layout.fillWidth: true; placeholderText: "Artist" }
        }
    }

    // ---- Add playlist (with color choice) ----
    Dialog {
        id: addPlaylistDialog
        anchors.centerIn: parent; width: 320; modal: true
        title: "New Playlist"
        standardButtons: Dialog.Ok | Dialog.Cancel
        property string chosen: win.colorChoices[0]
        onAccepted: { store.addPlaylist(plName2.text, chosen); plName2.clear() }
        onRejected: plName2.clear()
        contentItem: ColumnLayout {
            spacing: 10
            TextField { id: plName2; Layout.fillWidth: true; placeholderText: "Name" }
            Flow {
                Layout.fillWidth: true; spacing: 8
                Repeater {
                    model: win.colorChoices
                    Rectangle {
                        width: 30; height: 30; radius: 15; color: modelData
                        border.width: addPlaylistDialog.chosen === modelData ? 3 : 0
                        border.color: "white"
                        MouseArea { anchors.fill: parent; onClicked: addPlaylistDialog.chosen = modelData }
                    }
                }
            }
        }
    }
}
