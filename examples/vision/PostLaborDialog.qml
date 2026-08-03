import QtQuick 2.15
import QtQuick.Controls 2.15

// Posts one row into the LD ledger. Because the ledger is a live QiListModel and
// the reports recompute on postLabor(), every view updates the moment this closes.
Dialog {
    id: dlg
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(440, (Overlay.overlay ? Overlay.overlay.width : 440) - 24)
    title: "Post labor entry"

    background: Rectangle { radius: 14; color: Theme.card; border.width: 1; border.color: Theme.border }

    onOpened: {
        hoursField.text = "8";
        dateField.text = "2026-06-30";
        projCombo.currentIndex = 0;
        empCombo.currentIndex = 0;
    }

    contentItem: Column {
        spacing: 12
        width: parent ? parent.width : 400

        Label { text: "Project (WBS1)"; color: Theme.muted; font.pixelSize: 11; font.bold: true }
        ComboBox {
            id: projCombo; width: parent.width
            model: store.projectOptions; textRole: "name"
        }

        Label { text: "Employee"; color: Theme.muted; font.pixelSize: 11; font.bold: true }
        ComboBox {
            id: empCombo; width: parent.width
            model: store.staffOptions; textRole: "name"
        }

        Row {
            width: parent.width; spacing: 12
            Column {
                width: (parent.width - 12) / 2; spacing: 4
                Label { text: "Date"; color: Theme.muted; font.pixelSize: 11; font.bold: true }
                TextField { id: dateField; width: parent.width; text: "2026-06-30"; selectByMouse: true }
            }
            Column {
                width: (parent.width - 12) / 2; spacing: 4
                Label { text: "Hours"; color: Theme.muted; font.pixelSize: 11; font.bold: true }
                TextField { id: hoursField; width: parent.width; text: "8"; selectByMouse: true
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            validator: DoubleValidator { bottom: 0; top: 9999 } }
            }
        }

        // rate preview
        Text {
            width: parent.width; wrapMode: Text.WordWrap; color: Theme.muted; font.pixelSize: 11
            property var emp: (empCombo.currentIndex >= 0 && store.staffOptions.length) ? store.staffOptions[empCombo.currentIndex] : null
            property var proj: (projCombo.currentIndex >= 0 && store.projectOptions.length) ? store.projectOptions[projCombo.currentIndex] : null
            property bool billable: proj && proj.charge === "R"
            text: emp ? ("Rate: " + (billable ? ("$" + emp.billRate + "/hr billed") : "non-billable (overhead)")
                         + " · $" + emp.costRate + "/hr cost") : ""
        }

        Text { text: store.lastError; color: Theme.bad; font.pixelSize: 11; visible: text !== ""; width: parent.width; wrapMode: Text.WordWrap }
    }

    footer: Row {
        layoutDirection: Qt.RightToLeft
        spacing: 10
        padding: 16
        Rectangle {
            width: 110; height: 38; radius: 9; color: Theme.accent
            Text { anchors.centerIn: parent; text: "Post entry"; color: "white"; font.pixelSize: 13; font.bold: true }
            MouseArea {
                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var proj = store.projectOptions[projCombo.currentIndex];
                    var emp  = store.staffOptions[empCombo.currentIndex];
                    if (!proj || !emp) return;
                    var billRate = (proj.charge === "R") ? emp.billRate : 0;
                    if (store.postLabor(proj.wbs1, emp.employee, dateField.text,
                                        parseFloat(hoursField.text), billRate, emp.costRate))
                        dlg.close();
                }
            }
        }
        Rectangle {
            width: 90; height: 38; radius: 9; color: Theme.field
            Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.ink; font.pixelSize: 13; font.bold: true }
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: dlg.close() }
        }
    }
}
