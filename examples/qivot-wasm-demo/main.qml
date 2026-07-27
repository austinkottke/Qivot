import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 900; height: 680
    color: "#0E1220"
    title: "Qivot Notes — Qt for WebAssembly"

    readonly property color ink: "#1F2733"

    NoteStore { id: store }

    function addFromInput() {
        if (input.text.trim().length === 0) return
        store.add(input.text)
        input.text = ""
        input.forceActiveFocus()
    }

    // ---- header ----
    Column {
        id: header
        x: 32; y: 28; width: parent.width - 64; spacing: 4
        Row {
            spacing: 10
            Rectangle { width: 30; height: 30; radius: 8; color: "#7C5CFF"
                        anchors.verticalCenter: parent.verticalCenter
                        Text { anchors.centerIn: parent; text: "✎"; color: "white"; font.pixelSize: 17; font.bold: true } }
            Text { text: "Qivot Notes"; color: "white"; font.pixelSize: 28; font.bold: true
                   anchors.verticalCenter: parent.verticalCenter }
        }
        Text { text: "A Qt app compiled to WebAssembly · notes live in an in-browser SQLite database"
               color: "#8A8FA3"; font.pixelSize: 14 }
    }

    // ---- add bar ----
    Rectangle {
        id: addBar
        anchors { left: parent.left; right: parent.right; top: header.bottom; margins: 32; topMargin: 22 }
        height: 52; radius: 14; color: "#171B2C"; border.color: input.activeFocus ? "#7C5CFF" : "#262B40"
        Behavior on border.color { ColorAnimation { duration: 150 } }

        TextField {
            id: input
            anchors { left: parent.left; leftMargin: 16; right: addBtn.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
            placeholderText: "Write a note and press Enter…"
            placeholderTextColor: "#5A6078"
            color: "white"; font.pixelSize: 15; background: null
            onAccepted: win.addFromInput()
            Component.onCompleted: forceActiveFocus()
        }
        Rectangle {
            id: addBtn
            anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
            width: 84; height: 38; radius: 10
            color: addMa.pressed ? "#5B43D6" : (addMa.containsMouse ? "#8B6BFF" : "#7C5CFF")
            Behavior on color { ColorAnimation { duration: 120 } }
            Text { anchors.centerIn: parent; text: "Add"; color: "white"; font.pixelSize: 15; font.bold: true }
            MouseArea { id: addMa; anchors.fill: parent; hoverEnabled: true; onClicked: win.addFromInput() }
        }
    }

    // ---- count + clear ----
    Item {
        id: metaRow
        anchors { left: parent.left; right: parent.right; top: addBar.bottom; margins: 32; topMargin: 16 }
        height: 20
        Text { anchors.left: parent.left; text: store.notes.count + (store.notes.count === 1 ? " note" : " notes")
               color: "#8A8FA3"; font.pixelSize: 13 }
        Text { anchors.right: parent.right; text: "Clear all"; color: "#8A8FA3"; font.pixelSize: 13
               visible: store.notes.count > 0
               MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: store.clear()
                           onPressed: parent.color = "#FF7A8A"; onReleased: parent.color = "#8A8FA3" } }
    }

    // ---- notes ----
    Flickable {
        anchors { left: parent.left; right: parent.right; top: metaRow.bottom; bottom: parent.bottom
                  leftMargin: 32; rightMargin: 32; topMargin: 14; bottomMargin: 20 }
        contentHeight: flow.height; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Flow {
            id: flow
            width: parent.width; spacing: 16

            Repeater {
                model: store.notes
                Rectangle {
                    id: card
                    width: 224; height: Math.max(120, bodyText.implicitHeight + 62)
                    radius: 14; color: model.color

                    // entrance (target the card itself — not its parent Flow)
                    opacity: 0; scale: 0.94
                    Component.onCompleted: appear.start()
                    ParallelAnimation { id: appear
                        NumberAnimation { target: card; property: "opacity"; to: 1; duration: 220 }
                        NumberAnimation { target: card; property: "scale"; to: 1; duration: 260; easing.type: Easing.OutBack } }

                    Text {
                        id: bodyText
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                        text: model.text; color: win.ink; font.pixelSize: 15; wrapMode: Text.WordWrap
                    }
                    Text {
                        anchors { left: parent.left; bottom: parent.bottom; margins: 16 }
                        text: model.createdAt; color: "#00000066"; font.pixelSize: 11
                    }
                    Text {
                        anchors { right: parent.right; bottom: parent.bottom; margins: 12 }
                        text: "✕"; color: del.containsMouse ? "#00000099" : "#00000044"; font.pixelSize: 15
                        MouseArea { id: del; anchors.fill: parent; anchors.margins: -6; hoverEnabled: true
                                    onClicked: store.remove(model.id) }
                    }
                }
            }
        }

        // empty state
        Column {
            anchors.horizontalCenter: parent.horizontalCenter; y: 60; spacing: 8
            visible: store.notes.count === 0
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "✎"; color: "#2C3350"; font.pixelSize: 52 }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "No notes yet — add one above"; color: "#5A6078"; font.pixelSize: 15 }
        }
    }
}
