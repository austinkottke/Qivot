import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    width: 460
    height: 620
    visible: true
    title: "Qivot × QML"

    // Registered C++ type — created declaratively, no setContextProperty.
    PostStore {
        id: store
        Component.onCompleted: reload()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            Label { text: "📚  Posts"; font.pixelSize: 20; font.bold: true; Layout.fillWidth: true }
            Label { text: store.status; opacity: 0.7 }        // bound to a NOTIFY property
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // --- Add form ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            TextField {
                id: titleField
                placeholderText: "New post title…"
                Layout.fillWidth: true
                onAccepted: addButton.clicked()
            }
            TextField {
                id: authorField
                placeholderText: "author"
                Layout.preferredWidth: 110
            }
            Button {
                id: addButton
                text: "Add"
                enabled: titleField.text.trim().length > 0
                onClicked: {
                    store.add(titleField.text, authorField.text)   // insert via the library
                    titleField.clear()
                    authorField.clear()
                    titleField.forceActiveFocus()
                }
            }
        }

        // --- Search (full-text) ---
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "🔍  Search title / author…"
            onTextChanged: store.search(text)
        }

        // --- The posts, straight from the QiListModel ---
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: store.posts                              // a QiListModel; roles == field names

            delegate: Frame {
                width: listView.width
                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Label { text: title; font.bold: true; font.pixelSize: 15 }
                        Label {
                            text: "#" + remoteId + "  ·  by " + author
                            opacity: 0.55
                            font.pixelSize: 11
                        }
                    }
                    Button {
                        text: "🗑"
                        flat: true
                        onClicked: store.remove(remoteId)       // delete via the library
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: listView.count === 0
                text: "No posts"
                opacity: 0.4
            }
        }
    }
}
