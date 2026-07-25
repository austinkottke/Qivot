import QtQuick 2.15
import QtQuick.Controls 2.15

// Practice analytics — all fed by aggregate queries in store.overview.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 0
    property var ov: store.overview
    signal switchToPatients()

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

            Text { text: "Practice overview"; color: Theme.ink; font.pixelSize: 24; font.bold: true }

            // KPI row (reusable KpiCard)
            Row {
                width: parent.width; spacing: 14
                property real cw: (width - 5 * 14) / 6
                Repeater {
                    model: [
                        { l: "Patients",        v: (root.ov.patients || 0),          c: "#3B82F6" },
                        { l: "Today's visits",  v: (root.ov.todayTotal || 0),        c: "#0E8C93" },
                        { l: "Arrived",         v: (root.ov.todayArrived || 0),      c: "#10B981" },
                        { l: "Completion",      v: (root.ov.completion || 0) + "%",  c: "#8B5CF6" },
                        { l: "Active problems", v: (root.ov.activeProblems || 0),    c: "#EF4444" },
                        { l: "Avg age",         v: (root.ov.avgAge || 0),            c: "#F59E0B" }
                    ]
                    KpiCard { width: parent.cw; label: modelData.l; value: modelData.v; accent: modelData.c }
                }
            }

            // charts
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Rectangle {
                    width: parent.cw; height: 250; radius: 16; color: Theme.card
                    Text { id: bdTitle; text: "Appointments this week"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Row {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                                  leftMargin: 20; rightMargin: 20; bottomMargin: 18 }
                        height: 170; spacing: 10
                        Repeater {
                            model: root.ov.byDay || []
                            Column {
                                width: (parent.width - 60) / 7; height: parent.height
                                Item { width: 1; height: parent.height - bar.height - 34 }
                                Text { anchors.horizontalCenter: parent.horizontalCenter
                                       text: modelData.count; color: Theme.ink; font.pixelSize: 12; font.bold: true }
                                Rectangle { id: bar; anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width - 6
                                    height: Math.max(4, (modelData.count / Math.max(1, root.ov.byDayMax)) * 120); radius: 6
                                    gradient: Gradient { GradientStop { position: 0; color: "#0E8C93" }
                                                         GradientStop { position: 1; color: "#38C0C6" } }
                                    Behavior on height { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } } }
                                Text { anchors.horizontalCenter: parent.horizontalCenter
                                       text: modelData.label; color: Theme.muted; font.pixelSize: 11; topPadding: 6 }
                            }
                        }
                    }
                }

                Rectangle {
                    width: parent.cw; height: 250; radius: 16; color: Theme.card
                    Text { id: plTitle; text: "Provider load this week"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Column {
                        anchors { left: parent.left; right: parent.right; top: plTitle.bottom
                                  leftMargin: 20; rightMargin: 20; topMargin: 14 }
                        spacing: 11
                        Repeater {
                            model: root.ov.byProvider || []
                            Row {
                                width: parent.width; spacing: 10
                                Text { text: modelData.name; color: Theme.ink; font.pixelSize: 12
                                       width: 118; elide: Text.ElideRight; anchors.verticalCenter: parent.verticalCenter }
                                Rectangle { anchors.verticalCenter: parent.verticalCenter; height: 16; radius: 8; color: modelData.color
                                    width: Math.max(8, (modelData.count / Math.max(1, root.ov.byProviderMax)) * (parent.width - 160))
                                    Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } } }
                                Text { text: modelData.count; color: Theme.muted; font.pixelSize: 12; font.bold: true
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                        }
                    }
                }
            }

            // status + top conditions
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Rectangle {
                    width: parent.cw; height: 150; radius: 16; color: Theme.card
                    Text { id: stTitle; text: "Today's status"; color: Theme.ink; font.pixelSize: 16; font.bold: true
                           anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                    Row {
                        anchors { left: parent.left; top: stTitle.bottom; leftMargin: 20; topMargin: 16 }
                        spacing: 12
                        Repeater {
                            model: [ { l: "Scheduled", k: "scheduled", c: "#3B82F6" },
                                     { l: "Arrived",   k: "arrived",   c: "#10B981" },
                                     { l: "Completed", k: "completed", c: "#94A3B8" },
                                     { l: "Cancelled", k: "cancelled", c: "#EF4444" } ]
                            Rectangle { width: 96; height: 68; radius: 12; color: Qt.rgba(0,0,0,0.02); border.color: "#EEF1F6"
                                Column { anchors.centerIn: parent; spacing: 3
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: (root.ov.byStatus ? (root.ov.byStatus[modelData.k] || 0) : 0)
                                           color: modelData.c; font.pixelSize: 24; font.bold: true }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.l; color: Theme.muted; font.pixelSize: 11 } } }
                        }
                    }
                }

                SectionCard {
                    width: parent.cw; title: "Top active conditions"; bodySpacing: 7
                    Repeater {
                        model: root.ov.topConditions || []
                        Item { width: parent.width; height: 18
                            Text { text: modelData.name; color: Theme.ink; font.pixelSize: 13
                                   anchors { left: parent.left; right: cnt.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                   elide: Text.ElideRight }
                            Text { id: cnt; text: modelData.count; color: Theme.teal; font.pixelSize: 13; font.bold: true
                                   anchors { right: parent.right; verticalCenter: parent.verticalCenter } } }
                    }
                }
            }

            // full-text note search
            SectionCard {
                width: parent.width; title: "Search clinical notes"
                Row { spacing: 10; width: parent.width
                    Text { text: "FTS5 full-text"; color: Theme.muted; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter } }
                Rectangle {
                    width: parent.width; height: 42; radius: 10; color: Theme.field
                    border.color: noteSearch.activeFocus ? Theme.teal : "transparent"
                    Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                           text: "⌕"; color: Theme.muted; font.pixelSize: 20 }
                    TextField { id: noteSearch
                        anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                        placeholderText: "e.g. cholesterol, asthma, physical therapy…"
                        background: null; color: Theme.ink; font.pixelSize: 14
                        onTextChanged: noteDebounce.restart() }
                    Timer { id: noteDebounce; interval: 110; onTriggered: store.searchNotes(noteSearch.text) }
                }
                Text { visible: store.noteQuery.length > 0; text: store.noteResults.count + " matching notes"
                       color: Theme.muted; font.pixelSize: 12 }
                Repeater {
                    model: store.noteResults
                    Rectangle {
                        width: parent.width; height: nrCol.height + 20; radius: 10
                        color: nrHover.containsMouse ? "#F3F9FF" : "#F8F9FB"
                        Column { id: nrCol
                            anchors { left: parent.left; right: parent.right; top: parent.top; leftMargin: 14; rightMargin: 14; topMargin: 10 }
                            spacing: 3
                            Row { spacing: 8
                                Text { text: store.patientName(model.patientId); color: Theme.teal; font.pixelSize: 13; font.bold: true }
                                Text { text: store.kindLabel(model.kind) + " · " + Theme.dateLabel(model.date)
                                       color: Theme.muted; font.pixelSize: 11 } }
                            Text { text: model.body; color: "#374151"; font.pixelSize: 12
                                   width: parent.width; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight } }
                        MouseArea { id: nrHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: { store.selectPatient(model.patientId); root.switchToPatients() } }
                    }
                }
            }
            Item { width: 1; height: 8 }
        }
    }
}
