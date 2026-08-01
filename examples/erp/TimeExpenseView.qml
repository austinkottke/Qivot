import QtQuick 2.15
import QtQuick.Controls 2.15

// A firm-wide timesheet + expense ledger, optionally filtered to one project.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 3

    opacity: currentTab === myIndex ? 1 : 0
    visible: opacity > 0.01
    enabled: currentTab === myIndex
    transform: Translate { x: currentTab === myIndex ? 0 : (currentTab < myIndex ? 28 : -28)
                           Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
    Behavior on opacity { NumberAnimation { duration: 240 } }

    Flickable {
        anchors.fill: parent
        contentHeight: col.height + 48; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: col
            x: 28; y: 24; width: parent.width - 56; spacing: 18

            Row {
                width: parent.width
                Text { text: "Time & expense"; color: Theme.ink; font.pixelSize: 24; font.bold: true
                       anchors.verticalCenter: parent.verticalCenter }
                Item { width: parent.width - 560; height: 1 }
                Row { spacing: 10; anchors.verticalCenter: parent.verticalCenter
                    Rectangle { width: allT.width + 20; height: 34; radius: 10
                        property bool active: store.teFilterProject === 0
                        color: active ? Theme.accent : "white"; border.color: active ? Theme.accent : "#DCE2F5"
                        Text { id: allT; anchors.centerIn: parent; text: "All projects"
                               color: parent.active ? "white" : Theme.muted; font.pixelSize: 13; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: store.setTeFilterProject(0) } }
                    ComboBox { id: pf; width: 240; model: store.projects
                               textRole: "name"; valueRole: "id"
                               onActivated: store.setTeFilterProject(currentValue) }
                    Rectangle { width: ltT.width + 24; height: 34; radius: 10
                        color: ltMa.containsMouse ? "#EAF0FE" : "white"; border.color: "#DCE2F5"
                        Text { id: ltT; anchors.centerIn: parent; text: "＋ Log time"; color: Theme.accent; font.pixelSize: 13; font.bold: true }
                        MouseArea { id: ltMa; anchors.fill: parent; hoverEnabled: true; onClicked: addTimeDialog.open() } }
                    Rectangle { width: leT.width + 24; height: 34; radius: 10
                        color: leMa.containsMouse ? "#EAF0FE" : "white"; border.color: "#DCE2F5"
                        Text { id: leT; anchors.centerIn: parent; text: "＋ Log expense"; color: Theme.accent; font.pixelSize: 13; font.bold: true }
                        MouseArea { id: leMa; anchors.fill: parent; hoverEnabled: true; onClicked: addExpenseDialog.open() } }
                }
            }

            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                SectionCard {
                    width: parent.cw; title: "Time entries (" + store.timeEntries.count + ")"; bodySpacing: 6
                    Repeater { model: store.timeEntries
                        Rectangle { width: parent.width; height: 46; radius: 8
                            color: index % 2 === 0 ? "#F8F9FB" : "white"
                            Row { anchors.fill: parent; anchors.margins: 8
                                Column { width: parent.width - 140; spacing: 1
                                    Text { text: store.employeeName(model.employee) + "  ·  " + store.projectCode(model.project)
                                           color: Theme.ink; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; width: parent.width }
                                    Text { text: Theme.dateLabel(model.date, "MMM d") + (model.billable ? "" : "  ·  non-billable")
                                                 + (model.invoiceId > 0 ? "  ·  invoiced" : "")
                                           color: Theme.muted; font.pixelSize: 10 } }
                                Text { text: model.hours.toFixed(1) + " hrs"; color: Theme.ink; font.pixelSize: 12; font.bold: true
                                       anchors.verticalCenter: parent.verticalCenter; width: 100; horizontalAlignment: Text.AlignRight } }
                        } }
                    Text { visible: store.timeEntries.count === 0; text: "No time entries"; color: Theme.muted; font.pixelSize: 13 }
                }

                SectionCard {
                    width: parent.cw; title: "Expenses (" + store.expenses.count + ")"; bodySpacing: 6
                    Repeater { model: store.expenses
                        Rectangle { width: parent.width; height: 46; radius: 8
                            color: index % 2 === 0 ? "#F8F9FB" : "white"
                            Row { anchors.fill: parent; anchors.margins: 8
                                Column { width: parent.width - 140; spacing: 1
                                    Text { text: model.category + "  ·  " + store.projectCode(model.project)
                                           color: Theme.ink; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; width: parent.width }
                                    Text { text: Theme.dateLabel(model.date, "MMM d") + (model.billable ? "" : "  ·  non-billable")
                                                 + (model.invoiceId > 0 ? "  ·  invoiced" : "")
                                           color: Theme.muted; font.pixelSize: 10 } }
                                Text { text: store.money(model.amount); color: Theme.ink; font.pixelSize: 12; font.bold: true
                                       anchors.verticalCenter: parent.verticalCenter; width: 100; horizontalAlignment: Text.AlignRight } }
                        } }
                    Text { visible: store.expenses.count === 0; text: "No expenses"; color: Theme.muted; font.pixelSize: 13 }
                }
            }
            Item { width: 1; height: 8 }
        }
    }

    AddTimeEntryDialog { id: addTimeDialog;    lockProject: false }
    AddExpenseDialog   { id: addExpenseDialog; lockProject: false }
}
