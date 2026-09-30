import QtQuick 2.15
import QtQuick.Controls 2.15

/// The card a row opens: big avatar, quick actions, phone, delete.
Page {
    id: page
    property var    store
    property int    safeTop: 0
    property int    safeBottom: 0
    property int    contactId: -1
    property string firstName: ""
    property string lastName: ""
    property string phone: ""

    signal back()

    readonly property string fullName: (firstName + " " + lastName).trim()
    readonly property string dialable: phone.replace(/[^0-9+]/g, "")

    background: Rectangle { color: "#F2F2F7" }

    Flickable {
        id: scroller
        anchors.fill: parent
        contentHeight: content.height + page.safeBottom + 32
        boundsBehavior: Flickable.DragOverBounds

        Column {
            id: content
            width: parent.width
            spacing: 18

            // ---- Hero: tinted wash, avatar, name ----
            Item {
                width: parent.width
                height: page.safeTop + 44 + 236

                Rectangle {
                    anchors.fill: parent
                    // Stretch the wash upward when pulled down, so the top never shows a gap.
                    anchors.topMargin: Math.min(0, scroller.contentY)
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: Qt.lighter(heroAvatar.base, 1.55) }
                        GradientStop { position: 1.0; color: "#F2F2F7" }
                    }
                }

                Avatar {
                    id: heroAvatar
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: page.safeTop + 44 + 4
                    size: 108
                    first: page.firstName; last: page.lastName
                    // Grow a little when pulled down, like the system app.
                    scale: 1 + Math.max(0, -scroller.contentY) / 500
                    transformOrigin: Item.Bottom
                }

                Text {
                    id: nameText
                    anchors { top: heroAvatar.bottom; topMargin: 14
                              horizontalCenter: parent.horizontalCenter }
                    width: parent.width - 48
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    text: page.fullName
                    font.pixelSize: 30; font.weight: Font.DemiBold; color: "#000000"
                }

                // Quick actions.
                Row {
                    anchors { top: nameText.bottom; topMargin: 18
                              horizontalCenter: parent.horizontalCenter }
                    spacing: 10
                    Repeater {
                        model: [
                            { icon: "message", label: "message", url: "sms:" },
                            { icon: "call",    label: "call",    url: "tel:" }
                        ]
                        delegate: Rectangle {
                            width: (page.width - 32 - 10) / 2; height: 62; radius: 12
                            color: tileMouse.pressed ? "#E6FFFFFF" : "#B3FFFFFF"
                            scale: tileMouse.pressed ? 0.96 : 1
                            Behavior on scale { NumberAnimation { duration: 90 } }
                            Column {
                                anchors.centerIn: parent; spacing: 2
                                // Drawn rather than an emoji, so it's the tint color everywhere
                                // (Android ignores the text-presentation selector).
                                Canvas {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 26; height: 26
                                    property string kind: modelData.icon
                                    onPaint: {
                                        var c = getContext("2d");
                                        c.reset();
                                        c.fillStyle = "#007AFF"; c.strokeStyle = "#007AFF";
                                        if (kind === "message") {
                                            // Speech bubble with a tail at the lower left.
                                            c.beginPath();
                                            c.ellipse(1, 2, 24, 18);
                                            c.fill();
                                            c.beginPath();
                                            c.moveTo(5, 15); c.lineTo(3, 24); c.lineTo(12, 18);
                                            c.closePath(); c.fill();
                                        } else {
                                            // Handset: a curved grip with an earpiece and a
                                            // mouthpiece pad at either end, tilted 45°.
                                            c.translate(13, 13); c.rotate(-Math.PI / 4);
                                            c.lineWidth = 4; c.lineCap = "round";
                                            c.beginPath();
                                            c.arc(0, -3, 8, Math.PI * 0.9, Math.PI * 0.1, true);
                                            c.stroke();
                                            c.beginPath(); c.ellipse(-11, -3, 7, 10); c.fill();
                                            c.beginPath(); c.ellipse(4, -3, 7, 10); c.fill();
                                        }
                                    }
                                }
                                Text { anchors.horizontalCenter: parent.horizontalCenter
                                       text: modelData.label; font.pixelSize: 12; color: "#007AFF" }
                            }
                            MouseArea {
                                id: tileMouse; anchors.fill: parent
                                enabled: page.dialable.length > 0
                                onClicked: Qt.openUrlExternally(modelData.url + page.dialable)
                            }
                        }
                    }
                }
            }

            // ---- Phone ----
            Rectangle {
                x: 16; width: parent.width - 32; height: 64; radius: 12
                color: phoneMouse.pressed ? "#E5E5EA" : "white"
                Behavior on color { ColorAnimation { duration: 120 } }
                Column {
                    anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                    spacing: 3
                    Text { text: "mobile"; font.pixelSize: 13; color: "#3C3C43" }
                    Text { text: page.phone.length ? page.phone : "No number"
                           font.pixelSize: 17; color: page.phone.length ? "#007AFF" : "#8E8E93" }
                }
                MouseArea {
                    id: phoneMouse; anchors.fill: parent
                    enabled: page.dialable.length > 0
                    onClicked: Qt.openUrlExternally("tel:" + page.dialable)
                }
            }

            // ---- Delete ----
            Rectangle {
                x: 16; width: parent.width - 32; height: 50; radius: 12
                color: delMouse.pressed ? "#E5E5EA" : "white"
                Behavior on color { ColorAnimation { duration: 120 } }
                Text { anchors.centerIn: parent; text: "Delete Contact"
                       font.pixelSize: 17; color: "#FF3B30" }
                MouseArea { id: delMouse; anchors.fill: parent; onClicked: confirm.open() }
            }
        }
    }

    // ---- Floating back button (stays put while the card scrolls) ----
    Rectangle {
        id: navBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: page.safeTop + 44
        color: "#F2F9F9F9"
        opacity: Math.min(1, Math.max(0, (scroller.contentY - 120) / 60))
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#D1D1D6" }
        Text {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 12 }
            text: page.fullName; font.pixelSize: 17; font.weight: Font.DemiBold
        }
    }
    Item {
        anchors { left: parent.left; top: parent.top; topMargin: page.safeTop }
        width: backRow.width + 24; height: 44
        Row {
            id: backRow
            anchors { left: parent.left; leftMargin: 8; verticalCenter: parent.verticalCenter }
            spacing: 2
            opacity: backMouse.pressed ? 0.4 : 1
            Text { text: "‹"; font.pixelSize: 34; color: "#007AFF"
                   anchors.verticalCenter: parent.verticalCenter; anchors.verticalCenterOffset: -2 }
            Text { text: "Contacts"; font.pixelSize: 17; color: "#007AFF"
                   anchors.verticalCenter: parent.verticalCenter }
        }
        MouseArea { id: backMouse; anchors.fill: parent; onClicked: page.back() }
    }

    // ---- iOS-style action sheet to confirm the delete ----
    Popup {
        id: confirm
        parent: Overlay.overlay
        width: parent ? parent.width : 0
        height: sheet.height + page.safeBottom + 16
        y: parent ? parent.height - height : 0
        modal: true; dim: true
        padding: 0
        background: null
        Overlay.modal: Rectangle { color: "#66000000" }
        enter: Transition { NumberAnimation { property: "y"; from: confirm.parent.height; duration: 260; easing.type: Easing.OutCubic } }
        exit:  Transition { NumberAnimation { property: "y"; to: confirm.parent.height; duration: 200; easing.type: Easing.InCubic } }

        Column {
            id: sheet
            x: 8; width: parent.width - 16; spacing: 8
            Rectangle {
                width: parent.width; height: 104; radius: 14; color: "#F2F2F7"
                Column {
                    anchors.fill: parent
                    Text { width: parent.width; height: 46; horizontalAlignment: Text.AlignHCenter
                           verticalAlignment: Text.AlignVCenter; font.pixelSize: 13; color: "#8E8E93"
                           text: "Delete " + page.fullName + "?" ; elide: Text.ElideRight }
                    Rectangle { width: parent.width; height: 1; color: "#D1D1D6" }
                    Rectangle {
                        width: parent.width; height: 57; radius: 14
                        color: yesMouse.pressed ? "#E5E5EA" : "transparent"
                        Text { anchors.centerIn: parent; text: "Delete Contact"
                               font.pixelSize: 20; color: "#FF3B30" }
                        MouseArea {
                            id: yesMouse; anchors.fill: parent
                            onClicked: { confirm.close(); page.store.remove(page.contactId); page.back() }
                        }
                    }
                }
            }
            Rectangle {
                width: parent.width; height: 57; radius: 14
                color: noMouse.pressed ? "#E5E5EA" : "white"
                Text { anchors.centerIn: parent; text: "Cancel"
                       font.pixelSize: 20; font.weight: Font.DemiBold; color: "#007AFF" }
                MouseArea { id: noMouse; anchors.fill: parent; onClicked: confirm.close() }
            }
        }
    }
}
