import QtQuick 2.15

// The window header. Two layouts: the wide desktop one (brand + centered segmented
// tabs + chip + toggle) and a compact phone one (logo + horizontally-scrollable
// tab strip + toggle). Which one shows is driven by Theme.compact.
Rectangle {
    id: root
    property int currentTab: 0
    readonly property var tabs: [ "Overview", "Projects", "Visualization", "Utilization", "Ledger" ]
    signal select(int index)

    height: Theme.compact ? 54 : 62
    color: Theme.card
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

    // brand gradient logo (both layouts)
    Rectangle {
        id: logo
        x: Theme.compact ? 12 : 24
        width: 30; height: 30; radius: 8; anchors.verticalCenter: parent.verticalCenter
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: Theme.accent }
            GradientStop { position: 1.0; color: Theme.accent2 }
        }
        Text { anchors.centerIn: parent; text: "◆"; color: "white"; font.pixelSize: 15; font.bold: true }
    }

    // dark/light toggle (both layouts), pinned right
    Rectangle {
        id: toggle
        width: 34; height: 30; radius: 15; color: Theme.chip
        anchors { right: parent.right; rightMargin: Theme.compact ? 12 : 24; verticalCenter: parent.verticalCenter }
        Text { anchors.centerIn: parent; text: Theme.dark ? "☾" : "☀"
               color: Theme.dark ? Theme.warn : Theme.accent; font.pixelSize: 15 }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: Theme.toggle() }
    }

    // ---------------- WIDE (desktop) ----------------
    Column {
        visible: !Theme.compact
        anchors { left: logo.right; leftMargin: 10; verticalCenter: parent.verticalCenter }
        spacing: 0
        Text { text: "Qivot · Vision"; color: Theme.ink; font.pixelSize: 19; font.bold: true }
        Text { text: "A/E/C project accounting"; color: Theme.muted; font.pixelSize: 10 }
    }
    Rectangle {
        visible: !Theme.compact
        anchors.centerIn: parent
        width: root.tabs.length * 116 + 8; height: 42; radius: 12; color: Theme.field
        Rectangle {
            width: 108; height: 34; radius: 9; y: 4
            x: 4 + root.currentTab * 116
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accent }
                GradientStop { position: 1.0; color: Theme.accent2 }
            }
            Behavior on x { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
        }
        Row {
            anchors.fill: parent
            Repeater {
                model: root.tabs
                Item {
                    width: 116; height: 42
                    Text { anchors.centerIn: parent; text: modelData
                           color: root.currentTab === index ? "white" : Theme.muted
                           font.pixelSize: 13; font.bold: true
                           Behavior on color { ColorAnimation { duration: 200 } } }
                    MouseArea { anchors.fill: parent; onClicked: root.select(index) }
                }
            }
        }
    }
    Rectangle {
        visible: !Theme.compact
        width: chip.width + 26; height: 30; radius: 15; color: Theme.chip
        anchors { right: toggle.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
        Row { id: chip; anchors.centerIn: parent; spacing: 6
            Text { text: "◱"; color: Theme.accent; font.pixelSize: 13; font.bold: true; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "Synthetic demo · SQLite"; color: Theme.accent; font.pixelSize: 12; font.bold: true; anchors.verticalCenter: parent.verticalCenter } }
    }

    // ---------------- COMPACT (phone) ----------------
    // A horizontally-scrollable strip of pill tabs between the logo and the toggle.
    Flickable {
        visible: Theme.compact
        anchors { left: logo.right; leftMargin: 10; right: toggle.left; rightMargin: 10
                  verticalCenter: parent.verticalCenter }
        height: 38
        contentWidth: strip.width; contentHeight: height
        clip: true; flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        Row {
            id: strip
            height: parent.height; spacing: 6
            Repeater {
                model: root.tabs
                Rectangle {
                    height: 34; width: lbl.width + 22; radius: 9
                    anchors.verticalCenter: parent.verticalCenter
                    property bool active: root.currentTab === index
                    color: active ? Theme.accent : Theme.field
                    Text { id: lbl; anchors.centerIn: parent; text: modelData
                           color: active ? "white" : Theme.muted; font.pixelSize: 13; font.bold: true }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.select(index) }
                }
            }
        }
    }
}
