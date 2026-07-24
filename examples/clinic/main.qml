import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 1340; height: 880
    color: "#EEF1F6"
    title: "Qivot Clinic — scheduler & patient chart"

    property int tab: 0                       // 0 = Overview, 1 = Patients, 2 = Schedule
    readonly property color teal:  "#0E8C93"
    readonly property color ink:   "#1F2733"
    readonly property color muted: "#6B7280"

    function statusColor(s) {
        return s === "arrived"   ? "#10B981"
             : s === "completed" ? "#94A3B8"
             : s === "cancelled" ? "#EF4444"
             : "#3B82F6"                        // scheduled
    }
    function providerIndex(id) {
        for (var i = 0; i < store.providers.length; i++)
            if (store.providers[i].id === id) return i
        return 0
    }

    ClinicStore { id: store }

    // ============================= top bar =============================
    Rectangle {
        id: topbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 62; color: "white"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#E2E6EE" }

        Row {
            anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 10
            Rectangle { width: 30; height: 30; radius: 8; color: win.teal
                        anchors.verticalCenter: parent.verticalCenter
                        Text { anchors.centerIn: parent; text: "✚"; color: "white"; font.pixelSize: 18; font.bold: true } }
            Text { text: "Qivot Clinic"; color: win.ink; font.pixelSize: 20; font.bold: true
                   anchors.verticalCenter: parent.verticalCenter }
        }

        // segmented tabs with a sliding highlight
        Rectangle {
            anchors.centerIn: parent
            width: 404; height: 42; radius: 12; color: "#F1F3F8"
            Rectangle {   // the pill that glides between tabs
                width: 124; height: 34; radius: 9; y: 4
                x: 4 + win.tab * 132; color: win.teal
                Behavior on x { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
            }
            Row {
                anchors.fill: parent
                Repeater {
                    model: [ { t: "Overview", i: 0 }, { t: "Patients", i: 1 }, { t: "Schedule", i: 2 } ]
                    Item {
                        width: 132; height: 42
                        Text { anchors.centerIn: parent; text: modelData.t
                               color: win.tab === modelData.i ? "white" : win.muted
                               font.pixelSize: 14; font.bold: true
                               Behavior on color { ColorAnimation { duration: 200 } } }
                        MouseArea { anchors.fill: parent; onClicked: win.tab = modelData.i }
                    }
                }
            }
        }

        Row {
            anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 16
            Text { text: store.scheduleLabel; color: win.muted; font.pixelSize: 14
                   anchors.verticalCenter: parent.verticalCenter }
            Rectangle { width: statChip.width + 24; height: 30; radius: 15; color: "#E8F5F5"
                        anchors.verticalCenter: parent.verticalCenter
                        Text { id: statChip; anchors.centerIn: parent
                               text: store.stats.total + " today · " + store.providers.length + " providers"
                               color: win.teal; font.pixelSize: 13; font.bold: true } }
        }
    }

    // =========================================================================
    //  OVERVIEW / ANALYTICS VIEW  (aggregate queries)
    // =========================================================================
    Item {
        id: ovView
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        opacity: win.tab === 0 ? 1 : 0
        visible: opacity > 0.01
        enabled: win.tab === 0
        transform: Translate { x: win.tab === 0 ? 0 : -28
                               Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
        Behavior on opacity { NumberAnimation { duration: 240 } }

        property var ov: store.overview

        Flickable {
            anchors.fill: parent
            contentHeight: ovCol.height + 48; clip: true
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                id: ovCol
                x: 28; y: 24; width: parent.width - 56; spacing: 18

                Text { text: "Practice overview"; color: win.ink; font.pixelSize: 24; font.bold: true }

                // ---- KPI row ----
                Row {
                    width: parent.width; spacing: 14
                    property real cw: (width - 5 * 14) / 6
                    Repeater {
                        model: [
                            { l: "Patients",        v: (ovView.ov.patients || 0),      c: "#3B82F6" },
                            { l: "Today's visits",  v: (ovView.ov.todayTotal || 0),     c: "#0E8C93" },
                            { l: "Arrived",         v: (ovView.ov.todayArrived || 0),   c: "#10B981" },
                            { l: "Completion",      v: (ovView.ov.completion || 0) + "%", c: "#8B5CF6" },
                            { l: "Active problems", v: (ovView.ov.activeProblems || 0), c: "#EF4444" },
                            { l: "Avg age",         v: (ovView.ov.avgAge || 0),         c: "#F59E0B" }
                        ]
                        Rectangle {
                            width: parent.cw; height: 96; radius: 14; color: "white"
                            Rectangle { width: 34; height: 34; radius: 10; color: modelData.c; opacity: 0.14
                                        anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 16 } }
                            Rectangle { width: 6; height: 6; radius: 3; color: modelData.c
                                        anchors { left: parent.left; leftMargin: 30; top: parent.top; topMargin: 30 } }
                            Column {
                                anchors { left: parent.left; leftMargin: 16; bottom: parent.bottom; bottomMargin: 16 }
                                spacing: 2
                                Text { text: modelData.v; color: win.ink; font.pixelSize: 26; font.bold: true }
                                Text { text: modelData.l; color: win.muted; font.pixelSize: 12 }
                            }
                        }
                    }
                }

                // ---- charts row ----
                Row {
                    width: parent.width; spacing: 18
                    property real cw: (width - 18) / 2

                    // appointments per day (this week)
                    Rectangle {
                        width: parent.cw; height: 250; radius: 16; color: "white"
                        Text { id: bdTitle; text: "Appointments this week"; color: win.ink
                               font.pixelSize: 16; font.bold: true
                               anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                        Row {
                            anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                                      leftMargin: 20; rightMargin: 20; bottomMargin: 18 }
                            height: 170; spacing: 10
                            Repeater {
                                model: ovView.ov.byDay || []
                                Column {
                                    width: (parent.width - 60) / 7
                                    height: parent.height
                                    Item { width: 1; height: parent.height - bar.height - 34 }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.count; color: win.ink; font.pixelSize: 12; font.bold: true }
                                    Rectangle {
                                        id: bar
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: parent.width - 6
                                        height: Math.max(4, (modelData.count / Math.max(1, ovView.ov.byDayMax)) * 120)
                                        radius: 6
                                        gradient: Gradient {
                                            GradientStop { position: 0; color: "#0E8C93" }
                                            GradientStop { position: 1; color: "#38C0C6" }
                                        }
                                        Behavior on height { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
                                    }
                                    Text { anchors.horizontalCenter: parent.horizontalCenter
                                           text: modelData.label; color: win.muted; font.pixelSize: 11; topPadding: 6 }
                                }
                            }
                        }
                    }

                    // provider load
                    Rectangle {
                        width: parent.cw; height: 250; radius: 16; color: "white"
                        Text { id: plTitle; text: "Provider load this week"; color: win.ink
                               font.pixelSize: 16; font.bold: true
                               anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                        Column {
                            anchors { left: parent.left; right: parent.right; top: plTitle.bottom
                                      leftMargin: 20; rightMargin: 20; topMargin: 14 }
                            spacing: 11
                            Repeater {
                                model: ovView.ov.byProvider || []
                                Row {
                                    width: parent.width; spacing: 10
                                    Text { text: modelData.name; color: win.ink; font.pixelSize: 12
                                           width: 118; elide: Text.ElideRight
                                           anchors.verticalCenter: parent.verticalCenter }
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 16; radius: 8; color: modelData.color
                                        width: Math.max(8, (modelData.count / Math.max(1, ovView.ov.byProviderMax)) * (parent.width - 160))
                                        Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
                                    }
                                    Text { text: modelData.count; color: win.muted; font.pixelSize: 12; font.bold: true
                                           anchors.verticalCenter: parent.verticalCenter }
                                }
                            }
                        }
                    }
                }

                // ---- status + top conditions ----
                Row {
                    width: parent.width; spacing: 18
                    property real cw: (width - 18) / 2

                    // today's status
                    Rectangle {
                        width: parent.cw; height: 150; radius: 16; color: "white"
                        Text { id: stTitle; text: "Today's status"; color: win.ink; font.pixelSize: 16; font.bold: true
                               anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                        Row {
                            anchors { left: parent.left; top: stTitle.bottom; leftMargin: 20; topMargin: 16 }
                            spacing: 12
                            Repeater {
                                model: [ { l: "Scheduled", k: "scheduled", c: "#3B82F6" },
                                         { l: "Arrived",   k: "arrived",   c: "#10B981" },
                                         { l: "Completed", k: "completed", c: "#94A3B8" },
                                         { l: "Cancelled", k: "cancelled", c: "#EF4444" } ]
                                Rectangle {
                                    width: 96; height: 68; radius: 12; color: Qt.rgba(0,0,0,0.02)
                                    border.color: "#EEF1F6"
                                    Column { anchors.centerIn: parent; spacing: 3
                                        Text { anchors.horizontalCenter: parent.horizontalCenter
                                               text: (ovView.ov.byStatus ? (ovView.ov.byStatus[modelData.k] || 0) : 0)
                                               color: modelData.c; font.pixelSize: 24; font.bold: true }
                                        Text { anchors.horizontalCenter: parent.horizontalCenter
                                               text: modelData.l; color: win.muted; font.pixelSize: 11 } }
                                }
                            }
                        }
                    }

                    // top active conditions
                    Rectangle {
                        width: parent.cw; height: 150; radius: 16; color: "white"
                        Text { id: tcTitle; text: "Top active conditions"; color: win.ink; font.pixelSize: 16; font.bold: true
                               anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                        Column {
                            anchors { left: parent.left; right: parent.right; top: tcTitle.bottom
                                      leftMargin: 20; rightMargin: 20; topMargin: 10 }
                            spacing: 7
                            Repeater {
                                model: ovView.ov.topConditions || []
                                Item {
                                    width: parent.width; height: 18
                                    Text { text: modelData.name; color: win.ink; font.pixelSize: 13
                                           anchors { left: parent.left; right: tcCount.left; rightMargin: 10
                                                     verticalCenter: parent.verticalCenter }
                                           elide: Text.ElideRight }
                                    Text { id: tcCount; text: modelData.count; color: win.teal
                                           font.pixelSize: 13; font.bold: true
                                           anchors { right: parent.right; verticalCenter: parent.verticalCenter } }
                                }
                            }
                        }
                    }
                }

                // ---- full-text note search ----
                Rectangle {
                    width: parent.width; radius: 16; color: "white"
                    height: nsCol.height + 36
                    Column {
                        id: nsCol
                        anchors { left: parent.left; right: parent.right; top: parent.top
                                  leftMargin: 20; rightMargin: 20; topMargin: 18 }
                        spacing: 12
                        Row { spacing: 10; width: parent.width
                            Text { text: "Search clinical notes"; color: win.ink; font.pixelSize: 16; font.bold: true
                                   anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "FTS5 full-text"; color: win.muted; font.pixelSize: 11
                                   anchors.verticalCenter: parent.verticalCenter } }
                        Rectangle {
                            width: parent.width; height: 42; radius: 10; color: "#F1F3F8"
                            border.color: noteSearch.activeFocus ? win.teal : "transparent"
                            Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                                   text: "⌕"; color: win.muted; font.pixelSize: 20 }
                            TextField { id: noteSearch
                                anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10
                                          verticalCenter: parent.verticalCenter }
                                placeholderText: "e.g. cholesterol, asthma, physical therapy…"
                                background: null; color: win.ink; font.pixelSize: 14
                                onTextChanged: noteDebounce.restart() }
                            Timer { id: noteDebounce; interval: 110; onTriggered: store.searchNotes(noteSearch.text) }
                        }
                        Text { visible: store.noteQuery.length > 0
                               text: store.noteResults.length + " matching notes"
                               color: win.muted; font.pixelSize: 12 }
                        Column {
                            width: parent.width; spacing: 8
                            Repeater {
                                model: store.noteResults
                                Rectangle {
                                    width: parent.width; height: nrCol.height + 20; radius: 10
                                    color: nrHover.containsMouse ? "#F3F9FF" : "#F8F9FB"
                                    Column {
                                        id: nrCol
                                        anchors { left: parent.left; right: parent.right; top: parent.top
                                                  leftMargin: 14; rightMargin: 14; topMargin: 10 }
                                        spacing: 3
                                        Row { spacing: 8
                                            Text { text: modelData.patientName; color: win.teal
                                                   font.pixelSize: 13; font.bold: true }
                                            Text { text: modelData.kind + " · " + modelData.dateLabel
                                                   color: win.muted; font.pixelSize: 11 } }
                                        Text { text: modelData.body; color: "#374151"; font.pixelSize: 12
                                               width: parent.width; wrapMode: Text.WordWrap; maximumLineCount: 2
                                               elide: Text.ElideRight }
                                    }
                                    MouseArea { id: nrHover; anchors.fill: parent; hoverEnabled: true
                                        onClicked: { store.selectPatient(modelData.patientId); win.tab = 1 } }
                                }
                            }
                        }
                    }
                }
                Item { width: 1; height: 8 }
            }
        }
    }

    // =========================================================================
    //  PATIENTS VIEW
    // =========================================================================
    Item {
        id: patientsView
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        opacity: win.tab === 1 ? 1 : 0
        visible: opacity > 0.01
        enabled: win.tab === 1
        transform: Translate { x: win.tab === 1 ? 0 : (win.tab === 0 ? 28 : -28)
                               Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
        Behavior on opacity { NumberAnimation { duration: 240 } }

        // ---- left: directory ----
        Rectangle {
            id: directory
            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
            width: 340; color: "white"
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: "#E2E6EE" }

            // search
            Rectangle {
                id: searchBox
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                height: 42; radius: 10; color: "#F1F3F8"
                border.color: patientSearch.activeFocus ? win.teal : "transparent"
                Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                       text: "⌕"; color: win.muted; font.pixelSize: 20 }
                TextField {
                    id: patientSearch
                    anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10
                              verticalCenter: parent.verticalCenter }
                    placeholderText: "Search name or MRN…"
                    background: null; color: win.ink; font.pixelSize: 14
                    onTextChanged: searchDebounce.restart()
                }
                Timer { id: searchDebounce; interval: 90; onTriggered: store.search(patientSearch.text) }
            }

            Text { anchors { left: parent.left; leftMargin: 18; top: searchBox.bottom; topMargin: 8 }
                   text: store.patients.length + " patients"; color: win.muted; font.pixelSize: 12 }
            Rectangle {
                anchors { right: parent.right; rightMargin: 16; top: searchBox.bottom; topMargin: 2 }
                width: npT.width + 22; height: 26; radius: 13
                color: npMa.pressed ? Qt.darker(win.teal, 1.1) : (npMa.containsMouse ? Qt.lighter(win.teal, 1.08) : win.teal)
                Behavior on color { ColorAnimation { duration: 130 } }
                Text { id: npT; anchors.centerIn: parent; text: "＋ New"; color: "white"; font.pixelSize: 11; font.bold: true }
                MouseArea { id: npMa; anchors.fill: parent; hoverEnabled: true; onClicked: newPatientDialog.open() }
            }

            ListView {
                id: patientList
                anchors { left: parent.left; right: parent.right; top: searchBox.bottom; bottom: parent.bottom
                          topMargin: 28 }
                clip: true; model: store.patients
                // staggered cascade when the list (re)loads or search filters
                populate: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                    NumberAnimation { property: "x"; from: -24; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
                displaced: Transition { NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic } }
                delegate: Rectangle {
                    width: patientList.width; height: 64
                    color: modelData.id === store.selectedId ? "#EAF6F6" : (rowHover.containsMouse ? "#F6F8FB" : "white")
                    Behavior on color { ColorAnimation { duration: 160 } }
                    Rectangle { height: parent.height; color: win.teal
                                width: modelData.id === store.selectedId ? 3 : 0
                                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } } }
                    Rectangle {
                        id: av
                        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        width: 40; height: 40; radius: 20; color: modelData.color
                        scale: rowHover.containsMouse ? 1.08 : 1.0
                        Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutBack } }
                        Text { anchors.centerIn: parent; text: modelData.initials; color: "white"
                               font.pixelSize: 15; font.bold: true }
                    }
                    Column {
                        anchors { left: av.right; leftMargin: 12; right: parent.right; rightMargin: 12
                                  verticalCenter: parent.verticalCenter }
                        spacing: 2
                        Text { text: modelData.name; color: win.ink; font.pixelSize: 15; font.bold: true
                               width: parent.width; elide: Text.ElideRight }
                        Text { text: modelData.mrn + "  ·  " + modelData.age + "y  ·  " + modelData.sex
                               color: win.muted; font.pixelSize: 12 }
                    }
                    Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.leftMargin: 16
                                width: parent.width - 16; height: 1; color: "#EEF1F6" }
                    MouseArea { id: rowHover; anchors.fill: parent; hoverEnabled: true
                                onClicked: store.selectPatient(modelData.id) }
                }
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
            }
        }

        // ---- right: chart ----
        Flickable {
            anchors { left: directory.right; right: parent.right; top: parent.top; bottom: parent.bottom }
            contentHeight: chart.height + 40; clip: true
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            // Replay a fade+rise every time a new patient chart loads.
            Connections { target: store; function onChartChanged() { chartIn.restart() } }
            ParallelAnimation {
                id: chartIn; running: true
                NumberAnimation { target: chart; property: "opacity"; from: 0; to: 1; duration: 300 }
                NumberAnimation { target: chartRise; property: "y"; from: 22; to: 0
                                  duration: 380; easing.type: Easing.OutCubic }
            }

            Column {
                id: chart
                x: 24; y: 24; width: parent.width - 48; spacing: 18
                opacity: 0
                transform: Translate { id: chartRise; y: 22 }

                // ---- header card ----
                Rectangle {
                    width: parent.width; height: 128; radius: 16; color: "white"
                    Row {
                        anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                        spacing: 20
                        Rectangle { width: 76; height: 76; radius: 38; color: store.patient.color || "#ccc"
                                    anchors.verticalCenter: parent.verticalCenter
                                    Text { anchors.centerIn: parent; text: store.patient.initials || ""
                                           color: "white"; font.pixelSize: 28; font.bold: true } }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter; spacing: 6
                            Text { text: store.patient.name || ""; color: win.ink; font.pixelSize: 26; font.bold: true }
                            Text { text: (store.patient.mrn || "") + "   ·   " + (store.patient.age || "") + "y   ·   "
                                         + (store.patient.sex || "") + "   ·   DOB " + (store.patient.dob || "")
                                   color: win.muted; font.pixelSize: 14 }
                            Row {
                                spacing: 8
                                Text { text: "☎ " + (store.patient.phone || ""); color: win.muted; font.pixelSize: 13 }
                                Text { text: "·  Blood " + (store.patient.bloodType || ""); color: win.muted; font.pixelSize: 13 }
                            }
                        }
                    }
                    // allergy banner
                    Rectangle {
                        anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
                        height: 34; radius: 17; width: alg.width + 30
                        property bool has: (store.patient.allergies || "") !== ""
                        color: has ? "#FDECEC" : "#EAF7EF"
                        Text { id: alg; anchors.centerIn: parent
                               text: parent.has ? ("⚠  Allergy: " + store.patient.allergies) : "✓  No known allergies"
                               color: parent.has ? "#D14343" : "#1E9E63"; font.pixelSize: 13; font.bold: true }
                    }
                }

                // ---- quick actions ----
                Row {
                    spacing: 10
                    Repeater {
                        model: [ { t: "＋ Add vitals", d: "vital" }, { t: "＋ Add note", d: "note" } ]
                        Rectangle {
                            width: qaT.width + 28; height: 34; radius: 10
                            color: qaMa.containsMouse ? "#E8F5F5" : "white"; border.color: "#DCE6E6"
                            Behavior on color { ColorAnimation { duration: 130 } }
                            Text { id: qaT; anchors.centerIn: parent; text: modelData.t
                                   color: win.teal; font.pixelSize: 13; font.bold: true }
                            MouseArea { id: qaMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: modelData.d === "vital" ? addVitalDialog.open() : addNoteDialog.open() }
                        }
                    }
                }

                // ---- vitals row ----
                Row {
                    width: parent.width; spacing: 14
                    property real cw: (width - 4 * 14) / 5
                    Repeater {
                        model: [
                            { l: "Blood Pressure", v: store.patient.bp || "—", u: "mmHg", c: "#3B82F6" },
                            { l: "Heart Rate",     v: (store.patient.hr || "—"), u: "bpm", c: "#EF4444" },
                            { l: "Temp",           v: (store.patient.tempC || "—"), u: "°C", c: "#F59E0B" },
                            { l: "SpO₂",           v: (store.patient.spo2 || "—"), u: "%", c: "#10B981" },
                            { l: "BMI",            v: (store.patient.bmi || "—"), u: "kg/m²", c: "#8B5CF6" }
                        ]
                        Rectangle {
                            width: parent.cw; height: 92; radius: 14; color: "white"
                            Rectangle { width: 4; height: 30; radius: 2; color: modelData.c
                                        anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 16 } }
                            Column {
                                anchors { left: parent.left; leftMargin: 30; top: parent.top; topMargin: 14 }
                                spacing: 3
                                Text { text: modelData.l; color: win.muted; font.pixelSize: 11 }
                                Row { spacing: 4
                                    Text { text: modelData.v; color: win.ink; font.pixelSize: 24; font.bold: true }
                                    Text { text: modelData.u; color: "#9AA1AD"; font.pixelSize: 11
                                           anchors.bottom: parent.bottom; anchors.bottomMargin: 4 } }
                            }
                        }
                    }
                }

                // ---- two columns of cards ----
                Row {
                    width: parent.width; spacing: 18
                    property real cw: (width - 18) / 2

                    // left column
                    Column {
                        width: parent.cw; spacing: 18

                        // problems
                        Rectangle {
                            width: parent.width; radius: 16; color: "white"
                            height: probCol.height + 52
                            Text { id: probTitle; text: "Problem List"; color: win.ink; font.pixelSize: 16; font.bold: true
                                   anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                            Column {
                                id: probCol
                                anchors { left: parent.left; right: parent.right; top: probTitle.bottom
                                          leftMargin: 20; rightMargin: 20; topMargin: 12 }
                                spacing: 10
                                Repeater {
                                    model: store.problems
                                    Item {
                                        width: parent.width; height: 20
                                        Rectangle { id: pdot; width: 8; height: 8; radius: 4
                                                    color: modelData.status === "active" ? "#EF4444" : "#94A3B8"
                                                    anchors { left: parent.left; verticalCenter: parent.verticalCenter } }
                                        Text { text: modelData.name; color: win.ink; font.pixelSize: 14
                                               anchors { left: pdot.right; leftMargin: 10; right: pstatus.left; rightMargin: 10
                                                         verticalCenter: parent.verticalCenter }
                                               elide: Text.ElideRight }
                                        Text { id: pstatus; text: modelData.status; color: win.muted; font.pixelSize: 12
                                               anchors { right: parent.right; verticalCenter: parent.verticalCenter } }
                                    }
                                }
                                Text { visible: store.problems.length === 0; text: "No problems recorded"
                                       color: win.muted; font.pixelSize: 13 }
                            }
                        }

                        // medications
                        Rectangle {
                            width: parent.width; radius: 16; color: "white"
                            height: medCol.height + 52
                            Text { id: medTitle; text: "Medications"; color: win.ink; font.pixelSize: 16; font.bold: true
                                   anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                            Column {
                                id: medCol
                                anchors { left: parent.left; right: parent.right; top: medTitle.bottom
                                          leftMargin: 20; rightMargin: 20; topMargin: 12 }
                                spacing: 12
                                Repeater {
                                    model: store.medications
                                    Row {
                                        width: parent.width
                                        Column {
                                            width: parent.width - 70; spacing: 2
                                            Text { text: modelData.name; color: win.ink; font.pixelSize: 14; font.bold: true }
                                            Text { text: modelData.dose + " · " + modelData.frequency
                                                   color: win.muted; font.pixelSize: 12 }
                                        }
                                        Rectangle { width: 62; height: 22; radius: 11
                                                    color: modelData.active ? "#EAF7EF" : "#F1F3F8"
                                                    anchors.verticalCenter: parent.verticalCenter
                                                    Text { anchors.centerIn: parent
                                                           text: modelData.active ? "active" : "inactive"
                                                           color: modelData.active ? "#1E9E63" : win.muted
                                                           font.pixelSize: 11; font.bold: true } }
                                    }
                                }
                            }
                        }
                    }

                    // right column
                    Column {
                        width: parent.cw; spacing: 18

                        // appointments
                        Rectangle {
                            width: parent.width; radius: 16; color: "white"
                            height: apptCol.height + 52
                            Text { id: apptTitle; text: "Appointments"; color: win.ink; font.pixelSize: 16; font.bold: true
                                   anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                            Column {
                                id: apptCol
                                anchors { left: parent.left; right: parent.right; top: apptTitle.bottom
                                          leftMargin: 20; rightMargin: 20; topMargin: 12 }
                                spacing: 10
                                Repeater {
                                    model: store.appointments
                                    Rectangle {
                                        width: parent.width; height: 46; radius: 10
                                        color: modelData.upcoming ? "#F3F9FF" : "#F8F9FB"
                                        Rectangle { width: 4; height: parent.height - 16; radius: 2
                                                    color: win.statusColor(modelData.status)
                                                    anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } }
                                        Column {
                                            anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                                            spacing: 2
                                            Text { text: modelData.reason; color: win.ink; font.pixelSize: 13; font.bold: true }
                                            Text { text: modelData.providerName; color: win.muted; font.pixelSize: 11 }
                                        }
                                        Column {
                                            anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                                            spacing: 2
                                            Text { text: modelData.dateLabel; color: win.ink; font.pixelSize: 12
                                                   horizontalAlignment: Text.AlignRight; width: contentWidth
                                                   anchors.right: parent.right }
                                            Text { text: modelData.timeLabel + " · " + modelData.status
                                                   color: win.statusColor(modelData.status); font.pixelSize: 11
                                                   anchors.right: parent.right }
                                        }
                                    }
                                }
                            }
                        }

                        // notes
                        Rectangle {
                            width: parent.width; radius: 16; color: "white"
                            height: noteCol.height + 52
                            Text { id: noteTitle; text: "Recent Notes"; color: win.ink; font.pixelSize: 16; font.bold: true
                                   anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 } }
                            Column {
                                id: noteCol
                                anchors { left: parent.left; right: parent.right; top: noteTitle.bottom
                                          leftMargin: 20; rightMargin: 20; topMargin: 12 }
                                spacing: 14
                                Repeater {
                                    model: store.notes
                                    Column {
                                        width: parent.width; spacing: 4
                                        Row { spacing: 8
                                            Text { text: modelData.kind; color: win.teal; font.pixelSize: 12; font.bold: true }
                                            Text { text: modelData.dateLabel + " · " + modelData.providerName
                                                   color: win.muted; font.pixelSize: 12 } }
                                        Text { text: modelData.body; color: "#374151"; font.pixelSize: 13
                                               width: parent.width; wrapMode: Text.WordWrap }
                                    }
                                }
                            }
                        }
                    }
                }
                Item { width: 1; height: 8 }
            }
        }
    }

    // =========================================================================
    //  SCHEDULE VIEW
    // =========================================================================
    Item {
        anchors { left: parent.left; right: parent.right; top: topbar.bottom; bottom: parent.bottom }
        opacity: win.tab === 2 ? 1 : 0
        visible: opacity > 0.01
        enabled: win.tab === 2
        transform: Translate { x: win.tab === 2 ? 0 : 28
                               Behavior on x { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } } }
        Behavior on opacity { NumberAnimation { duration: 240 } }

        // toolbar
        Item {
            id: schedBar
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: 64
            Row {
                anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                spacing: 8
                Rectangle { width: 34; height: 34; radius: 8; color: "white"; border.color: "#E2E6EE"
                            Text { anchors.centerIn: parent; text: "‹"; color: win.ink; font.pixelSize: 20 }
                            MouseArea { anchors.fill: parent; onClicked: store.shiftDay(-1) } }
                Rectangle { width: 74; height: 34; radius: 8
                            color: store.isToday ? win.teal : "white"; border.color: "#E2E6EE"
                            Text { anchors.centerIn: parent; text: "Today"
                                   color: store.isToday ? "white" : win.ink; font.pixelSize: 13; font.bold: true }
                            MouseArea { anchors.fill: parent; onClicked: store.goToday() } }
                Rectangle { width: 34; height: 34; radius: 8; color: "white"; border.color: "#E2E6EE"
                            Text { anchors.centerIn: parent; text: "›"; color: win.ink; font.pixelSize: 20 }
                            MouseArea { anchors.fill: parent; onClicked: store.shiftDay(1) } }
                Text { text: store.scheduleLabel; color: win.ink; font.pixelSize: 18; font.bold: true
                       anchors.verticalCenter: parent.verticalCenter; leftPadding: 8 }
            }
            Row {
                anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
                spacing: 18
                Row { spacing: 14; anchors.verticalCenter: parent.verticalCenter
                    Repeater {
                        model: [ { l: "booked", v: store.stats.total, c: "#3B82F6" },
                                 { l: "arrived", v: store.stats.arrived, c: "#10B981" },
                                 { l: "done", v: store.stats.completed, c: "#94A3B8" } ]
                        Row { spacing: 6
                            Rectangle { width: 9; height: 9; radius: 4.5; color: modelData.c
                                        anchors.verticalCenter: parent.verticalCenter }
                            Text { text: modelData.v + " " + modelData.l; color: win.muted; font.pixelSize: 13
                                   anchors.verticalCenter: parent.verticalCenter } }
                    }
                }
                Rectangle { width: bkT.width + 34; height: 38; radius: 10
                            color: naMa.pressed ? Qt.darker(win.teal, 1.15) : (naMa.containsMouse ? Qt.lighter(win.teal, 1.08) : win.teal)
                            anchors.verticalCenter: parent.verticalCenter
                            scale: naMa.pressed ? 0.96 : 1.0
                            Behavior on scale { NumberAnimation { duration: 100 } }
                            Behavior on color { ColorAnimation { duration: 140 } }
                            Text { id: bkT; anchors.centerIn: parent; text: "＋  New appointment"
                                   color: "white"; font.pixelSize: 14; font.bold: true }
                            MouseArea { id: naMa; anchors.fill: parent; hoverEnabled: true; onClicked: bookDialog.open() } }
            }
        }

        // calendar
        Rectangle {
            id: cal
            anchors { left: parent.left; right: parent.right; top: schedBar.bottom; bottom: parent.bottom
                      margins: 20; topMargin: 0 }
            radius: 16; color: "white"; clip: true

            property int dayStart: 480
            property int dayEnd: 1080
            property real pxPerMin: 1.15
            property int gutter: 60
            property real colW: (width - gutter) / Math.max(1, store.providers.length)

            // provider headers
            Rectangle {
                id: calHead
                anchors { left: parent.left; right: parent.right; top: parent.top }
                height: 52; color: "#FAFBFD"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#E9ECF2" }
                Repeater {
                    model: store.providers
                    Column {
                        x: cal.gutter + index * cal.colW; width: cal.colW; height: parent.height
                        Item { width: 1; height: 8 }
                        Text { text: modelData.name; color: win.ink; font.pixelSize: 13; font.bold: true
                               width: parent.width - 12; anchors.horizontalCenter: parent.horizontalCenter
                               horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: modelData.specialty; color: win.muted; font.pixelSize: 11
                               width: parent.width; horizontalAlignment: Text.AlignHCenter }
                        Rectangle { width: 28; height: 3; radius: 2; color: modelData.color
                                    anchors.horizontalCenter: parent.horizontalCenter }
                    }
                }
            }

            Flickable {
                anchors { left: parent.left; right: parent.right; top: calHead.bottom; bottom: parent.bottom }
                contentHeight: (cal.dayEnd - cal.dayStart) * cal.pxPerMin + 20
                clip: true
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                Item {
                    width: parent.width
                    height: (cal.dayEnd - cal.dayStart) * cal.pxPerMin + 20

                    // hour lines + labels
                    Repeater {
                        model: (cal.dayEnd - cal.dayStart) / 60 + 1
                        Item {
                            y: index * 60 * cal.pxPerMin + 8
                            width: cal.width
                            Text { x: 12; y: -7; text: store.minuteLabel((cal.dayStart + index * 60))
                                   color: win.muted; font.pixelSize: 11 }
                            Rectangle { x: cal.gutter; width: cal.width - cal.gutter; height: 1; color: "#EEF1F6" }
                        }
                    }
                    // provider column separators
                    Repeater {
                        model: store.providers.length
                        Rectangle { x: cal.gutter + index * cal.colW; y: 0; width: 1
                                    height: parent.height; color: "#F2F4F8" }
                    }

                    // appointment blocks
                    Repeater {
                        model: store.schedule
                        Rectangle {
                            id: apptBlock
                            visible: modelData.status !== "cancelled"
                            x: cal.gutter + win.providerIndex(modelData.providerId) * cal.colW + 3
                            y: (modelData.minute - cal.dayStart) * cal.pxPerMin + 8
                            width: cal.colW - 6
                            height: Math.max(modelData.durationMin * cal.pxPerMin - 3, 26)
                            radius: 8
                            color: Qt.rgba(0,0,0,0)
                            border.width: apptMa.containsMouse ? 2 : 0
                            border.color: win.statusColor(modelData.status)
                            z: apptMa.containsMouse ? 5 : 1
                            scale: apptMa.containsMouse ? 1.03 : 1.0
                            Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
                            opacity: 0
                            Component.onCompleted: apptIn.start()
                            SequentialAnimation {
                                id: apptIn
                                PauseAnimation { duration: Math.min(index * 14, 320) }
                                NumberAnimation { target: apptBlock; property: "opacity"; to: 1
                                                  duration: 260; easing.type: Easing.OutCubic }
                            }
                            Rectangle {
                                anchors.fill: parent; radius: 8
                                color: win.statusColor(modelData.status)
                                opacity: modelData.status === "completed" ? 0.28 : 0.16
                            }
                            Rectangle { width: 3; height: parent.height - 10; radius: 2
                                        color: win.statusColor(modelData.status)
                                        anchors { left: parent.left; leftMargin: 5; verticalCenter: parent.verticalCenter } }
                            Column {
                                anchors { left: parent.left; leftMargin: 14; right: parent.right; rightMargin: 6
                                          top: parent.top; topMargin: 5 }
                                spacing: 1
                                Text { text: modelData.timeLabel + "  " + modelData.patientName
                                       color: win.ink; font.pixelSize: 12; font.bold: true
                                       width: parent.width; elide: Text.ElideRight }
                                Text { text: modelData.reason; color: "#4B5563"; font.pixelSize: 11
                                       width: parent.width; elide: Text.ElideRight
                                       visible: parent.parent.height > 34 }
                            }
                            MouseArea { id: apptMa; anchors.fill: parent; hoverEnabled: true
                                onClicked: { actionPopup.appt = modelData; actionPopup.open() } }
                        }
                    }
                }
            }
        }
    }

    // ===================== appointment action popup =====================
    Popup {
        id: actionPopup
        property var appt: ({})
        anchors.centerIn: Overlay.overlay
        width: 300; padding: 0; modal: true
        background: Rectangle { radius: 16; color: "white"; border.color: "#E2E6EE" }
        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.88; to: 1; duration: 240; easing.type: Easing.OutBack }
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; from: 1; to: 0.92; duration: 130 }
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130 }
        }
        Overlay.modal: Rectangle { color: "#33101828" }
        Column {
            width: parent.width
            Rectangle { width: parent.width; height: 74; color: "#FAFBFD"
                radius: 16
                Column { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                    spacing: 3
                    Text { text: actionPopup.appt.patientName || ""; color: win.ink; font.pixelSize: 17; font.bold: true }
                    Text { text: (actionPopup.appt.timeLabel || "") + " · " + (actionPopup.appt.reason || "")
                           color: win.muted; font.pixelSize: 12 } }
            }
            Column {
                width: parent.width; padding: 16; spacing: 8
                Repeater {
                    model: [ { l: "Check in (arrived)", s: "arrived", c: "#10B981" },
                             { l: "Mark completed", s: "completed", c: "#0E8C93" },
                             { l: "Cancel appointment", s: "cancelled", c: "#EF4444" } ]
                    Rectangle {
                        width: parent.width - 32; height: 44; radius: 10
                        color: btnHover.containsMouse ? Qt.lighter(modelData.c, 1.9) : "#F6F8FB"
                        Text { anchors.centerIn: parent; text: modelData.l; color: modelData.c
                               font.pixelSize: 14; font.bold: true }
                        MouseArea { id: btnHover; anchors.fill: parent; hoverEnabled: true
                            onClicked: { store.setStatus(actionPopup.appt.id, modelData.s); actionPopup.close() } }
                    }
                }
            }
        }
    }

    // ===================== booking dialog =====================
    Popup {
        id: bookDialog
        anchors.centerIn: Overlay.overlay
        width: 420; padding: 0; modal: true
        onOpened: bookErr.text = ""
        background: Rectangle { radius: 16; color: "white"; border.color: "#E2E6EE" }
        enter: Transition {
            NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 240; easing.type: Easing.OutBack }
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 }
        }
        exit: Transition {
            NumberAnimation { property: "scale"; from: 1; to: 0.92; duration: 130 }
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 130 }
        }
        Overlay.modal: Rectangle { color: "#33101828" }

        ListModel { id: slotModel }
        Component.onCompleted: { for (var m = 480; m <= 1050; m += 15) slotModel.append({ label: store.minuteLabel(m), minute: m }) }

        Column {
            width: parent.width
            Rectangle { width: parent.width; height: 60; color: "#FAFBFD"; radius: 16
                Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                       text: "New appointment — " + store.scheduleLabel; color: win.ink; font.pixelSize: 16; font.bold: true } }
            Column {
                width: parent.width; padding: 20; spacing: 14

                Column { spacing: 6; width: parent.width - 40
                    Text { text: "Patient"; color: win.muted; font.pixelSize: 12 }
                    ComboBox { id: cbPatient; width: parent.width; model: store.patients
                               textRole: "name"; valueRole: "id" } }
                Row { spacing: 14; width: parent.width - 40
                    Column { spacing: 6; width: (parent.width - 14) / 2
                        Text { text: "Provider"; color: win.muted; font.pixelSize: 12 }
                        ComboBox { id: cbProvider; width: parent.width; model: store.providers
                                   textRole: "name"; valueRole: "id" } }
                    Column { spacing: 6; width: (parent.width - 14) / 2
                        Text { text: "Time"; color: win.muted; font.pixelSize: 12 }
                        ComboBox { id: cbTime; width: parent.width; model: slotModel
                                   textRole: "label"; valueRole: "minute" } }
                }
                Row { spacing: 14; width: parent.width - 40
                    Column { spacing: 6; width: (parent.width - 14) / 2
                        Text { text: "Duration"; color: win.muted; font.pixelSize: 12 }
                        ComboBox { id: cbDur; width: parent.width
                                   model: [ 15, 20, 30, 45, 60 ]; currentIndex: 2 } }
                    Column { spacing: 6; width: (parent.width - 14) / 2
                        Text { text: "Reason"; color: win.muted; font.pixelSize: 12 }
                        TextField { id: tfReason; width: parent.width; placeholderText: "Office Visit"
                                    background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                }

                Text { id: bookErr; color: "#D14343"; font.pixelSize: 12; width: parent.width - 40; wrapMode: Text.WordWrap }

                Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                    Rectangle { width: 90; height: 40; radius: 10; color: "#F1F3F8"
                        Text { anchors.centerIn: parent; text: "Cancel"; color: win.muted; font.pixelSize: 14 }
                        MouseArea { anchors.fill: parent; onClicked: bookDialog.close() } }
                    Rectangle { width: 110; height: 40; radius: 10
                        color: bookMa.pressed ? Qt.darker(win.teal, 1.15) : win.teal
                        scale: bookMa.pressed ? 0.96 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100 } }
                        Behavior on color { ColorAnimation { duration: 140 } }
                        Text { anchors.centerIn: parent; text: "Book"; color: "white"; font.pixelSize: 14; font.bold: true }
                        MouseArea { id: bookMa; anchors.fill: parent; onClicked: {
                            var ok = store.book(cbPatient.currentValue, cbProvider.currentValue,
                                                cbTime.currentValue, cbDur.currentText * 1, tfReason.text)
                            if (ok) { bookDialog.close(); win.tab = 2 }
                            else bookErr.text = store.lastError
                        } } }
                }
            }
        }
    }

    // ===================== add patient =====================
    Popup {
        id: newPatientDialog
        anchors.centerIn: Overlay.overlay
        width: 420; padding: 0; modal: true
        background: Rectangle { radius: 16; color: "white"; border.color: "#E2E6EE" }
        enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
        Overlay.modal: Rectangle { color: "#33101828" }
        Column {
            width: parent.width
            Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
                Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                       text: "New patient"; color: win.ink; font.pixelSize: 16; font.bold: true } }
            Column {
                width: parent.width; padding: 20; spacing: 12
                Row { spacing: 12; width: parent.width - 40
                    Column { spacing: 5; width: (parent.width - 12) / 2
                        Text { text: "First name"; color: win.muted; font.pixelSize: 12 }
                        TextField { id: npFirst; width: parent.width; placeholderText: "Jordan"
                                    background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                    Column { spacing: 5; width: (parent.width - 12) / 2
                        Text { text: "Last name"; color: win.muted; font.pixelSize: 12 }
                        TextField { id: npLast; width: parent.width; placeholderText: "Rivera"
                                    background: Rectangle { radius: 8; color: "#F1F3F8" } } } }
                Row { spacing: 12; width: parent.width - 40
                    Column { spacing: 5; width: (parent.width - 12) / 2
                        Text { text: "Date of birth"; color: win.muted; font.pixelSize: 12 }
                        TextField { id: npDob; width: parent.width; placeholderText: "1990-05-14"
                                    background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                    Column { spacing: 5; width: (parent.width - 12) / 2
                        Text { text: "Sex"; color: win.muted; font.pixelSize: 12 }
                        ComboBox { id: npSex; width: parent.width; model: [ "F", "M" ] } } }
                Column { spacing: 5; width: parent.width - 40
                    Text { text: "Phone"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: npPhone; width: parent.width; placeholderText: "(555) 010-2233"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                    Rectangle { width: 90; height: 40; radius: 10; color: "#F1F3F8"
                        Text { anchors.centerIn: parent; text: "Cancel"; color: win.muted; font.pixelSize: 14 }
                        MouseArea { anchors.fill: parent; onClicked: newPatientDialog.close() } }
                    Rectangle { width: 130; height: 40; radius: 10; color: win.teal
                        Text { anchors.centerIn: parent; text: "Add patient"; color: "white"; font.pixelSize: 14; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: {
                            store.addPatient(npFirst.text, npLast.text, npDob.text, npSex.currentText, npPhone.text)
                            npFirst.text = ""; npLast.text = ""; npDob.text = ""; npPhone.text = ""
                            newPatientDialog.close(); win.tab = 1
                        } } }
                }
            }
        }
    }

    // ===================== add note =====================
    Popup {
        id: addNoteDialog
        anchors.centerIn: Overlay.overlay
        width: 480; padding: 0; modal: true
        background: Rectangle { radius: 16; color: "white"; border.color: "#E2E6EE" }
        enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
        Overlay.modal: Rectangle { color: "#33101828" }
        Column {
            width: parent.width
            Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
                Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                       text: "Add note — " + (store.patient.name || ""); color: win.ink; font.pixelSize: 16; font.bold: true } }
            Column {
                width: parent.width; padding: 20; spacing: 12
                Column { spacing: 5; width: parent.width - 40
                    Text { text: "Type"; color: win.muted; font.pixelSize: 12 }
                    ComboBox { id: anKind; width: parent.width
                               model: [ "Office Visit", "Phone", "Lab Review", "Follow-up" ] } }
                Column { spacing: 5; width: parent.width - 40
                    Text { text: "Note"; color: win.muted; font.pixelSize: 12 }
                    Rectangle { width: parent.width; height: 130; radius: 8; color: "#F1F3F8"
                        TextArea { id: anBody; anchors.fill: parent; anchors.margins: 8
                                   wrapMode: TextArea.Wrap; font.pixelSize: 13; color: win.ink
                                   placeholderText: "Subjective, assessment, plan…"; background: null } } }
                Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                    Rectangle { width: 90; height: 40; radius: 10; color: "#F1F3F8"
                        Text { anchors.centerIn: parent; text: "Cancel"; color: win.muted; font.pixelSize: 14 }
                        MouseArea { anchors.fill: parent; onClicked: addNoteDialog.close() } }
                    Rectangle { width: 110; height: 40; radius: 10
                        color: anBody.text.trim().length ? win.teal : "#B7C2C4"
                        Text { anchors.centerIn: parent; text: "Save"; color: "white"; font.pixelSize: 14; font.bold: true }
                        MouseArea { anchors.fill: parent; onClicked: {
                            if (!anBody.text.trim().length) return
                            store.addNote(store.selectedId, 0, anKind.currentText, anBody.text)
                            anBody.text = ""; addNoteDialog.close()
                        } } }
                }
            }
        }
    }

    // ===================== add vitals =====================
    Popup {
        id: addVitalDialog
        anchors.centerIn: Overlay.overlay
        width: 480; padding: 0; modal: true
        background: Rectangle { radius: 16; color: "white"; border.color: "#E2E6EE" }
        enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
        exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
        Overlay.modal: Rectangle { color: "#33101828" }
        Column {
            width: parent.width
            Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
                Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                       text: "Add vitals — " + (store.patient.name || ""); color: win.ink; font.pixelSize: 16; font.bold: true } }
            Grid {
                x: 20; topPadding: 20; columns: 3
                rowSpacing: 12; columnSpacing: 12
                property real fw: (addVitalDialog.width - 40 - 24) / 3
                Column { spacing: 5; width: parent.fw
                    Text { text: "Systolic"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avSys; width: parent.width; placeholderText: "120"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "Diastolic"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avDia; width: parent.width; placeholderText: "80"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "Heart rate"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avHr; width: parent.width; placeholderText: "72"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "Temp °C"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avTemp; width: parent.width; placeholderText: "36.8"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "SpO₂ %"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avSpo2; width: parent.width; placeholderText: "98"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "Weight kg"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avWt; width: parent.width; placeholderText: "70"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
                Column { spacing: 5; width: parent.fw
                    Text { text: "Height cm"; color: win.muted; font.pixelSize: 12 }
                    TextField { id: avHt; width: parent.width; placeholderText: "170"
                                background: Rectangle { radius: 8; color: "#F1F3F8" } } }
            }
            Row { spacing: 12; anchors.right: parent.right; rightPadding: 20; topPadding: 16; bottomPadding: 20
                Rectangle { width: 90; height: 40; radius: 10; color: "#F1F3F8"
                    Text { anchors.centerIn: parent; text: "Cancel"; color: win.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: addVitalDialog.close() } }
                Rectangle { width: 110; height: 40; radius: 10; color: win.teal
                    Text { anchors.centerIn: parent; text: "Save"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: {
                        store.addVital(store.selectedId,
                            parseInt(avSys.text) || 120, parseInt(avDia.text) || 80, parseInt(avHr.text) || 72,
                            parseFloat(avTemp.text) || 36.8, parseInt(avSpo2.text) || 98,
                            parseFloat(avWt.text) || 70, parseInt(avHt.text) || 170)
                        avSys.text = ""; avDia.text = ""; avHr.text = ""; avTemp.text = ""
                        avSpo2.text = ""; avWt.text = ""; avHt.text = ""
                        addVitalDialog.close()
                    } } }
            }
        }
    }
}
