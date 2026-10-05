import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 390; height: 844
    color: "white"
    title: "Contacts"

    ContactStore { id: contactStore }

    // Room for the status bar / home indicator on phones (0 on desktop).
    property int safeTop: 0
    property int safeBottom: 0

    // The contact to scroll to and flash when you come back from editing it.
    property int revealId: -1
    property int flashId: -1
    Timer { id: flashTimer; interval: 1200; onTriggered: win.flashId = -1 }
    function reveal(id) { win.revealId = id }

    // "Sync": another device changing the contacts you're looking at.
    property bool syncing: false

    // Open the card for a row, as tapping it does (the screenshot run uses these).
    function openRow(row) {
        var c = contactStore.contactAt(row);
        stack.push(detailPage, { contactId: c.id, firstName: c.firstName,
                                 lastName: c.lastName, phone: c.phone });
    }
    function editOpenCard() { addSheet.edit(stack.currentItem) }
    function scrollTo(row) { stack.get(0).showRow(row) }
    function goBack() { stack.pop() }

    // Android's back button (and closing on desktop) pops a pushed card first.
    onClosing: function(close) {
        if (stack.depth > 1) { close.accepted = false; stack.pop() }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: listPage

        // iOS-style push: the new card slides in over a parallaxed list.
        pushEnter: Transition { NumberAnimation { property: "x"; from: stack.width; to: 0; duration: 320; easing.type: Easing.OutCubic } }
        pushExit:  Transition { NumberAnimation { property: "x"; from: 0; to: -stack.width * 0.3; duration: 320; easing.type: Easing.OutCubic } }
        popEnter:  Transition { NumberAnimation { property: "x"; from: -stack.width * 0.3; to: 0; duration: 280; easing.type: Easing.OutCubic } }
        popExit:   Transition { NumberAnimation { property: "x"; from: 0; to: stack.width; duration: 280; easing.type: Easing.OutCubic } }
    }

    Component {
        id: detailPage
        ContactDetail {
            store: contactStore
            safeTop: win.safeTop; safeBottom: win.safeBottom
            onBack: stack.pop()
            onEdit: addSheet.edit(this)
        }
    }

    AddSheet { id: addSheet; store: contactStore; safeTop: win.safeTop
               onSaved: function(id) { win.reveal(id) } }

    // =====================================================================
    //  The list screen
    // =====================================================================
    Component {
        id: listPage
        Item {
            id: page

            // 0 → large title fully visible, 1 → collapsed into the nav bar.
            readonly property real collapse:
                Math.min(1, Math.max(0, (list.contentY - list.originY - 34) / 22))

            property string currentSection: ""
            property string currentName: ""
            property int    currentIndex: -1
            property bool   showHud: false

            Timer { id: hudTimer; interval: 700; onTriggered: page.showHud = false }
            function pokeHud() { page.showHud = true; hudTimer.restart() }

            // Back on the list after an edit: show where the contact went.
            StackView.onActivated: {
                if (win.revealId < 0) return;
                var row = contactStore.rowOf(win.revealId);
                if (row >= 0) {
                    list.positionViewAtIndex(row, ListView.Center);
                    win.flashId = win.revealId; flashTimer.restart();
                }
                win.revealId = -1;
            }

            function showRow(row) {
                if (row <= 0) list.positionViewAtBeginning();     // with the title and search
                else list.positionViewAtIndex(row, ListView.Beginning);
            }

            Timer {
                objectName: "syncTimer"
                interval: 1400; repeat: true; running: win.syncing && page.StackView.status === StackView.Active
                onTriggered: {
                    var first = list.indexAt(list.width / 2, list.contentY + 40);
                    var last  = list.indexAt(list.width / 2, list.contentY + list.height - 40);
                    if (first < 0) first = 0;
                    if (last < first) last = first + 8;
                    contactStore.simulateChange(first, last);
                }
            }

            // Which contact sits at the top of the viewport.
            function refreshSection() {
                var y = list.contentY + 2;
                var idx = list.indexAt(list.width / 2, y);
                if (idx < 0) idx = list.indexAt(list.width / 2, y + 30);   // skip a section band
                if (idx >= 0) {
                    page.currentIndex   = idx;
                    page.currentSection = contactStore.sectionForIndex(idx);
                    page.currentName    = contactStore.nameForIndex(idx);
                }
            }

            ListView {
                id: list
                anchors { top: navBar.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
                model: contactStore.contacts
                clip: true
                boundsBehavior: Flickable.DragAndOvershootBounds
                maximumFlickVelocity: 5000
                flickDeceleration: 1800
                cacheBuffer: 600

                onContentYChanged: {
                    page.refreshSection();
                    // The position HUD only earns its place on a fast fling.
                    if (moving && Math.abs(verticalVelocity) > 2200) page.pokeHud();
                }
                Connections { target: contactStore; function onCountChanged() { page.refreshSection() } }

                // ---- Large title, plus a slot the search box floats over ----
                // The search field itself lives outside the ListView (see searchBox
                // below): every keystroke resets the model, and a field inside the
                // header lost keyboard focus on that reset after the first letter.
                header: Item {
                    width: list.width; height: 104
                    Text {
                        x: 16; y: 2
                        text: "Contacts"
                        font.pixelSize: 34; font.weight: Font.Bold; color: "#000000"
                        opacity: 1 - page.collapse
                        // Rubber-band the title a touch when you pull down at the top.
                        scale: 1 + Math.max(0, list.originY - list.contentY) / 700
                        transformOrigin: Item.Left
                    }
                }

                // ---- Count at the very bottom, like the system app ----
                footer: Item {
                    width: list.width; height: 64 + win.safeBottom
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter; y: 18
                        text: contactStore.count.toLocaleString(Qt.locale(), "f", 0)
                              + (contactStore.count === 1 ? " Contact" : " Contacts")
                        font.pixelSize: 15; color: "#8E8E93"
                        visible: contactStore.count > 0
                    }
                }

                section.property: "lastName"
                section.criteria: ViewSection.FirstCharacter
                section.labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart
                section.delegate: Rectangle {
                    width: list.width; height: 28; color: "#F7F7F7"
                    Text {
                        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        text: section.toUpperCase()
                        font.pixelSize: 14; font.weight: Font.DemiBold; color: "#8E8E93"
                    }
                }

                // ---- A contact row: tap to open, swipe left to delete ----
                delegate: SwipeDelegate {
                    id: row
                    width: list.width; height: 62
                    padding: 0
                    readonly property int contactId: model.id

                    background: Rectangle {
                        color: row.pressed ? "#E5E5EA"
                             : row.contactId === win.flashId ? "#DCEBFF" : "white"
                        Behavior on color { ColorAnimation { duration: 300 } }
                    }
                    contentItem: Item {
                        Avatar {
                            x: 16; anchors.verticalCenter: parent.verticalCenter
                            size: 42; first: firstName; last: lastName
                        }
                        Column {
                            x: 70; anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 70 - 40; spacing: 2
                            Text {
                                width: parent.width; elide: Text.ElideRight
                                textFormat: Text.StyledText
                                text: firstName + " <b>" + lastName + "</b>"
                                font.pixelSize: 17; color: "#000000"
                            }
                            Text { text: phone; font.pixelSize: 14; color: "#8E8E93" }
                        }
                        Rectangle {
                            anchors { left: parent.left; leftMargin: 70; right: parent.right; bottom: parent.bottom }
                            height: 1; color: "#E5E5EA"
                        }
                    }

                    swipe.right: Rectangle {
                        width: 92; height: row.height
                        anchors.right: parent.right
                        color: deleteMouse.pressed ? "#D70015" : "#FF3B30"
                        Text { anchors.centerIn: parent; text: "Delete"; color: "white"
                               font.pixelSize: 16; font.weight: Font.DemiBold }
                        MouseArea {
                            id: deleteMouse; anchors.fill: parent
                            onClicked: { var id = row.contactId; row.swipe.close(); contactStore.remove(id) }
                        }
                    }

                    onClicked: {
                        if (swipe.position !== 0) { swipe.close(); return }
                        stack.push(detailPage, { contactId: model.id, firstName: firstName,
                                                 lastName: lastName, phone: phone })
                    }
                }

                // The model only ever inserts, removes and moves single rows, so
                // the view can animate each one like a table view does.
                add: Transition {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                    NumberAnimation { property: "x"; from: list.width * 0.25; to: 0; duration: 300; easing.type: Easing.OutCubic }
                }
                remove: Transition {
                    NumberAnimation { property: "opacity"; to: 0; duration: 220 }
                    NumberAnimation { property: "x"; to: -list.width * 0.3; duration: 240; easing.type: Easing.InCubic }
                }
                displaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 260; easing.type: Easing.OutCubic }
                    NumberAnimation { property: "opacity"; to: 1; duration: 120 }
                }

                ScrollIndicator.vertical: ScrollIndicator { }
            }

            // ---- Search: tracks the header slot, so it scrolls with the title ----
            Rectangle {
                id: searchBox
                x: 16; width: parent.width - 32; height: 38; radius: 10
                color: "#EEEEF0"
                y: list.y + (list.headerItem ? list.headerItem.y - list.contentY : 0) + 104 - 12 - height
                // Magnifier drawn with two shapes, so it tints and scales cleanly.
                Item {
                    id: glass
                    width: 16; height: 16
                    anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter }
                    Rectangle { width: 11; height: 11; radius: 5.5; color: "transparent"
                                border.color: "#8E8E93"; border.width: 2 }
                    Rectangle { x: 9; y: 11; width: 6; height: 2; radius: 1; color: "#8E8E93"
                                rotation: 45; transformOrigin: Item.Left }
                }
                TextField {
                    id: searchField
                    anchors { left: glass.right; leftMargin: 4; right: clearBtn.left
                              verticalCenter: parent.verticalCenter }
                    placeholderText: "Search"
                    placeholderTextColor: "#8E8E93"
                    font.pixelSize: 17
                    color: "#000000"
                    background: null
                    inputMethodHints: Qt.ImhNoPredictiveText
                    onTextChanged: contactStore.filter = text
                }
                Rectangle {
                    id: clearBtn
                    anchors { right: parent.right; rightMargin: 8; verticalCenter: parent.verticalCenter }
                    width: 18; height: 18; radius: 9; color: "#A6A6AB"
                    visible: searchField.text.length > 0
                    // A drawn cross: the "\u2715" glyph is missing from some Android fonts.
                    Rectangle { anchors.centerIn: parent; width: 9; height: 1.8; radius: 0.9; color: "white"; rotation: 45 }
                    Rectangle { anchors.centerIn: parent; width: 9; height: 1.8; radius: 0.9; color: "white"; rotation: -45 }
                    MouseArea { anchors.fill: parent; anchors.margins: -8; onClicked: searchField.clear() }
                }
            }

            // ---- Nothing matched the search ----
            Column {
                anchors.centerIn: list
                spacing: 6
                visible: contactStore.count === 0 && contactStore.filter.length > 0
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       text: "No Results"; font.pixelSize: 22; font.weight: Font.Bold; color: "#000000" }
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       text: "No results for “" + contactStore.filter + "”"
                       font.pixelSize: 15; color: "#8E8E93" }
            }

            // ---- Nav bar: small title fades in as the large one scrolls away ----
            Rectangle {
                id: navBar
                anchors { left: parent.left; right: parent.right; top: parent.top }
                height: win.safeTop + 48
                color: page.collapse > 0 ? "#F7F9F9F9" : "white"
                Rectangle {
                    anchors.bottom: parent.bottom; width: parent.width; height: 1
                    color: "#D1D1D6"; opacity: page.collapse
                }
                Text {
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 13 }
                    text: "Contacts"; font.pixelSize: 17; font.weight: Font.DemiBold
                    opacity: page.collapse
                }
                // "Sync" toggle: another device starts changing what's on screen.
                Rectangle {
                    id: syncBtn
                    objectName: "syncButton"
                    anchors { left: parent.left; leftMargin: 12; bottom: parent.bottom; bottomMargin: 9 }
                    width: syncRow.width + 20; height: 30; radius: 15
                    color: win.syncing ? "#007AFF" : "#EEEEF0"
                    opacity: syncMouse.pressed ? 0.6 : 1
                    Behavior on color { ColorAnimation { duration: 160 } }
                    Row {
                        id: syncRow
                        anchors.centerIn: parent; spacing: 6
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: win.syncing ? "white" : "#8E8E93"
                            SequentialAnimation on opacity {
                                running: win.syncing; loops: Animation.Infinite
                                NumberAnimation { to: 0.25; duration: 500 }
                                NumberAnimation { to: 1; duration: 500 }
                                onRunningChanged: if (!running) parent.opacity = 1
                            }
                        }
                        Text { text: "Sync"; font.pixelSize: 15; font.weight: Font.DemiBold
                               color: win.syncing ? "white" : "#007AFF" }
                    }
                    MouseArea { id: syncMouse; anchors.fill: parent; anchors.margins: -6
                                onClicked: win.syncing = !win.syncing }
                }

                // "+" drawn as two bars, so it's crisp at any density.
                Item {
                    id: addBtn
                    anchors { right: parent.right; rightMargin: 8; bottom: parent.bottom; bottomMargin: 2 }
                    width: 44; height: 44
                    opacity: addMouse.pressed ? 0.35 : 1
                    Rectangle { anchors.centerIn: parent; width: 20; height: 2.4; radius: 1.2; color: "#007AFF" }
                    Rectangle { anchors.centerIn: parent; width: 2.4; height: 20; radius: 1.2; color: "#007AFF" }
                    MouseArea { id: addMouse; anchors.fill: parent; onClicked: addSheet.open() }
                }
            }

            // ---- What the last change did, and what it cost ----
            Rectangle {
                id: activityPill
                objectName: "activityPill"
                property bool shown: false
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height - win.safeBottom - height - (shown ? 18 : -height)
                width: Math.min(parent.width - 24, activityText.implicitWidth + 32)
                height: activityText.implicitHeight + 18; radius: 16
                color: "#EB1C1C1E"
                opacity: shown ? 1 : 0
                Behavior on y { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: 200 } }
                Text {
                    id: activityText
                    anchors.centerIn: parent
                    width: Math.min(implicitWidth, activityPill.parent.width - 56)
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    lineHeight: 1.15
                    text: contactStore.activity
                    font.pixelSize: 13; color: "white"
                }
                Timer { id: pillTimer; interval: 3200; onTriggered: activityPill.shown = false }
                Connections {
                    target: contactStore
                    function onActivityChanged() {
                        if (contactStore.activity.length === 0) return;
                        activityPill.shown = true; pillTimer.restart()
                    }
                }
            }

            // ---- Floating position HUD (fast flings and index scrubbing) ----
            Rectangle {
                id: hud
                anchors.horizontalCenter: parent.horizontalCenter
                y: navBar.height + 110
                width: 240; height: 84; radius: 20
                color: "#E61C1C1E"
                visible: opacity > 0
                opacity: page.showHud && page.currentName.length ? 1 : 0
                scale: page.showHud ? 1.0 : 0.92
                Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Behavior on scale   { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                Row {
                    anchors.centerIn: parent; spacing: 14
                    Rectangle {
                        width: 54; height: 54; radius: 12; color: "#33FFFFFF"
                        anchors.verticalCenter: parent.verticalCenter
                        Text { anchors.centerIn: parent; text: page.currentSection
                               font.pixelSize: 30; font.bold: true; color: "white" }
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 3
                        Text { text: page.currentName; color: "white"; width: 136; elide: Text.ElideRight
                               font.pixelSize: 18; font.weight: Font.DemiBold }
                        Text { text: (page.currentIndex + 1) + " of " + contactStore.count
                               color: "#C8C8CE"; font.pixelSize: 13 }
                    }
                }
            }

            // ---- A–Z index: tap or drag to jump ----
            Item {
                id: indexBar
                width: 22
                anchors { right: parent.right; top: navBar.bottom; bottom: parent.bottom
                          topMargin: 120; bottomMargin: 24 + win.safeBottom }
                visible: contactStore.count > 30
                property var letters: "ABCDEFGHIJKLMNOPQRSTUVWXYZ#".split("")
                Column {
                    anchors.centerIn: parent
                    Repeater {
                        model: indexBar.letters
                        Text {
                            width: indexBar.width
                            height: Math.min(18, indexBar.height / indexBar.letters.length)
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            text: modelData
                            font.pixelSize: 11; font.weight: Font.DemiBold
                            color: "#007AFF"
                            scale: indexArea.pressed && modelData === page.currentSection ? 1.6 : 1.0
                            Behavior on scale { NumberAnimation { duration: 110 } }
                        }
                    }
                }
                MouseArea {
                    id: indexArea
                    anchors.fill: parent; anchors.leftMargin: -12   // a wider, thumb-friendly target
                    preventStealing: true
                    function jump(y) {
                        var n = indexBar.letters.length;
                        var rowH = Math.min(18, indexBar.height / n);
                        var top = (indexBar.height - rowH * n) / 2;
                        var i = Math.max(0, Math.min(n - 1, Math.floor((y - top) / rowH)));
                        var letter = indexBar.letters[i];
                        if (letter === page.currentSection && page.showHud) return;
                        page.currentSection = letter;
                        var idx = contactStore.indexForLetter(letter);
                        if (idx >= 0) {
                            page.currentIndex = idx;
                            page.currentName  = contactStore.nameForIndex(idx);
                            list.positionViewAtIndex(idx, ListView.Beginning);
                        }
                        page.pokeHud();
                    }
                    onPressed: function(mouse) { jump(mouse.y) }
                    onPositionChanged: function(mouse) { jump(mouse.y) }
                }
            }
        }
    }
}
