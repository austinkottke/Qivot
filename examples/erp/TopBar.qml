import QtQuick 2.15

// The window header: brand, the sliding segmented tabs, and a stat chip.
Rectangle {
    id: root
    property int currentTab: 0
    readonly property var tabs: [ "Overview", "Clients", "Projects", "Time & Expense", "Invoices" ]
    signal select(int index)

    height: 62; color: "white"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    Row {
        anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
        spacing: 10
        Rectangle { width: 30; height: 30; radius: 8; color: Theme.accent
                    anchors.verticalCenter: parent.verticalCenter
                    Text { anchors.centerIn: parent; text: "◈"; color: "white"; font.pixelSize: 16; font.bold: true } }
        Text { text: "Qivot ERP"; color: Theme.ink; font.pixelSize: 20; font.bold: true
               anchors.verticalCenter: parent.verticalCenter }
    }

    // segmented tabs with a gliding highlight
    Rectangle {
        anchors.centerIn: parent
        width: root.tabs.length * 132 + 8; height: 42; radius: 12; color: "#F1F3F8"
        Rectangle {
            width: 124; height: 34; radius: 9; y: 4
            x: 4 + root.currentTab * 132; color: Theme.accent
            Behavior on x { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
        }
        Row {
            anchors.fill: parent
            Repeater {
                model: root.tabs
                Item {
                    width: 132; height: 42
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
        spacing: 16
        Rectangle { width: statChip.width + 24; height: 30; radius: 15; color: "#EAF0FE"
                    anchors.verticalCenter: parent.verticalCenter
                    Text { id: statChip; anchors.centerIn: parent
                           text: store.clients.count + " clients · " + store.projects.count + " projects"
                           color: Theme.accent; font.pixelSize: 13; font.bold: true } }
    }
}
