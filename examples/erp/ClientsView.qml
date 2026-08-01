import QtQuick 2.15
import QtQuick.Controls 2.15

// Left: searchable client directory. Right: the selected client's CRM record —
// contacts and the opportunity pipeline.
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
            border.color: clientSearch.activeFocus ? Theme.accent : "transparent"
            Text { anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                   text: "⌕"; color: Theme.muted; font.pixelSize: 20 }
            TextField { id: clientSearch
                anchors { left: parent.left; leftMargin: 36; right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                placeholderText: "Search clients…"; background: null; color: Theme.ink; font.pixelSize: 14
                onTextChanged: searchDebounce.restart() }
            Timer { id: searchDebounce; interval: 90; onTriggered: store.searchClients(clientSearch.text) }
        }
        Text { anchors { left: parent.left; leftMargin: 18; top: searchBox.bottom; topMargin: 8 }
               text: store.clients.count + " clients"; color: Theme.muted; font.pixelSize: 12 }
        Rectangle {
            anchors { right: parent.right; rightMargin: 16; top: searchBox.bottom; topMargin: 2 }
            width: ncT.width + 22; height: 26; radius: 13
            color: ncMa.pressed ? Qt.darker(Theme.accent, 1.1) : (ncMa.containsMouse ? Qt.lighter(Theme.accent, 1.08) : Theme.accent)
            Behavior on color { ColorAnimation { duration: 130 } }
            Text { id: ncT; anchors.centerIn: parent; text: "＋ New"; color: "white"; font.pixelSize: 11; font.bold: true }
            MouseArea { id: ncMa; anchors.fill: parent; hoverEnabled: true; onClicked: addClientDialog.open() }
        }

        ListView {
            id: clientList
            anchors { left: parent.left; right: parent.right; top: searchBox.bottom; bottom: parent.bottom; topMargin: 28 }
            clip: true; model: store.clients
            populate: Transition {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                NumberAnimation { property: "x"; from: -24; to: 0; duration: 300; easing.type: Easing.OutCubic } }
            displaced: Transition { NumberAnimation { properties: "x,y"; duration: 220; easing.type: Easing.OutCubic } }
            delegate: ClientListItem {}
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
        }
    }

    // ---- CRM record ----
    Flickable {
        anchors { left: directory.right; right: parent.right; top: parent.top; bottom: parent.bottom }
        contentHeight: rec.height + 40; clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Connections { target: store; function onClientChanged() { recIn.restart() } }
        ParallelAnimation { id: recIn; running: true
            NumberAnimation { target: rec; property: "opacity"; from: 0; to: 1; duration: 300 }
            NumberAnimation { target: recRise; property: "y"; from: 22; to: 0; duration: 380; easing.type: Easing.OutCubic } }

        Column {
            id: rec
            x: 24; y: 24; width: parent.width - 48; spacing: 18
            opacity: 0
            transform: Translate { id: recRise; y: 22 }

            // header card
            Rectangle {
                width: parent.width; height: 112; radius: 16; color: "white"
                Row {
                    anchors { left: parent.left; leftMargin: 24; verticalCenter: parent.verticalCenter }
                    spacing: 20
                    Rectangle { width: 68; height: 68; radius: 16; color: Theme.avatarColor(store.selectedClientId)
                                anchors.verticalCenter: parent.verticalCenter
                                Text { anchors.centerIn: parent; text: Theme.initials(store.client.name)
                                       color: "white"; font.pixelSize: 24; font.bold: true } }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter; spacing: 6
                        Text { text: store.client.name; color: Theme.ink; font.pixelSize: 24; font.bold: true }
                        Text { text: store.client.industry + "   ·   " + store.client.city + ", " + store.client.state
                               color: Theme.muted; font.pixelSize: 14 }
                        Text { text: "☎ " + store.client.phone; color: Theme.muted; font.pixelSize: 13 }
                    }
                }
            }

            Row {
                width: parent.width; spacing: 18
                property real cw: (width - 18) / 2

                SectionCard {
                    width: parent.cw; title: "Contacts"; bodySpacing: 10
                    Row { spacing: 10; width: parent.width
                        Rectangle { width: acT.width + 24; height: 28; radius: 14
                            color: acMa.containsMouse ? "#EAF0FE" : "white"; border.color: "#DCE2F5"
                            Text { id: acT; anchors.centerIn: parent; text: "＋ Add contact"; color: Theme.accent; font.pixelSize: 12; font.bold: true }
                            MouseArea { id: acMa; anchors.fill: parent; hoverEnabled: true; onClicked: addContactDialog.open() } }
                    }
                    Repeater { model: store.contacts
                        Row { width: parent.width; spacing: 10
                            Column { width: parent.width - 90; spacing: 2
                                Text { text: model.name; color: Theme.ink; font.pixelSize: 14; font.bold: true }
                                Text { text: model.title + (model.email ? " · " + model.email : ""); color: Theme.muted
                                       font.pixelSize: 12; width: parent.width; elide: Text.ElideRight } }
                        } }
                    Text { visible: store.contacts.count === 0; text: "No contacts yet"; color: Theme.muted; font.pixelSize: 13 }
                }

                SectionCard {
                    width: parent.cw; title: "Opportunities"; bodySpacing: 10
                    Row { spacing: 10; width: parent.width
                        Rectangle { width: aoT.width + 24; height: 28; radius: 14
                            color: aoMa.containsMouse ? "#EAF0FE" : "white"; border.color: "#DCE2F5"
                            Text { id: aoT; anchors.centerIn: parent; text: "＋ Add opportunity"; color: Theme.accent; font.pixelSize: 12; font.bold: true }
                            MouseArea { id: aoMa; anchors.fill: parent; hoverEnabled: true; onClicked: addOpportunityDialog.open() } }
                    }
                    Repeater { model: store.opportunities
                        Rectangle {
                            id: oppRow
                            width: parent.width; height: 50; radius: 10; color: "#F8F9FB"
                            // captured before the ComboBox below shadows the delegate's `model` with its own
                            property int oppId: model.id
                            property int oppStage: model.stage
                            Rectangle { width: 4; height: parent.height - 16; radius: 2; color: Theme.stageColor(oppRow.oppStage)
                                        anchors { left: parent.left; leftMargin: 10; verticalCenter: parent.verticalCenter } }
                            Column { spacing: 2
                                anchors { left: parent.left; leftMargin: 22; right: stageBox.left; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                Text { text: model.name; color: Theme.ink; font.pixelSize: 13; font.bold: true
                                       width: parent.width; elide: Text.ElideRight }
                                Text { text: store.money(model.amount) + "  ·  closes " + Theme.dateLabel(model.closeDate)
                                       color: Theme.muted; font.pixelSize: 11 } }
                            ComboBox { id: stageBox
                                anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                width: 118; height: 30
                                model: [ "Lead", "Qualified", "Proposal", "Won", "Lost" ]
                                currentIndex: oppRow.oppStage
                                onActivated: store.setOpportunityStage(oppRow.oppId, currentIndex)
                            }
                        } }
                    Text { visible: store.opportunities.count === 0; text: "No opportunities yet"; color: Theme.muted; font.pixelSize: 13 }
                }
            }
            Item { width: 1; height: 8 }
        }
    }

    // dialogs owned by this view
    AddClientDialog      { id: addClientDialog }
    AddContactDialog      { id: addContactDialog;     clientId: store.selectedClientId }
    AddOpportunityDialog  { id: addOpportunityDialog;  clientId: store.selectedClientId }
}
