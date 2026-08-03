import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int currentTab: 0
    property int myTab: 3

    visible: opacity > 0
    opacity: currentTab === myTab ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    PostLaborDialog { id: postDialog }

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        // header row with the action button
        Row {
            width: parent.width
            Column {
                width: parent.width - 160; spacing: 2
                Text { text: "Labor ledger (LD)"; color: Theme.ink; font.pixelSize: 20; font.bold: true }
                Text { text: "A live QiListModel over the LD table — post an entry and the reports update instantly."
                       color: Theme.muted; font.pixelSize: 12 }
            }
            Rectangle {
                width: 148; height: 40; radius: 10; color: Theme.accent
                anchors.verticalCenter: parent.verticalCenter
                Text { anchors.centerIn: parent; text: "+  Post labor"; color: "white"; font.pixelSize: 13; font.bold: true }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: postDialog.open() }
            }
        }

        Rectangle {
            width: parent.width; height: parent.height - 84; radius: 16
            color: Theme.card; border.width: 1; border.color: Theme.border

            // column header
            Row {
                id: hdr
                x: 20; y: 16; width: parent.width - 40; height: 18; spacing: 0
                Text { width: parent.width * 0.10; text: "ID"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.30; text: "PROJECT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.22; text: "EMPLOYEE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.16; text: "DATE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.10; horizontalAlignment: Text.AlignRight; text: "HOURS"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.12; horizontalAlignment: Text.AlignRight; text: "BILLED"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            }
            Rectangle { anchors { left: parent.left; right: parent.right; margins: 20 } y: 40; height: 1; color: Theme.border }

            ListView {
                anchors { left: parent.left; right: parent.right; top: parent.top; bottom: parent.bottom
                          topMargin: 46; leftMargin: 20; rightMargin: 20; bottomMargin: 14 }
                clip: true
                model: store.ledger        // live QiListModel — roles are LD's fields
                delegate: Row {
                    width: ListView.view.width; height: 40; spacing: 0
                    // roles from the model: id, WBS1, Employee, TransDate, RegHrs, BillRate, CostRate
                    Text { width: parent.width * 0.10; height: parent.height; verticalAlignment: Text.AlignVCenter
                           text: "#" + id; color: Theme.muted; font.pixelSize: 12 }
                    Row {
                        width: parent.width * 0.30; height: parent.height; spacing: 8
                        Rectangle { width: 8; height: 8; radius: 4; anchors.verticalCenter: parent.verticalCenter; color: Theme.keyColor(WBS1) }
                        Column { anchors.verticalCenter: parent.verticalCenter; spacing: 0
                            Text { text: store.projectName(WBS1); color: Theme.ink; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; width: parent.parent.parent.width * 0.30 - 40 }
                            Text { text: WBS1; color: Theme.muted; font.pixelSize: 10 } }
                    }
                    Text { width: parent.width * 0.22; height: parent.height; verticalAlignment: Text.AlignVCenter
                           text: store.employeeName(Employee); color: Theme.ink; font.pixelSize: 12; elide: Text.ElideRight }
                    Text { width: parent.width * 0.16; height: parent.height; verticalAlignment: Text.AlignVCenter
                           text: Theme.dateLabel(TransDate); color: Theme.muted; font.pixelSize: 12 }
                    Text { width: parent.width * 0.10; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                           text: (+RegHrs).toFixed(1); color: Theme.ink; font.pixelSize: 12 }
                    Text { width: parent.width * 0.12; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                           text: BillRate > 0 ? store.money(RegHrs * BillRate) : "—"
                           color: BillRate > 0 ? Theme.ink : Theme.muted; font.pixelSize: 12 }
                }
            }
        }
    }
}
