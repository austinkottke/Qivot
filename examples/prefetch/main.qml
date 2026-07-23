import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 440; height: 820
    color: "#0B0B10"
    title: "Prefetch vs N+1 — Qivot"

    PrefetchStore { id: store }
    readonly property bool efficient: store.queryCount <= 2

    Column {
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 20; rightMargin: 20; topMargin: 18 }
        spacing: 3
        Text { text: "Eager Loading"; color: "white"; font.pixelSize: 30; font.bold: true }
        Text { text: "Load a one-to-many without the N+1 problem"
               color: "#8A8A93"; font.pixelSize: 13 }
    }

    // ---- query-count card ----
    Rectangle {
        id: card
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 16; rightMargin: 16; topMargin: 78 }
        height: 132; radius: 18
        color: win.efficient ? "#12241A" : "#241216"
        border.width: 1
        border.color: win.efficient ? "#30D158" : "#FF453A"
        Behavior on color { ColorAnimation { duration: 250 } }

        Column {
            anchors.centerIn: parent; spacing: 4
            Text { text: store.queryCount; color: win.efficient ? "#30D158" : "#FF453A"
                   font.pixelSize: 54; font.bold: true
                   anchors.horizontalCenter: parent.horizontalCenter }
            Text { text: "SQL queries"; color: "#8A8A93"; font.pixelSize: 12; font.bold: true
                   anchors.horizontalCenter: parent.horizontalCenter }
            Text { text: store.mode + "  ·  " + store.elapsedMs.toFixed(2) + " ms"
                   color: "#C8C8CE"; font.pixelSize: 13
                   anchors.horizontalCenter: parent.horizontalCenter }
        }
    }

    // ---- mode buttons ----
    Row {
        id: buttons
        anchors { horizontalCenter: parent.horizontalCenter; top: card.bottom; topMargin: 14 }
        spacing: 12
        Button {
            text: "Load N+1"; onClicked: store.loadNaive()
            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { implicitWidth: 150; implicitHeight: 44; radius: 22; color: "#3A2226"
                                   border.color: "#FF453A"; border.width: 1 }
        }
        Button {
            text: "Load prefetch"; onClicked: store.loadPrefetch()
            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { implicitWidth: 150; implicitHeight: 44; radius: 22; color: "#30D158" }
        }
    }

    // ---- authors + their books ----
    ListView {
        id: list
        anchors { left: parent.left; right: parent.right; top: buttons.bottom; bottom: parent.bottom
                  topMargin: 14; leftMargin: 14; rightMargin: 14 }
        clip: true; model: store.authors; spacing: 8; bottomMargin: 16

        delegate: Rectangle {
            width: list.width - 28; radius: 14; color: "#16161D"
            height: col.height + 24
            Column {
                id: col
                anchors { left: parent.left; right: parent.right; top: parent.top
                          leftMargin: 16; rightMargin: 16; topMargin: 12 }
                spacing: 6
                Row {
                    width: parent.width
                    Text { text: modelData.name; color: "white"; font.pixelSize: 16; font.bold: true
                           width: parent.width - badge.width }
                    Rectangle {
                        id: badge; height: 22; radius: 11; width: bt.width + 20; color: "#26262E"
                        Text { id: bt; anchors.centerIn: parent
                               text: modelData.count + (modelData.count === 1 ? " book" : " books")
                               color: "#0A84FF"; font.pixelSize: 11; font.bold: true }
                    }
                }
                Flow {
                    width: parent.width; spacing: 6
                    Repeater {
                        model: modelData.titles
                        Rectangle {
                            height: 20; radius: 10; width: tt.width + 16; color: "#20202A"
                            Text { id: tt; anchors.centerIn: parent; text: modelData
                                   color: "#B0B0B8"; font.pixelSize: 10 }
                        }
                    }
                }
            }
        }
    }
}
