import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 640
    height: 480
    visible: true
    title: "Qivot Tasks"

    Component.onCompleted: store.loadTasks()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Header
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: store.status
                font.pixelSize: 14
                Layout.fillWidth: true
            }
        }

        // Task list
        ListView {
            id: taskList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: store.tasks
            spacing: 5

            delegate: ItemDelegate {
                width: taskList.width
                implicitHeight: row.height + 10

                RowLayout {
                    id: row
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 10

                    CheckBox {
                        checked: done
                        onClicked: store.toggleTask(id)
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            text: title
                            font.strikeout: done
                            Layout.fillWidth: true
                        }

                        Label {
                            text: "Priority: " + priority
                            font.pixelSize: 12
                            color: "#666"
                        }
                    }

                    Button {
                        text: "Delete"
                        onClicked: store.removeTask(id)
                    }
                }
            }
        }

        // Add task controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            TextField {
                id: titleField
                placeholderText: "New task..."
                Layout.fillWidth: true
                onAccepted: addButton.clicked()
            }

            SpinBox {
                id: priorityField
                from: 1
                to: 10
                value: 1
            }

            Button {
                id: addButton
                text: "Add"
                onClicked: {
                    if (titleField.text.length > 0) {
                        store.addTask(titleField.text, priorityField.value)
                        titleField.clear()
                        priorityField.value = 1
                    }
                }
            }
        }
    }
}
