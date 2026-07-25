import QtQuick 2.15

// The window header: brand, the sliding segmented tabs, and the date + count chip.
Rectangle {
    id: root
    property int currentTab: 0
    signal select(int index)

    height: 62; color: "white"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    Row {
        anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
        spacing: 10
        Rectangle { width: 30; height: 30; radius: 8; color: Theme.teal
                    anchors.verticalCenter: parent.verticalCenter
                    Text { anchors.centerIn: parent; text: "✚"; color: "white"; font.pixelSize: 18; font.bold: true } }
        Text { text: "Qivot Clinic"; color: Theme.ink; font.pixelSize: 20; font.bold: true
               anchors.verticalCenter: parent.verticalCenter }
    }

    // segmented tabs with a gliding highlight
    Rectangle {
        anchors.centerIn: parent
        width: 404; height: 42; radius: 12; color: "#F1F3F8"
        Rectangle {
            width: 124; height: 34; radius: 9; y: 4
            x: 4 + root.currentTab * 132; color: Theme.teal
            Behavior on x { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
        }
        Row {
            anchors.fill: parent
            Repeater {
                model: [ { t: "Overview", i: 0 }, { t: "Patients", i: 1 }, { t: "Schedule", i: 2 } ]
                Item {
                    width: 132; height: 42
                    Text { anchors.centerIn: parent; text: modelData.t
                           color: root.currentTab === modelData.i ? "white" : Theme.muted
                           font.pixelSize: 14; font.bold: true
                           Behavior on color { ColorAnimation { duration: 200 } } }
                    MouseArea { anchors.fill: parent; onClicked: root.select(modelData.i) }
                }
            }
        }
    }

    Row {
        anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
        spacing: 16
        Text { text: store.scheduleLabel; color: Theme.muted; font.pixelSize: 14
               anchors.verticalCenter: parent.verticalCenter }
        Rectangle { width: statChip.width + 24; height: 30; radius: 15; color: "#E8F5F5"
                    anchors.verticalCenter: parent.verticalCenter
                    Text { id: statChip; anchors.centerIn: parent
                           text: store.stats.total + " today · " + store.providers.count + " providers"
                           color: Theme.teal; font.pixelSize: 13; font.bold: true } }
    }
}
