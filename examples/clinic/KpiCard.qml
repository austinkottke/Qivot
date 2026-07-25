import QtQuick 2.15

// A dashboard stat: an accent bar, an uppercase label, and a big value.
Rectangle {
    property string label: ""
    property var    value: ""
    property color  accent: "#3B82F6"

    height: 96; radius: 14; color: Theme.card
    Row {
        anchors { left: parent.left; leftMargin: 18; verticalCenter: parent.verticalCenter }
        spacing: 14
        Rectangle { width: 5; height: 42; radius: 3; color: accent
                    anchors.verticalCenter: parent.verticalCenter }
        Column {
            anchors.verticalCenter: parent.verticalCenter; spacing: 4
            Text { text: label.toUpperCase(); color: Theme.muted
                   font.pixelSize: 10; font.letterSpacing: 1; font.bold: true }
            Text { text: value; color: Theme.ink; font.pixelSize: 28; font.bold: true }
        }
    }
}
