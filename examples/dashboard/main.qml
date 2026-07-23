import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 440; height: 820
    color: "#0B0B10"
    title: "Sales Analytics — Qivot"

    DashboardStore { id: store }

    function money(v) { return "$" + Number(v).toLocaleString(Qt.locale(), "f", 0) }

    // ---- Header ----
    Column {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 20; rightMargin: 20; topMargin: 18 }
        spacing: 2
        Text { text: "Sales Analytics"; color: "white"; font.pixelSize: 30; font.bold: true }
        Text { text: "Window functions · async · cancellation"
               color: "#8A8A93"; font.pixelSize: 13 }
    }

    TabBar {
        id: tabs
        anchors { left: parent.left; right: parent.right; top: header.bottom; topMargin: 14 }
        background: Rectangle { color: "transparent" }
        Repeater {
            model: ["Leaderboard", "Recompute"]
            TabButton {
                contentItem: Text {
                    text: modelData; font.pixelSize: 15; font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    color: tabs.currentIndex === index ? "white" : "#8A8A93"
                }
                background: Rectangle { color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2
                                color: tabs.currentIndex === index ? "#0A84FF" : "transparent" } }
            }
        }
    }
    Rectangle { anchors.top: tabs.bottom; width: parent.width; height: 1; color: "#1D1D24" }

    StackLayout {
        anchors { top: tabs.bottom; left: parent.left; right: parent.right; bottom: parent.bottom
                  topMargin: 1 }
        currentIndex: tabs.currentIndex

        // ============================ LEADERBOARD ============================
        Item {
            Column {
                anchors.fill: parent
                // grand total banner
                Rectangle {
                    width: parent.width; height: 78; color: "transparent"
                    Column {
                        anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                        spacing: 2
                        Text { text: "TOTAL REVENUE"; color: "#8A8A93"; font.pixelSize: 11; font.bold: true }
                        Text { text: win.money(store.grandTotal); color: "#30D158"
                               font.pixelSize: 30; font.bold: true }
                    }
                    Text { text: store.leaderboard.length + " customers"
                           color: "#8A8A93"; font.pixelSize: 13
                           anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter } }
                }

                ListView {
                    id: board
                    width: parent.width; height: parent.height - 78
                    clip: true; model: store.leaderboard
                    spacing: 8; topMargin: 4; bottomMargin: 16
                    leftMargin: 14; rightMargin: 14

                    delegate: Rectangle {
                        width: board.width - 28; height: 72; radius: 16
                        color: "#16161D"
                        property bool top3: modelData.rank <= 3
                        property color accent: modelData.rank === 1 ? "#FFD60A"
                                             : modelData.rank === 2 ? "#AEB4BE"
                                             : modelData.rank === 3 ? "#FF9F0A" : "#0A84FF"

                        // rank badge
                        Rectangle {
                            id: badge
                            x: 12; anchors.verticalCenter: parent.verticalCenter
                            width: 40; height: 40; radius: 20
                            color: top3 ? accent : "#26262E"
                            Text { anchors.centerIn: parent
                                   text: modelData.rank <= 3
                                         ? ["🥇","🥈","🥉"][modelData.rank - 1] : modelData.rank
                                   color: top3 ? "#111" : "#B0B0B8"
                                   font.pixelSize: modelData.rank <= 3 ? 20 : 15; font.bold: true }
                        }

                        Column {
                            anchors { left: badge.right; leftMargin: 12; right: parent.right; rightMargin: 16
                                      verticalCenter: parent.verticalCenter }
                            spacing: 6
                            Row {
                                width: parent.width
                                Text { text: modelData.name; color: "white"; font.pixelSize: 16; font.bold: true
                                       width: parent.width - amt.width; elide: Text.ElideRight }
                                Text { id: amt; text: win.money(modelData.total)
                                       color: "white"; font.pixelSize: 16; font.bold: true }
                            }
                            // revenue bar (share of total)
                            Rectangle {
                                width: parent.width; height: 6; radius: 3; color: "#26262E"
                                Rectangle {
                                    height: parent.height; radius: 3
                                    width: Math.max(4, parent.width * modelData.pct / 100)
                                    gradient: Gradient {
                                        orientation: Gradient.Horizontal
                                        GradientStop { position: 0; color: Qt.darker(badge.color, 1.2) }
                                        GradientStop { position: 1; color: badge.color }
                                    }
                                    Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
                                }
                            }
                            Row {
                                width: parent.width
                                Text { text: modelData.orders + " orders · " + modelData.pct.toFixed(1) + "% of total"
                                       color: "#8A8A93"; font.pixelSize: 11
                                       width: parent.width - run.width }
                                Text { id: run; text: "running " + win.money(modelData.running)
                                       color: "#6D6D75"; font.pixelSize: 11 }
                            }
                        }
                    }
                }
            }
        }

        // ============================ RECOMPUTE ============================
        Item {
            Column {
                anchors.centerIn: parent
                width: parent.width - 60
                spacing: 22

                Text { text: "Heavy analytics job"; color: "white"; font.pixelSize: 22; font.bold: true
                       anchors.horizontalCenter: parent.horizontalCenter }
                Text {
                    width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                    text: "Runs on a background thread via QiAsync (its own pooled connection). "
                        + "The UI stays live — and you can cancel it mid-flight."
                    color: "#8A8A93"; font.pixelSize: 13
                }

                // big percent
                Text { text: store.progress + "%"; color: store.busy ? "#0A84FF" : "#48484E"
                       font.pixelSize: 64; font.bold: true
                       anchors.horizontalCenter: parent.horizontalCenter }

                // progress bar
                Rectangle {
                    width: parent.width; height: 12; radius: 6; color: "#1D1D24"
                    Rectangle {
                        height: parent.height; radius: 6
                        width: parent.width * store.progress / 100
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: "#0A84FF" }
                            GradientStop { position: 1; color: "#5E5CE6" }
                        }
                        Behavior on width { NumberAnimation { duration: 120 } }
                    }
                }

                Text { text: store.status; color: "#C8C8CE"; font.pixelSize: 14
                       anchors.horizontalCenter: parent.horizontalCenter }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter; spacing: 12
                    Button {
                        text: "Recompute"; enabled: !store.busy
                        onClicked: store.recompute()
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; font.bold: true
                                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { implicitWidth: 140; implicitHeight: 46; radius: 23
                                               color: parent.enabled ? "#0A84FF" : "#22222A" }
                    }
                    Button {
                        text: "Cancel"; enabled: store.busy
                        onClicked: store.cancel()
                        contentItem: Text { text: parent.text; color: parent.enabled ? "white" : "#5A5A62"
                                            font.pixelSize: 15; font.bold: true
                                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { implicitWidth: 120; implicitHeight: 46; radius: 23
                                               color: parent.enabled ? "#3A2030" : "#18181E"
                                               border.color: parent.enabled ? "#FF375F" : "transparent"; border.width: 1 }
                    }
                }

                // a spinner that only animates while busy — proof the UI thread is free
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter; spacing: 8
                    visible: store.busy
                    Repeater {
                        model: 3
                        Rectangle {
                            width: 10; height: 10; radius: 5; color: "#0A84FF"
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite; running: store.busy
                                PauseAnimation { duration: index * 160 }
                                NumberAnimation { to: 0.2; duration: 300 }
                                NumberAnimation { to: 1.0; duration: 300 }
                                PauseAnimation { duration: (2 - index) * 160 }
                            }
                        }
                    }
                }
            }
        }
    }
}
