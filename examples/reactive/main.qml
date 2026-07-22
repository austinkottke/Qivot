import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    visible: true; width: 440; height: 580; title: "Reactive Qivot"

    TaskStore { id: store }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 14; spacing: 10

        Label { text: "Tasks (" + store.tasks.count + ")"
                font.pixelSize: 24; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: input; Layout.fillWidth: true; placeholderText: "New task…"
                onAccepted: { store.add(text); text = "" }
            }
            Button { text: "Add"; onClicked: { store.add(input.text); input.text = "" } }
        }

        CheckBox {
            id: auto
            text: "Auto-add a task every 2s — watch the list grow by itself"
        }

        Frame {
            Layout.fillWidth: true; Layout.fillHeight: true
            ListView {
                anchors.fill: parent; clip: true; spacing: 4
                model: store.tasks
                delegate: RowLayout {
                    width: ListView.view ? ListView.view.width : 0
                    CheckBox { checked: done === 1; onToggled: store.toggle(id) }
                    Label {
                        Layout.fillWidth: true; text: title
                        elide: Text.ElideRight; font.strikeout: done === 1
                    }
                    Button { text: "✕"; flat: true; onClicked: store.remove(id) }
                }
            }
        }
    }

    // A background mutation source: proves the view reacts to changes the UI
    // didn't initiate.
    Timer {
        interval: 2000; repeat: true; running: auto.checked
        onTriggered: store.add("heartbeat " + Qt.formatDateTime(new Date(), "hh:mm:ss"))
    }
}
