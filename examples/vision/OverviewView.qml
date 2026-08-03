import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int currentTab: 0
    property int myTab: 0
    signal switchToProjects()

    visible: opacity > 0
    opacity: currentTab === myTab ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    readonly property var m: store.metrics

    // the Vision "Key Financial Metrics" list, as label/value cells
    readonly property var kfm: [
        { k: "Backlog (contract)",   v: store.money(m.backlog) },
        { k: "Compensation (JTD)",   v: store.money(m.compensation) },
        { k: "YTD Billed",           v: store.money(m.ytdBilled) },
        { k: "Total Unbilled",       v: store.money(m.unbilled) },
        { k: "Outstanding A/R",      v: store.money(m.outstandingAr) },
        { k: "Cash on Hand",         v: store.money(m.cashOnHand) },
        { k: "Active Projects",      v: "" + m.activeProjects },
        { k: "Active Staff",         v: "" + m.headcount }
    ]

    Flickable {
        anchors.fill: parent
        contentHeight: col.height + 48
        clip: true

        Column {
            id: col
            x: 24; y: 24; width: parent.width - 48; spacing: 18

            // ---- headline KPI row ----
            Row {
                width: parent.width; spacing: 16
                property real cardW: (width - 3 * 16) / 4
                KpiCard { width: parent.cardW; label: "YTD Revenue"; accent: Theme.accent
                          value: store.money(root.m.ytdRevenue); sub: "job-to-date labor value" }
                KpiCard { width: parent.cardW; label: "YTD Profit"; accent: Theme.good
                          value: store.money(root.m.ytdProfit)
                          sub: (root.m.profitPct !== undefined ? root.m.profitPct.toFixed(1) : "0") + "% profit margin" }
                KpiCard { width: parent.cardW; label: "Effective Multiplier"; accent: Theme.accent2
                          value: (root.m.multiplier !== undefined ? root.m.multiplier.toFixed(2) : "0") + "×"
                          sub: "revenue ÷ compensation" }
                KpiCard { width: parent.cardW; label: "Avg Utilization"; accent: Theme.warn
                          value: (root.m.avgUtil !== undefined ? root.m.avgUtil.toFixed(0) : "0") + "%"
                          sub: root.m.headcount + " active staff" }
            }

            // ---- Key Financial Metrics (Vision-style detail grid) ----
            SectionCard {
                width: parent.width
                title: "Key Financial Metrics"
                Grid {
                    width: parent.width; columns: 4; rowSpacing: 14; columnSpacing: 14
                    Repeater {
                        model: root.kfm
                        Rectangle {
                            width: (parent.width - 3 * 14) / 4; height: 60; radius: 10; color: Theme.field
                            Column {
                                anchors { left: parent.left; leftMargin: 14; verticalCenter: parent.verticalCenter }
                                spacing: 3
                                Text { text: modelData.k.toUpperCase(); color: Theme.muted
                                       font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                                Text { text: modelData.v; color: Theme.ink; font.pixelSize: 18; font.bold: true }
                            }
                        }
                    }
                }
            }

            // ---- earnings snapshot ----
            SectionCard {
                width: parent.width
                title: "Project earnings"
                Row {
                    width: parent.width
                    Text { width: parent.width - 90; text: "Revenue vs. contract fee and profit — a GROUP BY over the labor ledger, rolled up by project."
                           color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap }
                    Text { width: 90; horizontalAlignment: Text.AlignRight
                           text: "View all →"; color: Theme.accent; font.pixelSize: 12; font.bold: true
                           MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                       onClicked: root.switchToProjects() } }
                }
                Row {
                    width: parent.width; spacing: 0
                    Text { width: parent.width * 0.36; text: "PROJECT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                    Text { width: parent.width * 0.16; horizontalAlignment: Text.AlignRight; text: "FEE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                    Text { width: parent.width * 0.16; horizontalAlignment: Text.AlignRight; text: "REVENUE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                    Text { width: parent.width * 0.16; horizontalAlignment: Text.AlignRight; text: "PROFIT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                    Text { width: parent.width * 0.16; horizontalAlignment: Text.AlignRight; text: "MULT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                }
                Repeater {
                    model: store.projects
                    Row {
                        width: parent.width; height: 32; spacing: 0
                        Row {
                            width: parent.width * 0.36; height: parent.height; spacing: 8
                            Rectangle { width: 8; height: 8; radius: 4; anchors.verticalCenter: parent.verticalCenter
                                        color: Theme.statusColor(modelData.status) }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.name
                                   color: Theme.ink; font.pixelSize: 13; font.bold: true; elide: Text.ElideRight; width: parent.width - 20 }
                        }
                        Text { width: parent.width * 0.16; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.fee); color: Theme.muted; font.pixelSize: 13 }
                        Text { width: parent.width * 0.16; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.revenue); color: Theme.ink; font.pixelSize: 13 }
                        Text { width: parent.width * 0.16; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.profit); color: Theme.good; font.pixelSize: 13; font.bold: true }
                        Text { width: parent.width * 0.16; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: modelData.multiplier.toFixed(2) + "×"; color: Theme.muted; font.pixelSize: 13 }
                    }
                }
            }

            // ---- utilization snapshot ----
            SectionCard {
                width: parent.width
                title: "Team utilization"
                Repeater {
                    model: store.staff
                    Row {
                        width: parent.width; height: 30; spacing: 12
                        Text { width: 150; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: modelData.name; color: Theme.ink; font.pixelSize: 13 }
                        Text { width: 120; height: parent.height; verticalAlignment: Text.AlignVCenter
                               text: modelData.title; color: Theme.muted; font.pixelSize: 12 }
                        Rectangle {
                            width: parent.width - 150 - 120 - 70 - 24; height: 8; radius: 4
                            anchors.verticalCenter: parent.verticalCenter; color: Theme.track
                            Rectangle { width: parent.width * Math.min(1, modelData.util / 100); height: parent.height; radius: 4
                                        color: Theme.ratioColor(modelData.util) }
                        }
                        Text { width: 58; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: modelData.util.toFixed(0) + "%"; color: Theme.ink; font.pixelSize: 13; font.bold: true }
                    }
                }
            }
        }
    }
}
