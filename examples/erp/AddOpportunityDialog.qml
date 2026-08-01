import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root
    property int clientId: 0
    anchors.centerIn: Overlay.overlay
    width: 400; padding: 0; modal: true
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
            Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                   text: "New opportunity"; color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Column {
            width: parent.width; padding: 20; spacing: 12
            Column { spacing: 5; width: parent.width - 40
                Text { text: "Opportunity name"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: fName; width: parent.width; placeholderText: "Campus Expansion — Phase 2"
                            background: Rectangle { radius: 8; color: Theme.field } } }
            Row { spacing: 12; width: parent.width - 40
                Column { spacing: 5; width: (parent.width - 12) / 2
                    Text { text: "Stage"; color: Theme.muted; font.pixelSize: 12 }
                    ComboBox { id: fStage; width: parent.width; model: [ "Lead", "Qualified", "Proposal", "Won", "Lost" ] } }
                Column { spacing: 5; width: (parent.width - 12) / 2
                    Text { text: "Est. value ($)"; color: Theme.muted; font.pixelSize: 12 }
                    TextField { id: fAmount; width: parent.width; placeholderText: "250000"
                                validator: DoubleValidator { bottom: 0 }
                                background: Rectangle { radius: 8; color: Theme.field } } } }
            Column { spacing: 5; width: parent.width - 40
                Text { text: "Expected close date"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: fClose; width: parent.width; placeholderText: "2026-09-30"
                            background: Rectangle { radius: 8; color: Theme.field } } }
            Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() } }
                Rectangle { width: 130; height: 40; radius: 10; color: Theme.accent
                    Text { anchors.centerIn: parent; text: "Add opportunity"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: {
                        store.addOpportunity(root.clientId, fName.text, fStage.currentIndex,
                                              parseFloat(fAmount.text) || 0, fClose.text)
                        fName.text = ""; fStage.currentIndex = 0; fAmount.text = ""; fClose.text = ""
                        root.close()
                    } } }
            }
        }
    }
}
