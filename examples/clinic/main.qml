import QtQuick 2.15
import QtQuick.Controls 2.15

// The window shell: a top bar and three crossfading views. Everything else lives
// in its own component; data comes from the `store` context object, styling from
// `Theme`. See TopBar.qml, OverviewView.qml, PatientsView.qml, ScheduleView.qml.
ApplicationWindow {
    id: win
    visible: true
    width: 1340; height: 880
    color: Theme.bg
    title: "Qivot Clinic — scheduler & patient chart"

    property int tab: 0     // 0 = Overview, 1 = Patients, 2 = Schedule

    TopBar {
        id: topbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        currentTab: win.tab
        onSelect: win.tab = index
    }

    OverviewView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
        onSwitchToPatients: win.tab = 1
    }
    PatientsView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
    }
    ScheduleView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
    }
}
