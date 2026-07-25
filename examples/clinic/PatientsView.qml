import QtQuick 2.15
import QtQuick.Controls 2.15
import ClinicApp 1.0    // Clinic.Active / Clinic.Cancelled

// Left: searchable directory. Right: the selected patient's chart.
Item {
    id: root
    property int currentTab: 0
    readonly property int myIndex: 1

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
            border.color: patientSearch.activeFocus ? Theme.teal : "transparent"
            Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                   text: "⌕"; color: Theme.muted; font.pixelSize: 20 }
            TextField { id: patientSearch
                anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                placeholderText: "Search name or MRN…"; background: null; color: Theme.ink; font.pixelSize: 14
                onTextChanged: searchDebounce.restart() }
            Timer { id: searchDebounce; interval: 90; onTriggered: store.search(patientSearch.text) }
        }
        Text { anchors { left: parent.left; leftMargin: 18; top: searchBox.bottom; topMargin: 8 }
               text: store.patients.count + " patients"; color: Theme.muted; font.pixelSize: 12 }
        Rectangle {
            anchors { right: parent.right; rightMargin: 16; top: searchBox.bottom; topMargin: 2 }
            width: npT.width + 22; height: 26; radius: 13
            color: npMa.pressed ? Qt.darker(Theme.teal, 1.1) : (npMa.containsMouse ? Qt.lighter(Theme.teal, 1.08) : Theme.teal)
            Behavior on color { ColorAnimation { duration: 130 } }
            Text { id: npT; anchors.centerIn: parent; text: "＋ New"; color: "white"; font.pixelSize: 11; font.bold: true }
            MouseArea { id: npMa; anchors.fill: parent; hoverEnabled: true; onClicked: addPatientDialog.open() }
        }

        ListView {
            id: patientList
            anchors { left: parent.left; right: parent.right; top: searchBox.bottom; bottom: parent.bottom; topMargin: 28 }
            clip: true; model: store.patients
            populate: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                NumberAnimation { property: "x"; from: -24; to: 0; duration: 300; easing.type: Easing.OutCubic } }
            displaced: Transition { NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic } }
            delegate: PatientListItem {}
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
        }
    }

    // ---- chart ----
    Flickable {
        anchors { left: directory.right; right: parent.right; top: parent.top; bottom: parent.bottom }
        contentHeight: chart.height + 40; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Connections { target: store; function onChartChanged() { chartIn.restart() } }
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
                Row {
                    anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                    spacing: 20
                    Rectangle { width: 76; height: 76; radius: 38; color: Theme.avatarColor(store.selectedId)
                                anchors.verticalCenter: parent.verticalCenter
                                Text { anchors.centerIn: parent
                                       text: Theme.initials(store.patient.firstName, store.patient.lastName)
                                       color: "white"; font.pixelSize: 28; font.bold: true } }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 6
                        Text { text: store.patient.firstName + " " + store.patient.lastName
                               color: Theme.ink; font.pixelSize: 26; font.bold: true }
                        Text { text: store.patient.mrn + "   ·   " + store.ageOf(store.patient.dob) + "y   ·   "
                                     + store.patient.sex + "   ·   DOB " + store.patient.dob
                               color: Theme.muted; font.pixelSize: 14 }
                        Row { spacing: 8
                            Text { text: "☎ " + store.patient.phone; color: Theme.muted; font.pixelSize: 13 }
                            Text { text: "·  Blood " + store.patient.bloodType; color: Theme.muted; font.pixelSize: 13 } }
                    }
                }
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

            // quick actions
            Row {
                spacing: 10
                Repeater {
                    model: [ { t: "＋ Add vitals", d: "vital" }, { t: "＋ Add note", d: "note" } ]
                    Rectangle {
                        width: qaT.width + 28; height: 34; radius: 10
                        color: qaMa.containsMouse ? "#E8F5F5" : "white"; border.color: "#DCE6E6"
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Text { id: qaT; anchors.centerIn: parent; text: modelData.t; color: Theme.teal; font.pixelSize: 13; font.bold: true }
                        MouseArea { id: qaMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: modelData.d === "vital" ? addVitalDialog.open() : addNoteDialog.open() }
                    }
                }
            }

            // vitals row (reusable VitalCard)
            Row {
                id: vitalsRow
                width: parent.width; spacing: 14
                property real cw: (width - 4 * 14) / 5
                property var lv: store.latestVital
                Repeater {
                    model: [
                        { l: "Blood Pressure", v: vitalsRow.lv.systolic > 0 ? (vitalsRow.lv.systolic + "/" + vitalsRow.lv.diastolic) : "—", u: "mmHg", c: "#3B82F6" },
                        { l: "Heart Rate",     v: vitalsRow.lv.heartRate > 0 ? vitalsRow.lv.heartRate : "—", u: "bpm", c: "#EF4444" },
                        { l: "Temp",           v: vitalsRow.lv.tempC > 0 ? vitalsRow.lv.tempC.toFixed(1) : "—", u: "°C", c: "#F59E0B" },
                        { l: "SpO₂",           v: vitalsRow.lv.spo2 > 0 ? vitalsRow.lv.spo2 : "—", u: "%", c: "#10B981" },
                        { l: "BMI",            v: (vitalsRow.lv.weightKg > 0 && vitalsRow.lv.heightCm > 0)
                                                 ? (vitalsRow.lv.weightKg / Math.pow(vitalsRow.lv.heightCm/100, 2)).toFixed(1) : "—", u: "kg/m²", c: "#8B5CF6" }
                    ]
                    VitalCard { width: vitalsRow.cw; label: modelData.l; value: modelData.v; unit: modelData.u; accent: modelData.c }
                }
            }

            // two columns of panels
            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                Column {
                    width: parent.cw; spacing: 18
                    SectionCard { width: parent.width; title: "Problem List"; bodySpacing: 10
                        Repeater { model: store.problems
                            Item { width: parent.width; height: 20
                                Rectangle { id: pdot; width: 8; height: 8; radius: 4
                                            color: model.status === Clinic.Active ? "#EF4444" : "#94A3B8"
                                            anchors { left: parent.left; verticalCenter: parent.verticalCenter } }
                                Text { text: model.name; color: Theme.ink; font.pixelSize: 14
                                       anchors { left: pdot.right; leftMargin: 10; right: pst.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                       elide: Text.ElideRight }
                                Text { id: pst; text: model.status === Clinic.Active ? "active" : "resolved"
                                       color: Theme.muted; font.pixelSize: 12
                                       anchors { right: parent.right; verticalCenter: parent.verticalCenter } } } }
                        Text { visible: store.problems.count === 0; text: "No problems recorded"; color: Theme.muted; font.pixelSize: 13 }
                    }
                    SectionCard { width: parent.width; title: "Medications"; bodySpacing: 12
                        Repeater { model: store.medications
                            Row { width: parent.width
                                Column { width: parent.width - 70; spacing: 2
                                    Text { text: model.name; color: Theme.ink; font.pixelSize: 14; font.bold: true }
                                    Text { text: model.dose + " · " + model.frequency; color: Theme.muted; font.pixelSize: 12 } }
                                Rectangle { width: 62; height: 22; radius: 11
                                            color: model.active ? "#EAF7EF" : "#F1F3F8"; anchors.verticalCenter: parent.verticalCenter
                                            Text { anchors.centerIn: parent; text: model.active ? "active" : "inactive"
                                                   color: model.active ? "#1E9E63" : Theme.muted; font.pixelSize: 11; font.bold: true } } } }
                    }
                }

                Column {
                    width: parent.cw; spacing: 18
                    SectionCard { width: parent.width; title: "Appointments"; bodySpacing: 10
                        Repeater { model: store.appointments
                            Rectangle {
                                property bool upcoming: model.day >= store.todayIso() && model.status !== Clinic.Cancelled
                                width: parent.width; height: 46; radius: 10; color: upcoming ? "#F3F9FF" : "#F8F9FB"
                                Rectangle { width: 4; height: parent.height - 16; radius: 2; color: Theme.statusColor(model.status)
                                            anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } }
                                Column { spacing: 2
                                    anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                                    Text { text: model.reason; color: Theme.ink; font.pixelSize: 13; font.bold: true }
                                    Text { text: store.providerName(model.providerId); color: Theme.muted; font.pixelSize: 11 } }
                                Column { spacing: 2
                                    anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                                    Text { text: Theme.dateLabel(model.day, "ddd, MMM d"); color: Theme.ink; font.pixelSize: 12
                                           anchors.right: parent.right }
                                    Text { text: store.minuteLabel(model.minute) + " · " + store.statusLabel(model.status)
                                           color: Theme.statusColor(model.status); font.pixelSize: 11; anchors.right: parent.right } } } }
                    }
                    SectionCard { width: parent.width; title: "Recent Notes"; bodySpacing: 14
                        Repeater { model: store.notes
                            Column { width: parent.width; spacing: 4
                                Row { spacing: 8
                                    Text { text: store.kindLabel(model.kind); color: Theme.teal; font.pixelSize: 12; font.bold: true }
                                    Text { text: Theme.dateLabel(model.date) + " · " + store.providerName(model.providerId)
                                           color: Theme.muted; font.pixelSize: 12 } }
                                Text { text: model.body; color: "#374151"; font.pixelSize: 13; width: parent.width; wrapMode: Text.WordWrap } } }
                    }
                }
            }
            Item { width: 1; height: 8 }
        }
    }

    // dialogs owned by this view
    AddPatientDialog { id: addPatientDialog }
    AddNoteDialog    { id: addNoteDialog }
    AddVitalsDialog  { id: addVitalDialog }
}
