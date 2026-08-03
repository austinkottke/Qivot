import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    property int currentTab: 0
    property int myTab: 1

    visible: opacity > 0
    opacity: currentTab === myTab ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    // look up the by-org rollup for a given org name
    function orgInfo(name) {
        var list = store.orgs;
        for (var i = 0; i < list.length; i++) if (list[i].org === name) return list[i];
        return null;
    }

    Flickable {
        anchors.fill: parent
        contentHeight: card.height + 48
        clip: true

        SectionCard {
            id: card
            x: 24; y: 24; width: parent.width - 48
            title: "Project earnings by organization"

            Text { text: "Projects roll up to an Organization (Vision's org structure). Revenue = Σ hours × bill rate; Compensation = Σ hours × cost rate; Multiplier = revenue ÷ compensation."
                   width: parent.width; color: Theme.muted; font.pixelSize: 12; wrapMode: Text.WordWrap }

            // header
            Row {
                width: parent.width; spacing: 0; height: 20
                Text { width: parent.width * 0.32; text: "PROJECT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.16; text: "PRINCIPAL / PM"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.13; horizontalAlignment: Text.AlignRight; text: "FEE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.13; horizontalAlignment: Text.AlignRight; text: "REVENUE"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.12; horizontalAlignment: Text.AlignRight; text: "COMP"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
                Text { width: parent.width * 0.14; horizontalAlignment: Text.AlignRight; text: "PROFIT"; color: Theme.muted; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1 }
            }
            Rectangle { width: parent.width; height: 1; color: Theme.border }

            Repeater {
                model: store.projects
                Column {
                    width: parent.width; spacing: 0
                    property bool showHeader: index === 0 || store.projects[index - 1].org !== modelData.org
                    property var oi: root.orgInfo(modelData.org)

                    // ---- organization band header ----
                    Item {
                        width: parent.width; height: showHeader ? 42 : 0; visible: showHeader; clip: true
                        Row {
                            anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                            spacing: 8
                            Rectangle { width: 4; height: 18; radius: 2; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.org; color: Theme.ink; font.pixelSize: 14; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
                            Text { visible: oi !== null
                                   text: oi ? "· " + oi.count + " projects · " + oi.multiplier.toFixed(2) + "× multiplier" : ""
                                   color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                        }
                        Text {
                            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                            text: oi ? store.money(oi.profit) + " profit" : ""
                            color: Theme.good; font.pixelSize: 13; font.bold: true
                        }
                    }

                    // ---- project row ----
                    Row {
                        width: parent.width; height: 46; spacing: 0
                        Row {
                            width: parent.width * 0.32; height: parent.height; spacing: 10
                            Rectangle { width: 30; height: 30; radius: 8; anchors.verticalCenter: parent.verticalCenter
                                        color: Theme.keyColor(modelData.wbs1)
                                        Text { anchors.centerIn: parent; text: "◱"; color: "white"; font.pixelSize: 13 } }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter; spacing: 1
                                Text { text: modelData.name; color: Theme.ink; font.pixelSize: 13; font.bold: true; elide: Text.ElideRight; width: parent.parent.parent.width * 0.32 - 40 }
                                Row { spacing: 6
                                    Text { text: modelData.wbs1; color: Theme.muted; font.pixelSize: 10 }
                                    Rectangle { width: st.width + 12; height: 15; radius: 7; anchors.verticalCenter: parent.verticalCenter; color: Theme.chip
                                                Text { id: st; anchors.centerIn: parent; text: Theme.statusLabel(modelData.status)
                                                       color: Theme.statusColor(modelData.status); font.pixelSize: 9; font.bold: true } }
                                }
                            }
                        }
                        Column {
                            width: parent.width * 0.16; height: parent.height; spacing: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text { text: modelData.principal; color: Theme.ink; font.pixelSize: 12; elide: Text.ElideRight; width: parent.width }
                            Text { text: "PM: " + modelData.manager; color: Theme.muted; font.pixelSize: 10; elide: Text.ElideRight; width: parent.width }
                        }
                        Text { width: parent.width * 0.13; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.fee); color: Theme.muted; font.pixelSize: 13 }
                        Text { width: parent.width * 0.13; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.revenue); color: Theme.ink; font.pixelSize: 13 }
                        Text { width: parent.width * 0.12; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignRight
                               text: store.money(modelData.compensation); color: Theme.muted; font.pixelSize: 13 }
                        Column {
                            width: parent.width * 0.14; height: parent.height; spacing: 1
                            Text { width: parent.width; horizontalAlignment: Text.AlignRight
                                   text: store.money(modelData.profit); color: Theme.good; font.pixelSize: 13; font.bold: true }
                            Text { width: parent.width; horizontalAlignment: Text.AlignRight
                                   text: modelData.multiplier.toFixed(2) + "× · " + modelData.feePct.toFixed(0) + "% fee"
                                   color: Theme.muted; font.pixelSize: 10 }
                        }
                    }
                    Rectangle { width: parent.width; height: 1; color: Theme.border; opacity: 0.5 }
                }
            }
        }
    }
}
