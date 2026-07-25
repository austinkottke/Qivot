import QtQuick 2.15

// One vital sign: a small accent bar, a caption, and value + unit.
Rectangle {
    property string label: ""
    property var    value: "—"
    property string unit: ""
    property color  accent: "#3B82F6"

    height: 92; radius: 14; color: Theme.card
    Rectangle { width: 4; height: 30; radius: 2; color: accent
                anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 16 } }
    Column {
        anchors { left: parent.left; leftMargin: 30; top: parent.top; topMargin: 14 }
        spacing: 3
        Text { text: label; color: Theme.muted; font.pixelSize: 11 }
        Row { spacing: 4
            Text { text: value; color: Theme.ink; font.pixelSize: 24; font.bold: true }
            Text { text: unit; color: "#9AA1AD"; font.pixelSize: 11
                   anchors.bottom: parent.bottom; anchors.bottomMargin: 4 } }
    }
}
