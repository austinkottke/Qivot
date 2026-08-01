import QtQuick 2.15
import QtQuick.Controls 2.15

// Turns a project's unbilled billable time + expenses into an invoice
// (store.generateInvoice() runs this as a transaction).
Popup {
    id: root
    property int projectId: 0
    signal generated()
    anchors.centerIn: Overlay.overlay
    width: 400; padding: 0; modal: true
    onOpened: genErr.text = ""
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
            Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                   text: "Generate invoice"; color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Column {
            width: parent.width; padding: 20; spacing: 12
            Text { width: parent.width - 40; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 13
                   text: "Bills every unbilled, billable time entry and expense on "
                         + store.projectName(root.projectId) + " into a new draft invoice." }
            Column { spacing: 5; width: parent.width - 40
                Text { text: "Due date"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: fDue; width: parent.width
                            background: Rectangle { radius: 8; color: Theme.field } } }
            Text { id: genErr; color: "#D14343"; font.pixelSize: 12; width: parent.width - 40; wrapMode: Text.WordWrap }
            Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() } }
                Rectangle { width: 140; height: 40; radius: 10; color: Theme.accent
                    Text { anchors.centerIn: parent; text: "Generate invoice"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: {
                        var ok = store.generateInvoice(root.projectId, fDue.text)
                        if (ok) { root.close(); root.generated() } else { genErr.text = store.lastError }
                    } } }
            }
        }
    }
}
