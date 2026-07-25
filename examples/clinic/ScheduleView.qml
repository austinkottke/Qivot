import QtQuick 2.15
import QtQuick.Controls 2.15

// A day calendar: providers as columns, appointments positioned by time.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 2

    opacity: currentTab === myIndex ? 1 : 0
    visible: opacity > 0.01
    enabled: currentTab === myIndex
    transform: Translate { x: currentTab === myIndex ? 0 : (currentTab < myIndex ? 28 : -28)
                           Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
    Behavior on opacity { NumberAnimation { duration: 240 } }

    // toolbar
    Item {
        id: schedBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 64
        Row {
            anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 8
            Rectangle { width: 34; height: 34; radius: 8; color: "white"; border.color: Theme.border
                        Text { anchors.centerIn: parent; text: "‹"; color: Theme.ink; font.pixelSize: 20 }
                        MouseArea { anchors.fill: parent; onClicked: store.shiftDay(-1) } }
            Rectangle { width: 74; height: 34; radius: 8; color: store.isToday ? Theme.teal : "white"; border.color: Theme.border
                        Text { anchors.centerIn: parent; text: "Today"; color: store.isToday ? "white" : Theme.ink; font.pixelSize: 13; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: store.goToday() } }
            Rectangle { width: 34; height: 34; radius: 8; color: "white"; border.color: Theme.border
                        Text { anchors.centerIn: parent; text: "›"; color: Theme.ink; font.pixelSize: 20 }
                        MouseArea { anchors.fill: parent; onClicked: store.shiftDay(1) } }
            Text { text: store.scheduleLabel; color: Theme.ink; font.pixelSize: 18; font.bold: true
                   anchors.verticalCenter: parent.verticalCenter; leftPadding: 8 }
        }
        Row {
            anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 18
            Row { spacing: 14; anchors.verticalCenter: parent.verticalCenter
                Repeater {
                    model: [ { l: "booked", v: store.stats.total, c: "#3B82F6" },
                             { l: "arrived", v: store.stats.arrived, c: "#10B981" },
                             { l: "done", v: store.stats.completed, c: "#94A3B8" } ]
                    Row { spacing: 6
                        Rectangle { width: 9; height: 9; radius: 4.5; color: modelData.c; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: modelData.v + " " + modelData.l; color: Theme.muted; font.pixelSize: 13; anchors.verticalCenter: parent.verticalCenter } }
                }
            }
            Rectangle { width: bkT.width + 34; height: 38; radius: 10
                        color: naMa.pressed ? Qt.darker(Theme.teal, 1.15) : (naMa.containsMouse ? Qt.lighter(Theme.teal, 1.08) : Theme.teal)
                        anchors.verticalCenter: parent.verticalCenter
                        scale: naMa.pressed ? 0.96 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100 } }
                        Behavior on color { ColorAnimation { duration: 140 } }
                        Text { id: bkT; anchors.centerIn: parent; text: "＋  New appointment"; color: "white"; font.pixelSize: 14; font.bold: true }
                        MouseArea { id: naMa; anchors.fill: parent; hoverEnabled: true; onClicked: bookingDialog.open() } }
        }
    }

    // calendar
    Rectangle {
        id: cal
        anchors { left: parent.left; right: parent.right; top: schedBar.bottom; bottom: parent.bottom; margins: 20; topMargin: 0 }
        radius: 16; color: "white"; clip: true

        property int dayStart: 480
        property int dayEnd: 1080
        property real pxPerMin: 1.15
        property int gutter: 60
        property real colW: (width - gutter) / Math.max(1, store.providers.count)

        Rectangle {
            id: calHead
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 52; color: "#FAFBFD"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#E9ECF2" }
            Repeater {
                model: store.providers
                Column {
                    x: cal.gutter + index * cal.colW; width: cal.colW; height: parent.height
                    Item { width: 1; height: 8 }
                    Text { text: model.name; color: Theme.ink; font.pixelSize: 13; font.bold: true
                           width: parent.width - 12; anchors.horizontalCenter: parent.horizontalCenter
                           horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                    Text { text: model.specialty; color: Theme.muted; font.pixelSize: 11
                           width: parent.width; horizontalAlignment: Text.AlignHCenter }
                    Rectangle { width: 28; height: 3; radius: 2; color: model.color; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }
        }

        Flickable {
            anchors { left: parent.left; right: parent.right; top: calHead.bottom; bottom: parent.bottom }
            contentHeight: (cal.dayEnd - cal.dayStart) * cal.pxPerMin + 20; clip: true
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Item {
                width: parent.width; height: (cal.dayEnd - cal.dayStart) * cal.pxPerMin + 20

                Repeater {
                    model: (cal.dayEnd - cal.dayStart) / 60 + 1
                    Item {
                        y: index * 60 * cal.pxPerMin + 8; width: cal.width
                        Text { x: 12; y: -7; text: store.minuteLabel((cal.dayStart + index * 60)); color: Theme.muted; font.pixelSize: 11 }
                        Rectangle { x: cal.gutter; width: cal.width - cal.gutter; height: 1; color: "#EEF1F6" }
                    }
                }
                Repeater {
                    model: store.providers.count
                    Rectangle { x: cal.gutter + index * cal.colW; y: 0; width: 1; height: parent.height; color: "#F2F4F8" }
                }
                Repeater {
                    model: store.schedule
                    AppointmentBlock {
                        gutter: cal.gutter; colW: cal.colW; pxPerMin: cal.pxPerMin; dayStart: cal.dayStart
                        onActivate: apptActionPopup.openFor(appt)
                    }
                }
            }
        }
    }

    // dialogs owned by this view
    BookingDialog   { id: bookingDialog }
    ApptActionPopup { id: apptActionPopup }
}
