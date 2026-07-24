import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1200; height: 820
    color: "#06070F"
    title: "Fluxo — Qivot write recorder"

    readonly property color accent:  "#7C5CFF"
    readonly property color accent2: "#38E1FF"

    function fmt(n) { return Number(n).toLocaleString(Qt.locale(), 'f', 0) }

    // ============================ the canvas ============================
    FluxView {
        id: flux
        anchors.fill: parent
        particleCount: 1800
    }

    // subtle vignette so the UI panels sit on something
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#33000000" }
            GradientStop { position: 0.25; color: "#00000000" }
            GradientStop { position: 0.8; color: "#00000000" }
            GradientStop { position: 1.0; color: "#66000000" }
        }
    }

    // ============================ header ============================
    Column {
        x: 26; y: 22; spacing: 2
        Row {
            spacing: 10
            Rectangle { width: 12; height: 12; radius: 6; color: win.accent2
                        anchors.verticalCenter: parent.verticalCenter
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite; running: !flux.replaying && flux.running
                            NumberAnimation { to: 0.25; duration: 700 }
                            NumberAnimation { to: 1.0;  duration: 700 } } }
            Text { text: "Fluxo"; color: "white"; font.pixelSize: 30; font.bold: true }
        }
        Text {
            text: "Every particle, every frame → SQLite.  Drag the timeline to replay from the database."
            color: "#8A8B99"; font.pixelSize: 13
        }
    }

    // ============================ stats HUD ============================
    Rectangle {
        id: hud
        x: 26; y: 92; width: 232
        height: hudCol.height + 32
        radius: 16
        color: "#CC0C0E1A"
        border.color: "#1EFFFFFF"; border.width: 1

        Column {
            id: hudCol
            x: 18; y: 16; width: parent.width - 36; spacing: 14

            // headline: writes / sec
            Column {
                spacing: -2
                Text { text: "WRITES / SEC"; color: "#6F7180"; font.pixelSize: 10
                       font.letterSpacing: 1.5; font.bold: true }
                Text { text: win.fmt(flux.writesPerSec); color: "white"
                       font.pixelSize: 40; font.bold: true
                       font.family: "Menlo, Consolas, monospace" }
            }

            Rectangle { width: parent.width; height: 1; color: "#14FFFFFF" }

            Repeater {
                model: [
                    { k: "rows written",  v: win.fmt(flux.rowsWritten), a: true },
                    { k: "rows in table", v: win.fmt(flux.liveRows) },
                    { k: "db size",       v: Number(flux.dbSizeMB).toFixed(1) + " MB" },
                    { k: "batch write",   v: Number(flux.batchMs).toFixed(2) + " ms" },
                    { k: "frames kept",   v: win.fmt(flux.headFrame - flux.minFrame) },
                    { k: "particles",     v: win.fmt(flux.particleCount) }
                ]
                Row {
                    width: hudCol.width
                    Text { text: modelData.k; color: "#8A8B99"; font.pixelSize: 13
                           width: parent.width * 0.52 }
                    Text { text: modelData.v
                           color: modelData.a ? win.accent2 : "#EDEEF5"
                           font.pixelSize: 14; font.bold: true
                           font.family: "Menlo, Consolas, monospace"
                           horizontalAlignment: Text.AlignRight
                           width: parent.width * 0.48 }
                }
            }
        }
    }

    // ============================ control dock ============================
    Rectangle {
        id: dock
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                  margins: 20 }
        height: 96
        radius: 18
        color: "#D90A0C16"
        border.color: "#1EFFFFFF"; border.width: 1

        // ---- play / pause ----
        Rectangle {
            id: playBtn
            x: 18; anchors.verticalCenter: parent.verticalCenter
            width: 52; height: 52; radius: 26
            color: playMouse.pressed ? "#3A2F6E" : "#241E44"
            border.color: win.accent; border.width: 1
            Text {
                anchors.centerIn: parent
                text: flux.running ? "⏸" : "▶"
                color: "white"; font.pixelSize: 20
                x: flux.running ? 0 : 1
            }
            MouseArea { id: playMouse; anchors.fill: parent
                        onClicked: flux.running = !flux.running }
            scale: playMouse.pressed ? 0.94 : 1.0
            Behavior on scale { NumberAnimation { duration: 90 } }
        }

        // ---- timeline scrubber ----
        Item {
            anchors { left: playBtn.right; leftMargin: 20
                      right: rightCol.left; rightMargin: 20
                      verticalCenter: parent.verticalCenter }
            height: 56

            Row {
                width: parent.width; spacing: 8
                Text {
                    text: flux.replaying ? ("◀ REPLAY  ·  frame " + win.fmt(flux.scrubFrame))
                                         : "● LIVE"
                    color: flux.replaying ? win.accent2 : "#63E6A0"
                    font.pixelSize: 12; font.bold: true; font.letterSpacing: 1
                }
                Item { width: parent.width - 340; height: 1 }
                Text {
                    visible: flux.replaying
                    text: "Resume live  ▶"
                    color: win.accent2; font.pixelSize: 12; font.bold: true
                    MouseArea { anchors.fill: parent; anchors.margins: -8
                                onClicked: flux.live() }
                }
                Text {
                    text: "history: " + win.fmt(flux.minFrame) + " – " + win.fmt(flux.headFrame)
                    color: "#6F7180"; font.pixelSize: 12
                }
            }

            Slider {
                id: timeline
                anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                from: flux.minFrame
                to:   Math.max(flux.minFrame + 1, flux.headFrame - 1)
                live: true
                onMoved: flux.scrubFrame = Math.round(value)
                background: Rectangle {
                    x: timeline.leftPadding; y: timeline.topPadding + timeline.availableHeight/2 - 2
                    width: timeline.availableWidth; height: 4; radius: 2; color: "#22FFFFFF"
                    Rectangle {
                        width: timeline.visualPosition * parent.width
                        height: parent.height; radius: 2
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: win.accent }
                            GradientStop { position: 1; color: win.accent2 }
                        }
                    }
                }
                handle: Rectangle {
                    x: timeline.leftPadding + timeline.visualPosition * (timeline.availableWidth - width)
                    y: timeline.topPadding + timeline.availableHeight/2 - height/2
                    width: 16; height: 16; radius: 8
                    color: "white"; border.color: win.accent2; border.width: 2
                    scale: timeline.pressed ? 1.25 : 1.0
                    Behavior on scale { NumberAnimation { duration: 90 } }
                }
            }
            // Keep the handle tracking the live head, but don't fight the user's drag.
            Connections {
                target: flux
                function onScrubFrameChanged() {
                    if (!timeline.pressed) timeline.value = flux.scrubFrame
                }
            }
        }

        // ---- right side: particle slider + clear ----
        Column {
            id: rightCol
            anchors { right: parent.right; rightMargin: 18; verticalCenter: parent.verticalCenter }
            width: 210; spacing: 8

            Row {
                width: parent.width
                Text { text: "Particles"; color: "#8A8B99"; font.pixelSize: 12
                       width: parent.width - pv.width }
                Text { id: pv; text: win.fmt(flux.particleCount); color: "white"
                       font.pixelSize: 12; font.bold: true
                       font.family: "Menlo, Consolas, monospace" }
            }
            Slider {
                id: pslider
                width: parent.width
                from: 200; to: 5000; stepSize: 100
                value: flux.particleCount
                onMoved: flux.particleCount = value
                background: Rectangle {
                    x: pslider.leftPadding; y: pslider.topPadding + pslider.availableHeight/2 - 2
                    width: pslider.availableWidth; height: 4; radius: 2; color: "#22FFFFFF"
                    Rectangle { width: pslider.visualPosition * parent.width; height: parent.height
                                radius: 2; color: win.accent }
                }
                handle: Rectangle {
                    x: pslider.leftPadding + pslider.visualPosition * (pslider.availableWidth - width)
                    y: pslider.topPadding + pslider.availableHeight/2 - height/2
                    width: 14; height: 14; radius: 7; color: "white"
                    border.color: win.accent; border.width: 2
                }
            }
            Text {
                text: "✕  clear history"
                color: "#8A8B99"; font.pixelSize: 12
                MouseArea { anchors.fill: parent; anchors.margins: -6
                            onClicked: flux.clear()
                            onPressed: parent.color = "#FF6C8A"
                            onReleased: parent.color = "#8A8B99" }
            }
        }
    }
}
