import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 430; height: 820
    color: "#08080B"
    title: "Discover — Qivot infinite scroll"

    ItemStore { id: store }

    // Track how many rows have streamed in, to flash a toast on each new page.
    property int lastCount: 0
    property bool primed: false
    Connections {
        target: store.items
        function onCountChanged() {
            var delta = store.items.count - win.lastCount;
            win.lastCount = store.items.count;
            if (win.primed && delta > 0) toast.flash(delta);
            win.primed = true;
        }
    }

    // ---- The feed ----
    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: store.items
        spacing: 16
        topMargin: 118          // clear the floating header
        bottomMargin: 8
        leftMargin: 16; rightMargin: 16
        boundsBehavior: Flickable.OvershootBounds
        maximumFlickVelocity: 5000
        cacheBuffer: 600

        delegate: Item {
            id: cell
            width: list.width - 32
            height: 208

            // Soft drop shadow (no external effects needed).
            Rectangle {
                anchors.fill: card; anchors.topMargin: 10
                radius: 24; color: "#000000"; opacity: 0.35; z: -1
            }

            Rectangle {
                id: card
                anchors.fill: parent
                radius: 24; clip: true; color: colorA

                // Diagonal gradient: an oversized, rotated gradient layer.
                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 1.8; height: parent.height * 1.8
                    rotation: 35
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: colorA }
                        GradientStop { position: 1.0; color: colorB }
                    }
                }

                // Generative translucent orbs for depth.
                Repeater {
                    model: 3
                    Rectangle {
                        property real s: 70 + ((index * 53 + metric) % 90)
                        width: s; height: s; radius: s / 2
                        color: index % 2 ? "#26FFFFFF" : "#1AFFFFFF"
                        x: ((index * 97 + metric * 3) % 100) / 100 * (card.width - s)
                        y: ((index * 41 + metric * 7) % 100) / 100 * (card.height - s)
                    }
                }

                // Subtle darkening at the bottom so text always reads.
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.45; color: "#00000000" }
                        GradientStop { position: 1.0;  color: "#66000000" }
                    }
                }

                // Top row: category pill + row number.
                Row {
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                    Rectangle {
                        radius: 11; height: 22; color: "#33FFFFFF"
                        width: catText.width + 20
                        Text { id: catText; anchors.centerIn: parent; text: subtitle
                               color: "white"; font.pixelSize: 12; font.bold: true }
                    }
                    Item { width: parent.width - catText.width - 20 - noText.width; height: 1 }
                    Text { id: noText; text: "#" + (index + 1); color: "#E6FFFFFF"
                           font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                }

                // Bottom: title + metric.
                Column {
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 18 }
                    spacing: 8
                    Text {
                        text: title; color: "white"; font.pixelSize: 30; font.bold: true
                        width: parent.width; elide: Text.ElideRight
                        style: Text.Raised; styleColor: "#40000000"
                    }
                    Row {
                        spacing: 8
                        Rectangle {
                            radius: 13; height: 26; color: "#2EFFFFFF"
                            width: mText.width + 26
                            Text { id: mText; anchors.centerIn: parent
                                   text: "✦  " + metric.toLocaleString()
                                   color: "white"; font.pixelSize: 13; font.bold: true }
                        }
                        Rectangle {
                            radius: 13; height: 26; color: "#2EFFFFFF"
                            width: sText.width + 26
                            Text { id: sText; anchors.centerIn: parent; text: "streamed from SQLite"
                                   color: "#F2FFFFFF"; font.pixelSize: 12 }
                        }
                    }
                }

                // Press feedback.
                scale: cardMouse.pressed ? 0.975 : 1.0
                Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
                MouseArea { id: cardMouse; anchors.fill: parent }
            }

            // Entrance: rise + fade the first time this delegate is created.
            opacity: 0
            transform: Translate {
                id: rise; y: 28
                Behavior on y { NumberAnimation { duration: 460; easing.type: Easing.OutCubic } }
            }
            Behavior on opacity { NumberAnimation { duration: 420; easing.type: Easing.OutCubic } }
            Component.onCompleted: { opacity = 1; rise.y = 0; }
        }

        // ---- Footer: a live "fetching" state that shows the exact rows ----
        footer: Item {
            width: list.width; height: 118
            // Loading state.
            Column {
                anchors.centerIn: parent; spacing: 12
                visible: !store.items.atEnd
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter; spacing: 7
                    Repeater {
                        model: 3
                        Rectangle {
                            width: 9; height: 9; radius: 4.5; color: "white"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite; running: !store.items.atEnd
                                PauseAnimation { duration: index * 150 }
                                NumberAnimation { to: 0.2; duration: 320 }
                                NumberAnimation { to: 1.0; duration: 320 }
                                PauseAnimation { duration: (2 - index) * 150 }
                            }
                        }
                    }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Fetching rows " + (store.items.count + 1) + "–" +
                          (store.items.count + store.pageSize) + " from SQLite…"
                    color: "#8A8A93"; font.pixelSize: 13
                }
            }
            // End state.
            Column {
                anchors.centerIn: parent; spacing: 6
                visible: store.items.atEnd
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       text: "✓  You've reached the end"; color: "#EDEDED"
                       font.pixelSize: 15; font.bold: true }
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       text: store.items.count.toLocaleString() + " cards · streamed " +
                             store.pageSize + " at a time"
                       color: "#7A7A83"; font.pixelSize: 12 }
            }
        }

        // Thin auto-hiding scrollbar.
        ScrollBar.vertical: ScrollBar {
            id: vbar; policy: ScrollBar.AsNeeded; width: 6
            contentItem: Rectangle {
                implicitWidth: 4; radius: 2
                color: "#66FFFFFF"; opacity: vbar.active ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 300 } }
            }
        }
    }

    // ---- Floating glass header ----
    Rectangle {
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 104; color: "#E60A0A0F"
        Column {
            anchors { left: parent.left; leftMargin: 18; bottom: parent.bottom; bottomMargin: 16 }
            spacing: 3
            Text { text: "Discover"; color: "white"; font.pixelSize: 32; font.bold: true }
            Text { text: "Infinite feed · streamed from SQLite, " + store.pageSize + " at a time"
                   color: "#8A8A93"; font.pixelSize: 13 }
        }
        // Live "loaded" counter pill.
        Rectangle {
            anchors { right: parent.right; rightMargin: 16; bottom: parent.bottom; bottomMargin: 18 }
            radius: 15; height: 30; width: cnt.width + 28; color: "#1CFFFFFF"
            Text { id: cnt; anchors.centerIn: parent
                   text: "◆  " + store.items.count.toLocaleString() + " loaded"
                   color: "white"; font.pixelSize: 13; font.bold: true }
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#1AFFFFFF" }
    }

    // ---- "＋N loaded" toast, flashed on every fetched page ----
    Rectangle {
        id: toast
        anchors { top: parent.top; topMargin: 116; horizontalCenter: parent.horizontalCenter }
        radius: 17; height: 34; width: toastText.width + 34
        color: "#F0202028"; border.color: "#22FFFFFF"; border.width: 1
        opacity: 0
        Text { id: toastText; anchors.centerIn: parent; color: "white"
               font.pixelSize: 13; font.bold: true }
        Behavior on opacity { NumberAnimation { duration: 220 } }
        Timer { id: toastTimer; interval: 1000; onTriggered: toast.opacity = 0 }
        function flash(n) { toastText.text = "＋" + n + " loaded"; opacity = 1; toastTimer.restart(); }
    }
}
