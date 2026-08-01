import QtQuick 2.15
import QtQuick.Controls 2.15

// Left: searchable project directory. Right: the selected project — budget vs.
// spend, the team (derived from who logged time), recent time/expense entries,
// and its invoices (with a "Generate Invoice" action over unbilled work).
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 2
    property var ps: store.projectSummary
    signal switchToInvoices()

    opacity: currentTab === myIndex ? 1 : 0
    visible: opacity > 0.01
    enabled: currentTab === myIndex
    transform: Translate { x: currentTab === myIndex ? 0 : (currentTab < myIndex ? 28 : -28)
                           Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
    Behavior on opacity { NumberAnimation { duration: 240 } }

    // ---- directory ----
    Rectangle {
        id: directory
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
        width: 340; color: "white"
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }

        Rectangle {
            id: searchBox
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
            height: 42; radius: 10; color: Theme.field
            border.color: projectSearch.activeFocus ? Theme.accent : "transparent"
            Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                   text: "⌕"; color: Theme.muted; font.pixelSize: 20 }
            TextField { id: projectSearch
                anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                placeholderText: "Search name or code…"; background: null; color: Theme.ink; font.pixelSize: 14
                onTextChanged: searchDebounce.restart() }
            Timer { id: searchDebounce; interval: 90; onTriggered: store.searchProjects(projectSearch.text) }
        }
        Text { anchors { left: parent.left; leftMargin: 18; top: searchBox.bottom; topMargin: 8 }
               text: store.projects.count + " projects"; color: Theme.muted; font.pixelSize: 12 }
        Rectangle {
            anchors { right: parent.right; rightMargin: 16; top: searchBox.bottom; topMargin: 2 }
            width: npT.width + 22; height: 26; radius: 13
            color: npMa.pressed ? Qt.darker(Theme.accent, 1.1) : (npMa.containsMouse ? Qt.lighter(Theme.accent, 1.08) : Theme.accent)
            Behavior on color { ColorAnimation { duration: 130 } }
            Text { id: npT; anchors.centerIn: parent; text: "＋ New"; color: "white"; font.pixelSize: 11; font.bold: true }
            MouseArea { id: npMa; anchors.fill: parent; hoverEnabled: true; onClicked: addProjectDialog.open() }
        }

        ListView {
            id: projectList
            anchors { left: parent.left; right: parent.right; top: searchBox.bottom; bottom: parent.bottom; topMargin: 28 }
            clip: true; model: store.projects
            populate: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                NumberAnimation { property: "x"; from: -24; to: 0; duration: 300; easing.type: Easing.OutCubic } }
            displaced: Transition { NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic } }
            delegate: ProjectListItem {}
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
        }
    }

    // ---- project detail ----
    Flickable {
        anchors { left: directory.right; right: parent.right; top: parent.top; bottom: parent.bottom }
        contentHeight: chart.height + 40; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Connections { target: store; function onProjectChanged() { chartIn.restart() } }
        ParallelAnimation { id: chartIn; running: true
            NumberAnimation { target: chart; property: "opacity"; from: 0; to: 1; duration: 300 }
            NumberAnimation { target: chartRise; property: "y"; from: 22; to: 0; duration: 380; easing.type: Easing.OutCubic } }

        Column {
            id: chart
            x: 24; y: 24; width: parent.width - 48; spacing: 18
            opacity: 0
            transform: Translate { id: chartRise; y: 22 }

            // header card
            Rectangle {
                width: parent.width; height: 128; radius: 16; color: "white"
                Column {
                    anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                    spacing: 6
                    Row { spacing: 10
                        Text { text: store.project.name; color: Theme.ink; font.pixelSize: 22; font.bold: true }
                        Rectangle { height: 22; radius: 11; width: statusT.width + 20
                                    color: Qt.lighter(Theme.projectStatusColor(store.project.status), 1.75)
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { id: statusT; anchors.centerIn: parent
                                           text: store.projectStatusLabel(store.project.status)
                                           color: Theme.projectStatusColor(store.project.status); font.pixelSize: 11; font.bold: true } }
                    }
                    Text { text: store.project.code + "   ·   " + store.clientName(store.project.clientId)
                           color: Theme.muted; font.pixelSize: 14 }
                    Text { text: "Manager: " + store.employeeName(store.project.managerId)
                                 + "   ·   Started " + Theme.dateLabel(store.project.startDate)
                           color: Theme.muted; font.pixelSize: 13 }
                }
                Row {
                    anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
                    spacing: 8
                    Repeater {
                        model: [ { l: "Planning", v: 0 }, { l: "Active", v: 1 }, { l: "On Hold", v: 2 }, { l: "Completed", v: 3 } ]
                        Rectangle {
                            width: stT.width + 18; height: 26; radius: 13
                            property bool current: store.project.status === modelData.v
                            color: current ? Theme.accent : "white"; border.color: current ? Theme.accent : "#DDE3EE"
                            Text { id: stT; anchors.centerIn: parent; text: modelData.l
                                   color: parent.current ? "white" : Theme.muted; font.pixelSize: 11; font.bold: true }
                            MouseArea { anchors.fill: parent; onClicked: store.setProjectStatus(store.selectedProjectId, modelData.v) }
                        }
                    }
                }
            }

            // budget KPIs
            Row {
                width: parent.width; spacing: 14
                property real cw: (width - 4 * 14) / 5
                Repeater {
                    model: [
                        { l: "Budget",    v: store.money(root.ps.budget || 0),         c: "#2563EB" },
                        { l: "Spent",     v: store.money(root.ps.spent || 0),          c: "#F59E0B" },
                        { l: "Remaining", v: store.money(root.ps.remaining || 0),      c: (root.ps.remaining || 0) < 0 ? "#EF4444" : "#10B981" },
                        { l: "% used",    v: (root.ps.percentUsed || 0) + "%",         c: "#8B5CF6" },
                        { l: "Unbilled",  v: store.money(root.ps.unbilled || 0),       c: "#EF4444" }
                    ]
                    KpiCard { width: parent.cw; label: modelData.l; value: modelData.v; accent: modelData.c }
                }
            }

            // quick actions
            Row {
                spacing: 10
                Repeater {
                    model: [ { t: "＋ Log time", d: "time" }, { t: "＋ Log expense", d: "expense" }, { t: "⟶ Generate invoice", d: "invoice" } ]
                    Rectangle {
                        width: qaT.width + 28; height: 34; radius: 10
                        color: qaMa.containsMouse ? "#EAF0FE" : "white"; border.color: "#DCE2F5"
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Text { id: qaT; anchors.centerIn: parent; text: modelData.t; color: Theme.accent; font.pixelSize: 13; font.bold: true }
                        MouseArea { id: qaMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: modelData.d === "time" ? addTimeDialog.open()
                                     : modelData.d === "expense" ? addExpenseDialog.open() : generateInvoiceDialog.open() }
                    }
                }
            }

            // assigned team — the formal QI_MANY_TO_MANY staffing roster (project_team
            // join table). Tap a chip to toggle; store.toggleTeamMember() calls
            // QiRelationSet::add()/remove() under the hood.
            SectionCard {
                width: parent.width; title: "Assigned team (" + (root.ps.assignedCount || 0) + ")"; bodySpacing: 10
                Flow {
                    width: parent.width; spacing: 8
                    Repeater {
                        model: root.ps.assignedTeam || []
                        Rectangle {
                            width: chipT.width + 20; height: 30; radius: 15
                            color: modelData.assigned ? Theme.accent : "white"
                            border.color: modelData.assigned ? Theme.accent : "#DCE2F5"
                            Behavior on color { ColorAnimation { duration: 130 } }
                            Text { id: chipT; anchors.centerIn: parent; text: modelData.name
                                   color: modelData.assigned ? "white" : Theme.muted; font.pixelSize: 12; font.bold: true }
                            MouseArea { anchors.fill: parent
                                        onClicked: store.toggleTeamMember(store.selectedProjectId, modelData.id) }
                        }
                    }
                }
                Text { visible: (root.ps.assignedTeam || []).length === 0
                       text: "No employees available"; color: Theme.muted; font.pixelSize: 13 }
            }

            // hours actually logged — deliberately a separate signal from the
            // assigned-team roster above (see models.h's note on TimeEntry).
            SectionCard {
                id: teamCard
                width: parent.width; title: "Hours logged (" + (root.ps.teamCount || 0) + ")"; bodySpacing: 8
                property real maxHours: {
                    var m = 1
                    for (var i = 0; i < (root.ps.team || []).length; i++) m = Math.max(m, root.ps.team[i].hours)
                    return m
                }
                Repeater { model: root.ps.team || []
                    Row { width: parent.width; spacing: 10
                        Text { text: modelData.name; color: Theme.ink; font.pixelSize: 13
                               width: 200; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 14; radius: 7; color: Theme.accent
                            width: Math.max(6, (modelData.hours / teamCard.maxHours) * (parent.width - 260))
                            Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.OutCubic } }
                        }
                        Text { text: modelData.hours.toFixed(1) + " hrs"; color: Theme.muted; font.pixelSize: 12; font.bold: true
                               anchors.verticalCenter: parent.verticalCenter } } }
                Text { visible: (root.ps.team || []).length === 0; text: "No hours logged yet"; color: Theme.muted; font.pixelSize: 13 }
            }

            // two columns: recent activity + invoices
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Column {
                    width: parent.cw; spacing: 18
                    SectionCard { width: parent.width; title: "Recent time entries"; bodySpacing: 8
                        Repeater { model: store.projectTimeEntries
                            Row { width: parent.width; visible: index < 8
                                Column { width: parent.width - 90; spacing: 1
                                    Text { text: store.employeeName(model.employee) + " · " + model.hours.toFixed(1) + " hrs"
                                           color: Theme.ink; font.pixelSize: 13; font.bold: true }
                                    Text { text: model.notes; color: Theme.muted; font.pixelSize: 11
                                           width: parent.width; elide: Text.ElideRight } }
                                Text { text: Theme.dateLabel(model.date, "MMM d"); color: Theme.muted; font.pixelSize: 11
                                       anchors.right: parent.right } } }
                        Text { visible: store.projectTimeEntries.count === 0; text: "No time logged yet"; color: Theme.muted; font.pixelSize: 13 }
                    }
                    SectionCard { width: parent.width; title: "Recent expenses"; bodySpacing: 8
                        Repeater { model: store.projectExpenses
                            Row { width: parent.width; visible: index < 8
                                Column { width: parent.width - 90; spacing: 1
                                    Text { text: model.category + (model.billable ? "" : "  (non-billable)")
                                           color: Theme.ink; font.pixelSize: 13; font.bold: true }
                                    Text { text: store.employeeName(model.employee); color: Theme.muted; font.pixelSize: 11 } }
                                Text { text: store.money(model.amount); color: Theme.ink; font.pixelSize: 12; font.bold: true
                                       anchors.right: parent.right } } }
                        Text { visible: store.projectExpenses.count === 0; text: "No expenses yet"; color: Theme.muted; font.pixelSize: 13 }
                    }
                }

                SectionCard {
                    width: parent.cw; title: "Invoices"; bodySpacing: 10
                    Repeater { model: store.projectInvoices
                        Rectangle {
                            id: invRow
                            width: parent.width; height: 52; radius: 10; color: "#F8F9FB"
                            property int invStatus: model.status
                            property string invDue: model.dueDate
                            Rectangle { width: 4; height: parent.height - 16; radius: 2
                                        color: Theme.invoiceStatusColor(invRow.invStatus, invRow.invDue)
                                        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } }
                            Column { spacing: 2
                                anchors { left: parent.left; leftMargin: 22; verticalCenter: parent.verticalCenter }
                                Text { text: model.number + "  ·  " + store.money(model.amount)
                                       color: Theme.ink; font.pixelSize: 13; font.bold: true }
                                Text { text: store.invoiceStatusLabel(invRow.invStatus, invRow.invDue) + "  ·  due " + Theme.dateLabel(invRow.invDue)
                                       color: Theme.invoiceStatusColor(invRow.invStatus, invRow.invDue); font.pixelSize: 11 } }
                        } }
                    Text { visible: store.projectInvoices.count === 0; text: "No invoices yet"; color: Theme.muted; font.pixelSize: 13 }
                    Rectangle { width: parent.width; height: 30; color: "transparent"
                        Text { anchors.right: parent.right; text: "View all invoices ⟶"; color: Theme.accent
                               font.pixelSize: 12; font.bold: true
                               MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: root.switchToInvoices() } } }
                }
            }
            Item { width: 1; height: 8 }
        }
    }

    // dialogs owned by this view
    AddProjectDialog      { id: addProjectDialog }
    AddTimeEntryDialog     { id: addTimeDialog;         projectId: store.selectedProjectId; lockProject: true }
    AddExpenseDialog       { id: addExpenseDialog;      projectId: store.selectedProjectId; lockProject: true }
    GenerateInvoiceDialog  { id: generateInvoiceDialog; projectId: store.selectedProjectId }
}
