import QtQuick 2.15
import QtQuick.Controls 2.15

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
                   text: "Add vitals — " + store.patient.firstName + " " + store.patient.lastName
                   color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Grid {
            x: 20; topPadding: 20; columns: 3; rowSpacing: 12; columnSpacing: 12
            property real fw: (root.width - 40 - 24) / 3
            Column { spacing: 5; width: parent.fw
                Text { text: "Systolic"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avSys; width: parent.width; placeholderText: "120"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "Diastolic"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avDia; width: parent.width; placeholderText: "80"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "Heart rate"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avHr; width: parent.width; placeholderText: "72"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "Temp °C"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avTemp; width: parent.width; placeholderText: "36.8"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "SpO₂ %"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avSpo2; width: parent.width; placeholderText: "98"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "Weight kg"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avWt; width: parent.width; placeholderText: "70"; background: Rectangle { radius: 8; color: Theme.field } } }
            Column { spacing: 5; width: parent.fw
                Text { text: "Height cm"; color: Theme.muted; font.pixelSize: 12 }
                TextField { id: avHt; width: parent.width; placeholderText: "170"; background: Rectangle { radius: 8; color: Theme.field } } }
        }
        Row { spacing: 12; anchors.right: parent.right; rightPadding: 20; topPadding: 16; bottomPadding: 20
            Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                MouseArea { anchors.fill: parent; onClicked: root.close() } }
            Rectangle { width: 110; height: 40; radius: 10; color: Theme.teal
                Text { anchors.centerIn: parent; text: "Save"; color: "white"; font.pixelSize: 14; font.bold: true }
                MouseArea { anchors.fill: parent; onClicked: {
                    store.addVital(store.selectedId,
                        parseInt(avSys.text) || 120, parseInt(avDia.text) || 80, parseInt(avHr.text) || 72,
                        parseFloat(avTemp.text) || 36.8, parseInt(avSpo2.text) || 98,
                        parseFloat(avWt.text) || 70, parseInt(avHt.text) || 170)
                    avSys.text = ""; avDia.text = ""; avHr.text = ""; avTemp.text = ""
                    avSpo2.text = ""; avWt.text = ""; avHt.text = ""
                    root.close()
                } } }
        }
    }
}
