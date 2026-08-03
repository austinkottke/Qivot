import QtQuick 2.15
import QtQuick.Controls 2.15

// Company Project Visualization — a treemap grouped by Organization, tiles sized
// by Compensation and coloured by Profit (red = low → blue = high), mirroring
// Vision's dashboard. Layout is a slice-and-dice computed in JS from store.treemap.
Item {
    id: root
    property int currentTab: 0
    property int myTab: 2

    visible: opacity > 0
    opacity: currentTab === myTab ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 220 } }

    function heatColor(h) {
        // 0 = red (low profit) → 1 = blue (high profit)
        var r = 0.94 + (0.31 - 0.94) * h;
        var g = 0.30 + (0.55 - 0.30) * h;
        var b = 0.30 + (1.00 - 0.30) * h;
        return Qt.rgba(r, g, b, 0.94);
    }

    // Wide: org COLUMNS (width ∝ compensation), tiles stacked vertically within.
    // Compact (phone): org ROWS (height ∝ compensation), tiles laid out horizontally
    // within each band — reads far better in portrait than three thin columns.
    function computeCells(W, H, tiles, compact) {
        if (!tiles || tiles.length === 0 || W <= 0 || H <= 0) return [];
        var order = [], byOrg = {}, orgSize = {}, total = 0;
        for (var i = 0; i < tiles.length; i++) {
            var t = tiles[i];
            if (byOrg[t.org] === undefined) { byOrg[t.org] = []; orgSize[t.org] = 0; order.push(t.org); }
            byOrg[t.org].push(t); orgSize[t.org] += t.size; total += t.size;
        }
        var out = [], pad = 6, headerH = 24, o, org, list, j, c;
        if (!compact) {
            var x = 0;
            for (o = 0; o < order.length; o++) {
                org = order[o];
                var colW = W * (orgSize[org] / total);
                out.push({ header: true, org: org, x: x, y: 0, w: colW, h: headerH });
                var y = headerH, innerHc = H - headerH; list = byOrg[org];
                for (j = 0; j < list.length; j++) {
                    c = list[j];
                    var th = innerHc * (c.size / orgSize[org]);
                    out.push({ header: false, x: x + pad / 2, y: y + pad / 2,
                               w: Math.max(0, colW - pad), h: Math.max(0, th - pad),
                               name: c.name, wbs1: c.wbs1, profit: c.profit, revenue: c.revenue, heat: c.heat });
                    y += th;
                }
                x += colW;
            }
        } else {
            var yy = 0;
            for (o = 0; o < order.length; o++) {
                org = order[o];
                var rowH = H * (orgSize[org] / total);
                out.push({ header: true, org: org, x: 0, y: yy, w: W, h: headerH });
                var xx = 0, innerY = yy + headerH, innerHr = rowH - headerH; list = byOrg[org];
                for (j = 0; j < list.length; j++) {
                    c = list[j];
                    var tw = W * (c.size / orgSize[org]);
                    out.push({ header: false, x: xx + pad / 2, y: innerY + pad / 2,
                               w: Math.max(0, tw - pad), h: Math.max(0, innerHr - pad),
                               name: c.name, wbs1: c.wbs1, profit: c.profit, revenue: c.revenue, heat: c.heat });
                    xx += tw;
                }
                yy += rowH;
            }
        }
        return out;
    }

    Column {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        // header + Vision-style measure selectors (static — this is a demo)
        Row {
            width: parent.width
            Column {
                width: Theme.compact ? parent.width : parent.width - 520; spacing: 2
                Text { text: "Company Project Visualization"; color: Theme.ink; font.pixelSize: Theme.compact ? 17 : 20; font.bold: true }
                Text { text: "Every active project, grouped by organization."; color: Theme.muted; font.pixelSize: 12 }
            }
            Row {
                visible: !Theme.compact          // the Color/Size/Grouping selectors don't fit a phone
                width: 520; height: 44; spacing: 10; layoutDirection: Qt.LeftToRight
                Repeater {
                    model: [ { k: "Color", v: "JTD Profit" }, { k: "Size", v: "Compensation" }, { k: "Grouping", v: "Organization" } ]
                    Rectangle {
                        width: 166; height: 44; radius: 10; color: Theme.card; border.width: 1; border.color: Theme.border
                        Column { spacing: 1
                            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                            Text { text: modelData.k.toUpperCase(); color: Theme.muted; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1 }
                            Text { text: modelData.v; color: Theme.ink; font.pixelSize: 13; font.bold: true } }
                        Text { anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                               text: "▾"; color: Theme.muted; font.pixelSize: 12 }
                    }
                }
            }
        }

        Rectangle {
            width: parent.width; height: parent.height - 44 - 14 - 30 - 14; radius: 16
            color: Theme.card; border.width: 1; border.color: Theme.border; clip: true

            Item {
                id: plane
                anchors.fill: parent; anchors.margins: 12
                property var cells: root.computeCells(width, height, store.treemap, Theme.compact)

                Repeater {
                    model: plane.cells
                    Item {
                        x: modelData.x; y: modelData.y; width: modelData.w; height: modelData.h

                        // organization band header
                        Text {
                            visible: modelData.header === true
                            anchors { left: parent.left; leftMargin: 2; verticalCenter: parent.verticalCenter }
                            text: modelData.org ? modelData.org : ""; color: Theme.ink; font.pixelSize: 13; font.bold: true; elide: Text.ElideRight
                            width: parent.width - 4
                        }

                        // project tile
                        Rectangle {
                            visible: modelData.header !== true
                            anchors.fill: parent; radius: 8
                            color: root.heatColor(modelData.heat === undefined ? 0.5 : modelData.heat)
                            Column {
                                anchors { left: parent.left; top: parent.top; margins: 10; right: parent.right }
                                spacing: 2
                                visible: parent.height > 42 && parent.width > 80
                                Text { text: modelData.name ? modelData.name : ""; color: "white"; font.pixelSize: 13; font.bold: true
                                       elide: Text.ElideRight; width: parent.width }
                                Text { text: modelData.profit !== undefined ? store.money(modelData.profit) + " profit" : ""
                                       color: "white"; opacity: 0.92; font.pixelSize: 12 }
                                Text { text: modelData.revenue !== undefined ? store.money(modelData.revenue) + " revenue" : ""
                                       color: "white"; opacity: 0.75; font.pixelSize: 10; visible: parent.height > 62 }
                            }
                        }
                    }
                }
            }
        }

        // legend
        Row {
            width: parent.width; height: 30; spacing: 10
            Text { text: "Profit:"; color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "low"; color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            Rectangle {
                width: Theme.compact ? 130 : 220; height: 10; radius: 5; anchors.verticalCenter: parent.verticalCenter
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: root.heatColor(0) }
                    GradientStop { position: 0.5; color: root.heatColor(0.5) }
                    GradientStop { position: 1.0; color: root.heatColor(1) }
                }
            }
            Text { text: "high"; color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
            Text { visible: !Theme.compact; text: "· tile size = compensation (direct labor cost)"; color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
        }
    }
}
