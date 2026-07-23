import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 440; height: 820
    color: "#0B0B10"
    title: "Nested transactions — Qivot"

    SandboxStore { id: store }

    Column {
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 20; rightMargin: 20; topMargin: 18 }
        spacing: 3
        Text { text: "Savepoint Sandbox"; color: "white"; font.pixelSize: 30; font.bold: true }
        Text { text: "Nested transactions — roll a scope back to undo just its edits"
               color: "#8A8A93"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap }
    }

    // ---- scope depth stack ----
    Rectangle {
        id: depthCard
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 16; rightMargin: 16; topMargin: 82 }
        height: 66; radius: 16
        color: store.depth > 0 ? "#161622" : "#141419"
        border.width: 1; border.color: store.depth > 0 ? "#5E5CE6" : "#22222A"
        Behavior on color { ColorAnimation { duration: 200 } }

        Row {
            anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
            spacing: 8
            Text { text: store.depth === 0 ? "No open scope" : "Open scopes:"
                   color: "#C8C8CE"; font.pixelSize: 14; anchors.verticalCenter: parent.verticalCenter }
            Repeater {
                model: store.depth
                Rectangle {
                    width: 34; height: 26; radius: 7
                    color: index === store.depth - 1 ? "#5E5CE6" : "#2A2A38"
                    Text { anchors.centerIn: parent
                           text: index === 0 ? "TX" : "SP" + (index + 1)
                           color: "white"; font.pixelSize: 10; font.bold: true }
                }
            }
        }
    }

    // ---- scope controls ----
    Row {
        id: scopeButtons
        anchors { horizontalCenter: parent.horizontalCenter; top: depthCard.bottom; topMargin: 12 }
        spacing: 8
        function mkBtn(label, col, en, cb) { return {} }
        Button {
            text: "Begin scope"; onClicked: store.beginScope()
            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { implicitWidth: 116; implicitHeight: 42; radius: 21; color: "#5E5CE6" }
        }
        Button {
            text: "Roll back"; enabled: store.depth > 0; onClicked: store.rollbackScope()
            contentItem: Text { text: parent.text; color: parent.enabled ? "white" : "#5A5A62"
                                font.pixelSize: 13; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { implicitWidth: 106; implicitHeight: 42; radius: 21
                                   color: parent.enabled ? "#3A2030" : "#18181E"
                                   border.color: parent.enabled ? "#FF375F" : "transparent"; border.width: 1 }
        }
        Button {
            text: "Commit"; enabled: store.depth > 0; onClicked: store.commitScope()
            contentItem: Text { text: parent.text; color: parent.enabled ? "white" : "#5A5A62"
                                font.pixelSize: 13; font.bold: true
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            background: Rectangle { implicitWidth: 96; implicitHeight: 42; radius: 21
                                   color: parent.enabled ? "#12241A" : "#18181E"
                                   border.color: parent.enabled ? "#30D158" : "transparent"; border.width: 1 }
        }
    }

    // ---- add row ----
    Rectangle {
        id: addRow
        anchors { left: parent.left; right: parent.right; top: scopeButtons.bottom
                  leftMargin: 16; rightMargin: 16; topMargin: 14 }
        height: 44; radius: 10; color: "#16161D"
        TextField {
            id: input
            anchors { left: parent.left; right: addBtn.left; verticalCenter: parent.verticalCenter
                      leftMargin: 12; rightMargin: 8 }
            placeholderText: "Add an item…"; color: "white"; font.pixelSize: 14
            background: null
            onAccepted: { store.addItem(text); text = "" }
        }
        Rectangle {
            id: addBtn; anchors { right: parent.right; rightMargin: 6; verticalCenter: parent.verticalCenter }
            width: 64; height: 32; radius: 8; color: "#0A84FF"
            Text { anchors.centerIn: parent; text: "Add"; color: "white"; font.pixelSize: 13; font.bold: true }
            MouseArea { anchors.fill: parent; onClicked: { store.addItem(input.text); input.text = "" } }
        }
    }

    // ---- items ----
    ListView {
        id: list
        anchors { left: parent.left; right: parent.right; top: addRow.bottom; bottom: parent.bottom
                  topMargin: 12; leftMargin: 16; rightMargin: 16 }
        clip: true; model: store.items; spacing: 6; bottomMargin: 16

        delegate: Rectangle {
            width: list.width - 32; height: 48; radius: 10; color: "#16161D"
            Text { text: label; color: "white"; font.pixelSize: 15
                   anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter } }
            Rectangle {
                anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                width: 28; height: 28; radius: 14; color: rmMouse.pressed ? "#3A2030" : "transparent"
                Text { anchors.centerIn: parent; text: "✕"; color: "#8A8A93"; font.pixelSize: 14 }
                MouseArea { id: rmMouse; anchors.fill: parent; onClicked: store.removeItem(id) }
            }
        }
    }
}
