import QtQuick 2.15

// A dashboard stat: an accent bar, an uppercase label, a big value, a sub-line.
Rectangle {
    id: card
    property string label: ""
    property var    value: ""
    property string sub: ""
    property color  accent: Theme.accent

    height: 100; radius: 14; color: Theme.card
    border.width: 1; border.color: Theme.border

    Row {
        anchors { left: parent.left; leftMargin: 16; right: parent.right; rightMargin: 12
                  verticalCenter: parent.verticalCenter }
        spacing: 12
        Rectangle { width: 5; height: 48; radius: 3; color: accent
                    anchors.verticalCenter: parent.verticalCenter }
        Column {
            anchors.verticalCenter: parent.verticalCenter; spacing: 3
            width: card.width - 16 - 5 - 12 - 12        // card minus paddings + accent bar
            // label + sub elide; the value shrinks on phones so the number is never cut off
            Text { width: parent.width; text: label.toUpperCase(); color: Theme.muted; elide: Text.ElideRight
                   font.pixelSize: 10; font.letterSpacing: Theme.compact ? 0 : 1; font.bold: true }
            Text { width: parent.width; text: value; color: Theme.ink; font.bold: true
                   font.pixelSize: Theme.compact ? 21 : 26 }
            Text { width: parent.width; text: sub; color: Theme.muted; font.pixelSize: 11; visible: sub !== ""; elide: Text.ElideRight }
        }
    }
}
