import QtQuick 2.15
import QtQuick.Controls 2.15

// A single, filterable list of every invoice — status filter chips up top,
// per-row actions (Send / Mark Paid) that mirror a normal AR workflow.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 4

    opacity: currentTab === myIndex ? 1 : 0
    visible: opacity > 0.01
    enabled: currentTab === myIndex
    transform: Translate { x: currentTab === myIndex ? 0 : 28
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
                Text { text: "Invoices"; color: Theme.ink; font.pixelSize: 24; font.bold: true
                       anchors.verticalCenter: parent.verticalCenter }
                Item { width: parent.width - 420; height: 1 }
                Row { spacing: 8; anchors.verticalCenter: parent.verticalCenter
                    Repeater {
                        model: [ { l: "All", v: "" }, { l: "Draft", v: "Draft" }, { l: "Sent", v: "Sent" },
                                 { l: "Overdue", v: "Overdue" }, { l: "Paid", v: "Paid" } ]
                        Rectangle {
                            width: fT.width + 20; height: 32; radius: 10
                            property bool active: store.invoiceFilter === modelData.v
                            color: active ? Theme.accent : "white"; border.color: active ? Theme.accent : "#DCE2F5"
                            Text { id: fT; anchors.centerIn: parent; text: modelData.l
                                   color: parent.active ? "white" : Theme.muted; font.pixelSize: 12; font.bold: true }
                            MouseArea { anchors.fill: parent; onClicked: store.setInvoiceFilter(modelData.v) }
                        }
                    }
                }
            }

            SectionCard {
                width: parent.width; title: store.invoices.count + " invoice" + (store.invoices.count === 1 ? "" : "s"); bodySpacing: 8
                Repeater { model: store.invoices
                    Rectangle {
                        id: row
                        width: parent.width; height: 58; radius: 10; color: "#F8F9FB"
                        property int    invId: model.id
                        property int    invStatus: model.status
                        property string invDue: model.dueDate
                        Rectangle { width: 4; height: parent.height - 16; radius: 2
                                    color: Theme.invoiceStatusColor(row.invStatus, row.invDue)
                                    anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } }
                        Column { spacing: 2
                            anchors { left: parent.left; leftMargin: 22; verticalCenter: parent.verticalCenter }
                            width: parent.width - 340
                            Text { text: model.number + "  ·  " + store.clientName(model.client)
                                   color: Theme.ink; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight; width: parent.width }
                            Text { text: store.projectName(model.project) + "  ·  issued " + Theme.dateLabel(model.issueDate)
                                         + "  ·  due " + Theme.dateLabel(row.invDue)
                                   color: Theme.muted; font.pixelSize: 11; elide: Text.ElideRight; width: parent.width }
                        }
                        Row { spacing: 14
                            anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                            Column { spacing: 2
                                Text { text: store.money(model.amount); color: Theme.ink; font.pixelSize: 15; font.bold: true
                                       anchors.right: parent.right }
                                Text { text: store.invoiceStatusLabel(row.invStatus, row.invDue)
                                       color: Theme.invoiceStatusColor(row.invStatus, row.invDue); font.pixelSize: 11; font.bold: true
                                       anchors.right: parent.right }
                            }
                            Rectangle { visible: row.invStatus === 0   // Draft -> Sent
                                width: sendT.width + 20; height: 30; radius: 9; color: Theme.accent
                                anchors.verticalCenter: parent.verticalCenter
                                Text { id: sendT; anchors.centerIn: parent; text: "Send"; color: "white"; font.pixelSize: 12; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: store.setInvoiceStatus(row.invId, 1) } }
                            Rectangle { visible: row.invStatus === 1   // Sent (or overdue) -> Paid
                                width: paidT.width + 20; height: 30; radius: 9; color: "#10B981"
                                anchors.verticalCenter: parent.verticalCenter
                                Text { id: paidT; anchors.centerIn: parent; text: "Mark paid"; color: "white"; font.pixelSize: 12; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: store.setInvoiceStatus(row.invId, 2) } }
                        }
                    } }
                Text { visible: store.invoices.count === 0; text: "No invoices match this filter"; color: Theme.muted; font.pixelSize: 13 }
            }
            Item { width: 1; height: 8 }
        }
    }
}
