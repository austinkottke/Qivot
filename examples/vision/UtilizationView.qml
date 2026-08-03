import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int currentTab: 0
    property int myTab: 2

    visible: opacity > 0
    opacity: currentTab === myTab ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    Flickable {
        anchors.fill: parent
        contentHeight: card.height + 48
        clip: true

        SectionCard {
            id: card
            x: 24; y: 24; width: parent.width - 48
            title: "Employee utilization — billable share of logged hours"

            Text { text: "Utilization = billable hours ÷ total hours logged. Inactive staff are excluded (Vision Status = 'A')."
                   width: parent.width; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap }

            Repeater {
                model: store.staff
                Rectangle {
                    id: card
                    width: parent.width; height: 64; radius: 12; color: Theme.field
                    Row {
                        anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                        spacing: 14
                        Rectangle { width: 40; height: 40; radius: 20; color: Theme.keyColor(modelData.employee)
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: Theme.initials(modelData.name)
                                           color: "white"; font.pixelSize: 15; font.bold: true } }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter; spacing: 2
                            // on phones the hours+gauge block is hidden, so the name gets the room
                            width: Theme.compact ? (card.width - 152) : 200
                            Text { text: modelData.name; color: Theme.ink; font.pixelSize: 14; font.bold: true }
                            Text { text: modelData.title + " · " + modelData.employee; color: Theme.muted; font.pixelSize: 11 }
                        }
                    }
                    // hours + gauge on the right
                    Row {
                        anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                        spacing: 18
                        Column {
                            visible: !Theme.compact          // hidden on phones — the % chip carries it
                            anchors.verticalCenter: parent.verticalCenter; spacing: 1
                            Text { horizontalAlignment: Text.AlignRight; width: 160
                                   text: modelData.billableHrs.toFixed(0) + " billable · " + modelData.totalHrs.toFixed(0) + " total"
                                   color: Theme.muted; font.pixelSize: 11 }
                            Rectangle {
                                width: 160; height: 8; radius: 4; color: Theme.track
                                Rectangle { width: parent.width * Math.min(1, modelData.util / 100); height: parent.height; radius: 4
                                            color: Theme.ratioColor(modelData.util) }
                            }
                        }
                        Rectangle {
                            width: 62; height: 40; radius: 10; anchors.verticalCenter: parent.verticalCenter
                            color: Theme.chip
                            Text { anchors.centerIn: parent; text: modelData.util.toFixed(0) + "%"
                                   color: Theme.ratioColor(modelData.util); font.pixelSize: 16; font.bold: true }
                        }
                    }
                }
            }
        }
    }
}
