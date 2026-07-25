import QtQuick 2.15
import QtQuick.Controls 2.15
import ClinicApp 1.0

// Status actions for a tapped appointment.
Popup {
    id: root
    property var appt: ({})
    function openFor(a) { appt = a; open() }

    anchors.centerIn: Overlay.overlay
    width: 300; padding: 0; modal: true
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.88; to: 1; duration: 240; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "scale"; from: 1; to: 0.92; duration: 130 }
                        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 74; color: "#FAFBFD"; radius: 16
            Column { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                spacing: 3
                Text { text: root.appt.name || ""; color: Theme.ink; font.pixelSize: 17; font.bold: true }
                Text { text: (root.appt.time || "") + " · " + (root.appt.reason || "")
                       color: Theme.muted; font.pixelSize: 12 } }
        }
        Column {
            width: parent.width; padding: 16; spacing: 8
            Repeater {
                model: [ { l: "Check in (arrived)", s: Clinic.Arrived, c: "#10B981" },
                         { l: "Mark completed", s: Clinic.Completed, c: "#0E8C93" },
                         { l: "Cancel appointment", s: Clinic.Cancelled, c: "#EF4444" } ]
                Rectangle {
                    width: parent.width - 32; height: 44; radius: 10
                    color: bhover.containsMouse ? Qt.lighter(modelData.c, 1.9) : "#F6F8FB"
                    Text { anchors.centerIn: parent; text: modelData.l; color: modelData.c
                           font.pixelSize: 14; font.bold: true }
                    MouseArea { id: bhover; anchors.fill: parent; hoverEnabled: true
                        onClicked: { store.setStatus(root.appt.id, modelData.s); root.close() } }
                }
            }
        }
    }
}
