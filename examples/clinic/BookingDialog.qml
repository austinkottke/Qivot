import QtQuick 2.15
import QtQuick.Controls 2.15

// Book a visit into the day currently shown on the schedule. store.book() runs a
// transaction and refuses to double-book — the error surfaces here.
Popup {
    id: root
    anchors.centerIn: Overlay.overlay
    width: 420; padding: 0; modal: true
    onOpened: bookErr.text = ""
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 240; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    ListModel { id: slotModel }
    Component.onCompleted: { for (var m = 480; m <= 1050; m += 15) slotModel.append({ label: store.minuteLabel(m), minute: m }) }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 60; color: "#FAFBFD"; radius: 16
            Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                   text: "New appointment — " + store.scheduleLabel; color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Column {
            width: parent.width; padding: 20; spacing: 14

            Column { spacing: 6; width: parent.width - 40
                Text { text: "Patient"; color: Theme.muted; font.pixelSize: 12 }
                ComboBox { id: cbPatient; width: parent.width; model: store.patients
                           textRole: "lastName"; valueRole: "id"
                           delegate: ItemDelegate { width: cbPatient.width
                               text: model.firstName + " " + model.lastName + "   ·   " + model.mrn
                               highlighted: cbPatient.highlightedIndex === index } } }
            Row { spacing: 14; width: parent.width - 40
                Column { spacing: 6; width: (parent.width - 14) / 2
                    Text { text: "Provider"; color: Theme.muted; font.pixelSize: 12 }
                    ComboBox { id: cbProvider; width: parent.width; model: store.providers
                               textRole: "name"; valueRole: "id" } }
                Column { spacing: 6; width: (parent.width - 14) / 2
                    Text { text: "Time"; color: Theme.muted; font.pixelSize: 12 }
                    ComboBox { id: cbTime; width: parent.width; model: slotModel
                               textRole: "label"; valueRole: "minute" } }
            }
            Row { spacing: 14; width: parent.width - 40
                Column { spacing: 6; width: (parent.width - 14) / 2
                    Text { text: "Duration"; color: Theme.muted; font.pixelSize: 12 }
                    ComboBox { id: cbDur; width: parent.width; model: [ 15, 20, 30, 45, 60 ]; currentIndex: 2 } }
                Column { spacing: 6; width: (parent.width - 14) / 2
                    Text { text: "Reason"; color: Theme.muted; font.pixelSize: 12 }
                    TextField { id: tfReason; width: parent.width; placeholderText: "Office Visit"
                                background: Rectangle { radius: 8; color: Theme.field } } }
            }

            Text { id: bookErr; color: "#D14343"; font.pixelSize: 12; width: parent.width - 40; wrapMode: Text.WordWrap }

            Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() } }
                Rectangle { width: 110; height: 40; radius: 10
                    color: bookMa.pressed ? Qt.darker(Theme.teal, 1.15) : Theme.teal
                    scale: bookMa.pressed ? 0.96 : 1.0
                    Behavior on scale { NumberAnimation { duration: 100 } }
                    Text { anchors.centerIn: parent; text: "Book"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { id: bookMa; anchors.fill: parent; onClicked: {
                        var ok = store.book(cbPatient.currentValue, cbProvider.currentValue,
                                            cbTime.currentValue, cbDur.currentText * 1, tfReason.text)
                        if (ok) root.close(); else bookErr.text = store.lastError
                    } } }
            }
        }
    }
}
