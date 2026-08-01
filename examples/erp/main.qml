import QtQuick 2.15
import QtQuick.Controls 2.15

// The window shell: a top bar and five crossfading views. Everything else lives
// in its own component; data comes from the `store` context object, styling from
// `Theme`. See TopBar.qml, OverviewView.qml, ClientsView.qml, ProjectsView.qml,
// TimeExpenseView.qml, InvoicesView.qml.
ApplicationWindow {
    id: win
    visible: true
    width: 1400; height: 900
    color: Theme.bg
    title: "Qivot ERP — CRM, projects, time & billing"

    property int tab: 0     // 0 Overview, 1 Clients, 2 Projects, 3 Time & Expense, 4 Invoices

    TopBar {
        id: topbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        currentTab: win.tab
        onSelect: win.tab = index
    }

    OverviewView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
        onSwitchToProjects: win.tab = 2
    }
    ClientsView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
    }
    ProjectsView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
        onSwitchToInvoices: win.tab = 4
    }
    TimeExpenseView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
    }
    InvoicesView {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        currentTab: win.tab
    }
}
