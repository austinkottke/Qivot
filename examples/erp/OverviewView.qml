import QtQuick 2.15
import QtQuick.Controls 2.15

// Firm-wide analytics — all fed by aggregate queries in store.overview.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 0
    property var ov: store.overview
    signal switchToProjects()

    opacity: currentTab === myIndex ? 1 : 0
    visible: opacity > 0.01
    enabled: currentTab === myIndex
    transform: Translate { x: currentTab === myIndex ? 0 : -28
                           Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
    Behavior on opacity { NumberAnimation { duration: 240 } }

    Flickable {
        anchors.fill: parent
        contentHeight: col.height + 48; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: col
            x: 28; y: 24; width: parent.width - 56; spacing: 18

            Text { text: "Firm overview"; color: Theme.ink; font.pixelSize: 24; font.bold: true }

            // KPI row
            Row {
                width: parent.width; spacing: 14
                property real cw: (width - 4 * 14) / 5
                Repeater {
                    model: [
                        { l: "Clients",         v: (root.ov.clients || 0),                            c: "#2563EB" },
                        { l: "Active projects", v: (root.ov.activeProjects || 0),                      c: "#10B981" },
                        { l: "Open pipeline",   v: store.money(root.ov.openPipeline || 0),              c: "#F59E0B" },
                        { l: "Unbilled",        v: store.money(root.ov.unbilled || 0),                  c: "#8B5CF6" },
                        { l: "AR outstanding",  v: store.money(root.ov.arTotal || 0),                   c: "#EF4444" }
                    ]
                    KpiCard { width: parent.cw; label: modelData.l; value: modelData.v; accent: modelData.c }
                }
            }

            // revenue + pipeline charts
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Rectangle {
                    width: parent.cw; height: 250; radius: 16; color: Theme.card
                    Text { id: revTitle; text: "Invoiced revenue, last 6 months"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Row {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                                  leftMargin: 20; rightMargin: 20; bottomMargin: 18 }
                        height: 170; spacing: 14
                        Repeater {
                            model: root.ov.revenueByMonth || []
                            Column {
                                width: (parent.width - 5 * 14) / 6; height: parent.height
                                Item { width: 1; height: parent.height - bar.height - 34 }
                                Text { anchors.horizontalCenter: parent.horizontalCenter
                                       text: store.money(modelData.amount); color: Theme.ink; font.pixelSize: 11; font.bold: true }
                                Rectangle { id: bar; anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width - 10
                                    height: Math.max(4, (modelData.amount / Math.max(1, root.ov.revenueByMonthMax)) * 120); radius: 6
                                    gradient: Gradient { GradientStop { position: 0; color: "#2563EB" }
                                                         GradientStop { position: 1; color: "#60A5FA" } }
                                    Behavior on height { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } } }
                                Text { anchors.horizontalCenter: parent.horizontalCenter
                                       text: modelData.label; color: Theme.muted; font.pixelSize: 11; topPadding: 6 }
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.cw; height: 250; radius: 16; color: Theme.card
                    Text { id: plTitle; text: "Pipeline by stage"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Column {
                        anchors { left: parent.left; right: parent.right; top: plTitle.bottom
                                  leftMargin: 20; rightMargin: 20; topMargin: 14 }
                        spacing: 11
                        property real maxVal: {
                            var m = 1
                            for (var i = 0; i < (root.ov.byStage || []).length; i++) m = Math.max(m, root.ov.byStage[i].value)
                            return m
                        }
                        Repeater {
                            model: root.ov.byStage || []
                            Row {
                                width: parent.width; spacing: 10
                                Text { text: modelData.label; color: Theme.ink; font.pixelSize: 12
                                       width: 78; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 16; radius: 8
                                    color: Theme.stageColor(modelData.stage)
                                    width: Math.max(8, (modelData.value / parent.parent.maxVal) * (parent.width - 180))
                                    Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } } }
                                Text { text: modelData.count + " · " + store.money(modelData.value); color: Theme.muted
                                       font.pixelSize: 11; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
                    }
                }
            }

            // AR aging + top clients
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Rectangle {
                    width: parent.cw; height: 150; radius: 16; color: Theme.card
                    Text { id: agTitle; text: "AR aging"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Row {
                        anchors { left: parent.left; top: agTitle.bottom; leftMargin: 20; topMargin: 16 }
                        spacing: 10
                        Repeater {
                            model: [ { l: "Current", k: "current", c: "#10B981" },
                                     { l: "1-30",     k: "d30",     c: "#F59E0B" },
                                     { l: "31-60",    k: "d60",     c: "#F97316" },
                                     { l: "61-90",    k: "d90",     c: "#EF4444" },
                                     { l: "90+",       k: "d90p",    c: "#B91C1C" } ]
                            Rectangle { width: 92; height: 68; radius: 12; color: Qt.rgba(0,0,0,0.02); border.color: "#EEF1F6"
                                Column { anchors.centerIn: parent; spacing: 3
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: store.money(root.ov.aging ? (root.ov.aging[modelData.k] || 0) : 0)
                                           color: modelData.c; font.pixelSize: 14; font.bold: true }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.l; color: Theme.muted; font.pixelSize: 11 } } }
                        }
                    }
                }

                SectionCard {
                    width: parent.cw; title: "Top clients by revenue"; bodySpacing: 8
                    Repeater {
                        model: root.ov.topClients || []
                        Item { width: parent.width; height: 18
                            Text { text: modelData.name; color: Theme.ink; font.pixelSize: 13
                                   anchors { left: parent.left; right: amt.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                   elide: Text.ElideRight }
                            Text { id: amt; text: store.money(modelData.amount); color: Theme.accent; font.pixelSize: 13; font.bold: true
                                   anchors { right: parent.right; verticalCenter: parent.verticalCenter } } }
                    }
                    Text { visible: (root.ov.topClients || []).length === 0
                           text: "No invoiced revenue yet"; color: Theme.muted; font.pixelSize: 13 }
                }
            }
            Item { width: 1; height: 8 }
        }
    }
}
