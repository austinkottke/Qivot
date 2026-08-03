import QtQuick 2.15

// The window header: brand, the sliding segmented tabs, a database chip, and a
// light/dark toggle.
Rectangle {
    id: root
    property int currentTab: 0
    readonly property var tabs: [ "Overview", "Projects", "Visualization", "Utilization", "Ledger" ]
    signal select(int index)

    height: 62; color: Theme.card
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    Row {
        anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
        spacing: 10
        Rectangle {
            width: 30; height: 30; radius: 8; anchors.verticalCenter: parent.verticalCenter
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.accent }
                GradientStop { position: 1.0; color: Theme.accent2 }
            }
            Text { anchors.centerIn: parent; text: "◆"; color: "white"; font.pixelSize: 15; font.bold: true }
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter; spacing: 0
            Text { text: "Qivot · Vision"; color: Theme.ink; font.pixelSize: 19; font.bold: true }
            Text { text: "A/E/C project accounting"; color: Theme.muted; font.pixelSize: 10 }
        }
    }

    // segmented tabs with a gliding gradient highlight
    Rectangle {
        anchors.centerIn: parent
        width: root.tabs.length * 116 + 8; height: 42; radius: 12; color: Theme.field
        Rectangle {
            width: 108; height: 34; radius: 9; y: 4
            x: 4 + root.currentTab * 116
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accent }
                GradientStop { position: 1.0; color: Theme.accent2 }
            }
            Behavior on x { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
        }
        Row {
            anchors.fill: parent
            Repeater {
                model: root.tabs
                Item {
                    width: 116; height: 42
                    Text { anchors.centerIn: parent; text: modelData
                           color: root.currentTab === index ? "white" : Theme.muted
                           font.pixelSize: 13; font.bold: true
                           Behavior on color { ColorAnimation { duration: 200 } } }
                    MouseArea { anchors.fill: parent; onClicked: root.select(index) }
                }
            }
        }
    }

    Row {
        anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
        spacing: 12
        Rectangle { width: chip.width + 26; height: 30; radius: 15; color: Theme.chip
                    anchors.verticalCenter: parent.verticalCenter
                    Row { id: chip; anchors.centerIn: parent; spacing: 6
                        Text { text: "◱"; color: Theme.accent; font.pixelSize: 13; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter }
                        Text { text: "Synthetic demo · SQLite"; color: Theme.accent
                               font.pixelSize: 12; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter } } }
        // dark/light toggle
        Rectangle {
            width: 34; height: 30; radius: 15; color: Theme.chip
            anchors.verticalCenter: parent.verticalCenter
            Text { anchors.centerIn: parent; text: Theme.dark ? "☾" : "☀"
                   color: Theme.dark ? Theme.warn : Theme.accent; font.pixelSize: 15 }
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Theme.toggle() }
        }
    }
}
