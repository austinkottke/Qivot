import QtQuick 2.15

// A dashboard stat: an accent bar, an uppercase label, a big value, a sub-line.
Rectangle {
    property string label: ""
    property var    value: ""
    property string sub: ""
    property color  accent: Theme.accent

    height: 100; radius: 14; color: Theme.card
    border.width: 1; border.color: Theme.border

    Row {
        anchors { left: parent.left; leftMargin: 18; verticalCenter: parent.verticalCenter }
        spacing: 14
        Rectangle { width: 5; height: 48; radius: 3; color: accent
                    anchors.verticalCenter: parent.verticalCenter }
        Column {
            anchors.verticalCenter: parent.verticalCenter; spacing: 3
            Text { text: label.toUpperCase(); color: Theme.muted
                   font.pixelSize: 10; font.letterSpacing: 1; font.bold: true }
            Text { text: value; color: Theme.ink; font.pixelSize: 26; font.bold: true }
            Text { text: sub; color: Theme.muted; font.pixelSize: 11; visible: sub !== "" }
        }
    }
}
