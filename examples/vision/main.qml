import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15

// The window shell: a gradient backdrop, a top bar, and four crossfading views.
// Data comes from the `store` context object, styling from `Theme`.
ApplicationWindow {
    id: win
    visible: true
    width: 1360; height: 880
    title: "Qivot · Vision — project earnings & utilization"

    // Route Qt Quick Controls (the dialog's ComboBox/TextField) through Material,
    // themed to match the rest of the app.
    Material.theme: Theme.dark ? Material.Dark : Material.Light
    Material.accent: Theme.accent
    Material.foreground: Theme.ink
    Material.background: Theme.card

    property int tab: 0     // 0 Overview, 1 Projects, 2 Visualization, 3 Utilization, 4 Ledger

    // gradient backdrop
    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.bg }
            GradientStop { position: 1.0; color: Theme.bg2 }
        }
    }

    TopBar {
        id: topbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        currentTab: win.tab
        onSelect: win.tab = index
    }

    OverviewView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab; myTab: 0
        onSwitchToProjects: win.tab = 1
    }
    ProjectsView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab; myTab: 1
    }
    TreemapView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab; myTab: 2
    }
    UtilizationView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab; myTab: 3
    }
    LedgerView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab; myTab: 4
    }
}
