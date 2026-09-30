import QtQuick 2.15
import QtQuick.Controls 2.15

/// "New Contact" card that slides up from the bottom. Drag it down to dismiss.
Drawer {
    id: sheet
    property var store
    property int safeTop: 0

    edge: Qt.BottomEdge
    dragMargin: 0                       // open only from the + button, never by an edge swipe
    width: parent ? parent.width : 0
    height: parent ? parent.height - safeTop - 12 : 0
    modal: true
    Overlay.modal: Rectangle { color: "#59000000" }

    readonly property bool canSave: firstField.text.trim().length > 0 || lastField.text.trim().length > 0

    function reset() { firstField.clear(); lastField.clear(); phoneField.clear() }
    onOpened: firstField.forceActiveFocus()
    onClosed: reset()

    background: Rectangle {
        color: "#F2F2F7"; radius: 12
        // Square off the bottom corners (they sit at the screen edge).
        Rectangle { anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                    height: 12; color: parent.color }
    }

    Item {
        anchors.fill: parent

        // Grab handle.
        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: 6
                    width: 36; height: 5; radius: 2.5; color: "#C7C7CC" }

        // ---- Cancel · New Contact · Done ----
        Item {
            id: bar
            width: parent.width; height: 56
            Text {
                anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                text: "Cancel"; font.pixelSize: 17; color: "#007AFF"
                opacity: cancelMouse.pressed ? 0.4 : 1
                MouseArea { id: cancelMouse; anchors.fill: parent; anchors.margins: -10; onClicked: sheet.close() }
            }
            Text {
                anchors.centerIn: parent
                text: "New Contact"; font.pixelSize: 17; font.weight: Font.DemiBold
            }
            Text {
                anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                text: "Done"; font.pixelSize: 17; font.weight: Font.DemiBold
                color: sheet.canSave ? "#007AFF" : "#C7C7CC"
                opacity: doneMouse.pressed ? 0.4 : 1
                MouseArea {
                    id: doneMouse; anchors.fill: parent; anchors.margins: -10
                    enabled: sheet.canSave
                    onClicked: { sheet.store.add(firstField.text, lastField.text, phoneField.text); sheet.close() }
                }
            }
        }

        Flickable {
            anchors { top: bar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
            contentHeight: form.height + 40
            clip: true

            Column {
                id: form
                width: parent.width
                spacing: 24
                topPadding: 8

                // Live monogram: fills in as you type.
                Item {
                    width: parent.width; height: 112
                    Avatar {
                        anchors.centerIn: parent
                        size: 108
                        first: firstField.text; last: lastField.text
                        visible: sheet.canSave
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: 108; height: 108; radius: 54
                        visible: !sheet.canSave
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#B8BCC6" }
                            GradientStop { position: 1.0; color: "#8E939E" }
                        }
                        // Person silhouette, clipped to the circle so the shoulders
                        // meet its edge cleanly.
                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var c = getContext("2d");
                                c.reset();
                                c.beginPath(); c.arc(54, 54, 54, 0, Math.PI * 2); c.clip();
                                c.fillStyle = "#F2F2F7";
                                c.beginPath(); c.arc(54, 42, 19, 0, Math.PI * 2); c.fill();
                                c.beginPath(); c.ellipse(16, 70, 76, 64); c.fill();
                            }
                        }
                    }
                }

                // Grouped card of fields.
                Rectangle {
                    x: 16; width: parent.width - 32; radius: 12; color: "white"
                    height: fields.height
                    Column {
                        id: fields
                        width: parent.width
                        FormField { id: firstField; placeholder: "First name" }
                        Rectangle { x: 16; width: parent.width - 16; height: 1; color: "#E5E5EA" }
                        FormField { id: lastField; placeholder: "Last name" }
                    }
                }

                Rectangle {
                    x: 16; width: parent.width - 32; radius: 12; color: "white"
                    height: phoneField.height
                    FormField {
                        id: phoneField; width: parent.width; placeholder: "Phone"
                        inputMethodHints: Qt.ImhDialableCharactersOnly
                    }
                }
            }
        }
    }

    component FormField: TextField {
        property string placeholder: ""
        width: parent ? parent.width : 0
        height: 50
        leftPadding: 16; rightPadding: 16
        font.pixelSize: 17
        placeholderText: placeholder
        placeholderTextColor: "#C7C7CC"
        color: "#000000"
        selectByMouse: true
        background: null
    }
}
