import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 390; height: 800
    color: "#FFFFFF"
    title: "Contacts"

    ContactStore { id: store }

    // The row currently at the top of the viewport (which contact / section).
    property string currentSection: ""
    property string currentName: ""
    property int    currentIndex: -1
    // The floating HUD is shown briefly while scrolling / scrubbing.
    property bool   showHud: false

    // Stable per-name avatar color (iOS-like palette).
    function avatarColor(seed) {
        var colors = ["#FF3B30","#FF9500","#FFCC00","#34C759","#30B0C7",
                      "#007AFF","#5856D6","#AF52DE","#FF2D55","#A2845E"];
        var h = 0;
        for (var i = 0; i < seed.length; i++) h = (h * 31 + seed.charCodeAt(i)) >>> 0;
        return colors[h % colors.length];
    }

    // Flash the HUD, then let it fade out shortly after motion stops.
    Timer { id: hudTimer; interval: 650; onTriggered: win.showHud = false }
    function pokeHud() { win.showHud = true; hudTimer.restart() }

    // Work out which contact sits at the top of the list and surface it.
    function refreshSection() {
        var x = list.width / 2;
        var idx = list.indexAt(x, list.contentY + 2);
        if (idx < 0) idx = list.indexAt(x, list.contentY + 30);   // skip a header band
        if (idx >= 0) {
            win.currentIndex   = idx;
            win.currentSection = store.sectionForIndex(idx);
            win.currentName    = store.nameForIndex(idx);
            pokeHud();
        }
    }

    // ---- Header: status, big title, + button, search ----
    Rectangle {
        id: topBar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: 118; color: "#F9F9F9"; z: 2

        Text {
            anchors { top: parent.top; topMargin: 8; horizontalCenter: parent.horizontalCenter }
            text: store.count + " Contacts"; font.pixelSize: 12; color: "#8E8E93"
        }
        Text {
            id: bigTitle
            anchors { left: parent.left; leftMargin: 16; top: parent.top; topMargin: 24 }
            text: "Contacts"; font.pixelSize: 32; font.bold: true; color: "#000000"
        }
        Rectangle {
            id: addBtn
            anchors { right: parent.right; rightMargin: 14; top: parent.top; topMargin: 28 }
            width: 34; height: 34; radius: 17
            color: addMouse.pressed ? "#E4E4EA" : "transparent"
            scale: addMouse.pressed ? 0.9 : 1.0
            Behavior on scale { NumberAnimation { duration: 90 } }
            Text { anchors.centerIn: parent; text: "＋"; font.pixelSize: 27; color: "#007AFF" }
            MouseArea { id: addMouse; anchors.fill: parent; onClicked: addDialog.open() }
        }
        Rectangle {
            id: searchBox
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom
                      leftMargin: 12; rightMargin: 12; bottomMargin: 10 }
            height: 36; radius: 10; color: "#EFEFF0"
            Text { text: "\u{1F50D}"; color: "#8E8E93"; font.pixelSize: 14
                   anchors { left: parent.left; leftMargin: 9; verticalCenter: parent.verticalCenter } }
            TextField {
                id: searchField
                anchors { left: parent.left; leftMargin: 30; right: clearBtn.left
                          verticalCenter: parent.verticalCenter }
                placeholderText: "Search"; font.pixelSize: 15
                background: null
                onTextChanged: store.filter = text
            }
            Text {
                id: clearBtn; visible: searchField.text.length > 0; text: "✕"; color: "#8E8E93"
                anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: searchField.clear() }
            }
        }
    }
    // Soft shadow under the header (no external effects needed).
    Rectangle {
        anchors { top: topBar.bottom; left: parent.left; right: parent.right }
        height: 6; z: 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#22000000" }
            GradientStop { position: 1.0; color: "#00000000" }
        }
    }

    // ---- The sectioned, reactive contact list ----
    ListView {
        id: list
        anchors { top: topBar.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
        clip: true
        model: store.contacts
        boundsBehavior: Flickable.OvershootBounds
        maximumFlickVelocity: 4500
        flickDeceleration: 2200
        cacheBuffer: 400

        onContentYChanged: win.refreshSection()
        onMovementStarted: win.pokeHud()
        Component.onCompleted: win.refreshSection()
        Connections { target: store; function onCountChanged() { win.refreshSection() } }

        section.property: "lastName"
        section.criteria: ViewSection.FirstCharacter
        section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
        section.delegate: Rectangle {
            width: list.width; height: 22; color: "#F2F2F7"
            Text {
                anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                text: section.toUpperCase(); font.pixelSize: 12; font.bold: true; color: "#6D6D72"
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#E1E1E6" }
        }

        delegate: Rectangle {
            id: row
            width: list.width; height: 50
            color: rowMouse.pressed ? "#D6D6DB" : "white"
            Behavior on color { ColorAnimation { duration: 120 } }
            Row {
                anchors.fill: parent; anchors.leftMargin: 16; spacing: 11
                Rectangle {
                    width: 34; height: 34; radius: 17; anchors.verticalCenter: parent.verticalCenter
                    color: win.avatarColor(firstName + lastName)
                    Text {
                        anchors.centerIn: parent; color: "white"; font.pixelSize: 13; font.bold: true
                        text: (firstName.charAt(0) + lastName.charAt(0)).toUpperCase()
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter; spacing: 1
                    Text {
                        textFormat: Text.StyledText; font.pixelSize: 15; color: "#000000"
                        text: firstName + " <b>" + lastName + "</b>"
                    }
                    Text { text: phone; font.pixelSize: 12; color: "#8E8E93" }
                }
            }
            Rectangle {
                anchors { left: parent.left; leftMargin: 61; right: parent.right; bottom: parent.bottom }
                height: 1; color: "#E7E7EA"
            }
            MouseArea { id: rowMouse; anchors.fill: parent }
        }

        ScrollBar.vertical: ScrollBar {
            id: vbar
            policy: ScrollBar.AlwaysOn          // always visible
            width: 12
            anchors.right: parent.right
            anchors.rightMargin: 1
            padding: 2
            background: Rectangle {
                implicitWidth: 12; radius: 5; color: "#14000000"
            }
            contentItem: Rectangle {
                implicitWidth: 8; radius: 4
                color: vbar.pressed ? "#5A5A60" : "#9A9AA0"
                opacity: vbar.active ? 1.0 : 0.75
                Behavior on color   { ColorAnimation { duration: 150 } }
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }
        }
    }

    // ---- Floating "current position" HUD ----
    // Shows the section letter, the contact now at the top, and how far you are
    // through the list — so you always know exactly which row you're on.
    Rectangle {
        id: hud
        anchors.horizontalCenter: list.horizontalCenter
        anchors.top: list.top; anchors.topMargin: 40
        width: 250; height: 96; radius: 22
        color: "#E61C1C1E"
        visible: opacity > 0
        opacity: win.showHud && win.currentName.length ? 1 : 0
        scale: win.showHud ? 1.0 : 0.9
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        Behavior on scale   { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

        Row {
            anchors.centerIn: parent; spacing: 16
            // Big section letter.
            Rectangle {
                width: 60; height: 60; radius: 14; anchors.verticalCenter: parent.verticalCenter
                color: "#33FFFFFF"
                Text { anchors.centerIn: parent; text: win.currentSection
                       font.pixelSize: 34; font.bold: true; color: "white" }
            }
            // Name + position.
            Column {
                anchors.verticalCenter: parent.verticalCenter; spacing: 4
                Text {
                    text: win.currentName; color: "white"
                    font.pixelSize: 20; font.bold: true
                    width: 130; elide: Text.ElideRight
                }
                Text {
                    text: (win.currentIndex + 1) + " of " + store.count
                    color: "#C8C8CE"; font.pixelSize: 13
                }
            }
        }
    }

    // ---- A–Z scrubber on the right (tap or drag to jump) ----
    Item {
        id: indexBar
        width: 18
        anchors { right: parent.right; rightMargin: 18; top: list.top; bottom: list.bottom
                  topMargin: 8; bottomMargin: 8 }
        property var letters: "ABCDEFGHIJKLMNOPQRSTUVWXYZ#".split("")
        Column {
            anchors.fill: parent
            Repeater {
                model: indexBar.letters
                Item {
                    width: indexBar.width; height: indexBar.height / indexBar.letters.length
                    property bool active: modelData === win.currentSection
                    Text {
                        anchors.centerIn: parent; text: modelData
                        font.pixelSize: parent.active ? 13 : 10
                        font.bold: true
                        color: parent.active ? "#0051D5" : "#007AFF"
                        scale: parent.active ? 1.25 : 1.0
                        Behavior on scale { NumberAnimation { duration: 120 } }
                        Behavior on font.pixelSize { NumberAnimation { duration: 120 } }
                    }
                }
            }
        }
        // Track pill that appears while scrubbing.
        Rectangle {
            anchors.fill: parent; anchors.margins: -2
            radius: width / 2; color: "#14007AFF"
            visible: indexArea.pressed
        }
        MouseArea {
            id: indexArea
            anchors.fill: parent
            function jump(y) {
                var n = indexBar.letters.length;
                var i = Math.max(0, Math.min(n - 1, Math.floor(y / (indexBar.height / n))));
                var letter = indexBar.letters[i];
                win.currentSection = letter;
                var idx = store.indexForLetter(letter);
                if (idx >= 0) {
                    win.currentIndex = idx;
                    win.currentName  = store.nameForIndex(idx);
                    list.positionViewAtIndex(idx, ListView.Beginning);
                }
                win.pokeHud();
            }
            onPressed: jump(mouse.y)
            onPositionChanged: jump(mouse.y)
        }
    }

    // ---- Add contact ----
    Dialog {
        id: addDialog
        anchors.centerIn: parent
        width: 300
        title: "New Contact"; modal: true
        standardButtons: Dialog.Save | Dialog.Cancel
        onAccepted: { store.add(fn.text, ln.text, ph.text); fn.clear(); ln.clear(); ph.clear() }
        onRejected: { fn.clear(); ln.clear(); ph.clear() }
        contentItem: ColumnLayout {
            spacing: 8
            TextField { id: fn; Layout.fillWidth: true; placeholderText: "First name" }
            TextField { id: ln; Layout.fillWidth: true; placeholderText: "Last name" }
            TextField { id: ph; Layout.fillWidth: true; placeholderText: "Phone" }
        }
    }
}
