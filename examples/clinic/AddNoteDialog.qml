import QtQuick 2.15
import QtQuick.Controls 2.15

// The Type combo's index maps directly to the Clinic::NoteKind enum value.
Popup {
    id: root
    anchors.centerIn: Overlay.overlay
    width: 480; padding: 0; modal: true
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
            Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                   text: "Add note — " + store.patient.firstName + " " + store.patient.lastName
                   color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Column {
            width: parent.width; padding: 20; spacing: 12
            Column { spacing: 5; width: parent.width - 40
                Text { text: "Type"; color: Theme.muted; font.pixelSize: 12 }
                ComboBox { id: anKind; width: parent.width
                           model: [ "Office Visit", "Phone", "Lab Review", "Follow-up" ] } }   // index == Clinic::NoteKind
            Column { spacing: 5; width: parent.width - 40
                Text { text: "Note"; color: Theme.muted; font.pixelSize: 12 }
                Rectangle { width: parent.width; height: 130; radius: 8; color: Theme.field
                    TextArea { id: anBody; anchors.fill: parent; anchors.margins: 8
                               wrapMode: TextArea.Wrap; font.pixelSize: 13; color: Theme.ink
                               placeholderText: "Subjective, assessment, plan…"; background: null } } }
            Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() } }
                Rectangle { width: 110; height: 40; radius: 10
                    color: anBody.text.trim().length ? Theme.teal : "#B7C2C4"
                    Text { anchors.centerIn: parent; text: "Save"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: {
                        if (!anBody.text.trim().length) return
                        store.addNote(store.selectedId, 0, anKind.currentIndex, anBody.text)
                        anBody.text = ""; root.close()
                    } } }
            }
        }
    }
}
