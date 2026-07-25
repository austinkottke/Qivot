import QtQuick 2.15
import ClinicApp 1.0    // Clinic.Cancelled

// One time-positioned block on the day calendar. Geometry is passed in; the row
// data comes from the schedule model roles.
Rectangle {
    id: root
    property int   gutter: 60
    property real  colW: 100
    property real  pxPerMin: 1.15
    property int   dayStart: 480
    signal activate(var appt)

    visible: model.status !== Clinic.Cancelled
    x: gutter + store.providerIndex(model.providerId) * colW + 3
    y: (model.minute - dayStart) * pxPerMin + 8
    width: colW - 6
    height: Math.max(model.durationMin * pxPerMin - 3, 26)
    radius: 8
    color: Qt.rgba(0,0,0,0)
    border.width: hover.containsMouse ? 2 : 0
    border.color: Theme.statusColor(model.status)
    z: hover.containsMouse ? 5 : 1
    scale: hover.containsMouse ? 1.03 : 1.0
    Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
    opacity: 0
    Component.onCompleted: appear.start()
    SequentialAnimation {
        id: appear
        PauseAnimation { duration: Math.min(index * 14, 320) }
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: 260; easing.type: Easing.OutCubic }
    }

    Rectangle { anchors.fill: parent; radius: 8; color: Theme.statusColor(model.status)
                opacity: model.status === Clinic.Completed ? 0.28 : 0.16 }
    Rectangle { width: 3; height: parent.height - 10; radius: 2; color: Theme.statusColor(model.status)
                anchors { left: parent.left; leftMargin: 5; verticalCenter: parent.verticalCenter } }
    Column {
        anchors { left: parent.left; leftMargin: 14; right: parent.right; rightMargin: 6
                  top: parent.top; topMargin: 5 }
        spacing: 1
        Text { text: store.minuteLabel(model.minute) + "  " + store.patientName(model.patientId)
               color: Theme.ink; font.pixelSize: 12; font.bold: true; width: parent.width; elide: Text.ElideRight }
        Text { text: model.reason; color: "#4B5563"; font.pixelSize: 11
               width: parent.width; elide: Text.ElideRight; visible: root.height > 34 }
    }
    MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
        onClicked: root.activate({ id: model.id, name: store.patientName(model.patientId),
                                   time: store.minuteLabel(model.minute), reason: model.reason }) }
}
