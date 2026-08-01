import QtQuick 2.15

// One row in the client directory.
Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : 300
    height: 64
    color: model.id === store.selectedClientId ? "#EAF0FE" : (hover.containsMouse ? "#F6F8FB" : "white")
    Behavior on color { ColorAnimation { duration: 160 } }

    Rectangle { height: parent.height; color: Theme.accent
                width: model.id === store.selectedClientId ? 3 : 0
                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } } }

    Rectangle {
        id: av
        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
        width: 40; height: 40; radius: 20; color: Theme.avatarColor(model.id)
        scale: hover.containsMouse ? 1.08 : 1.0
        Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutBack } }
        Text { anchors.centerIn: parent; text: Theme.initials(model.name)
               color: "white"; font.pixelSize: 15; font.bold: true }
    }
    Column {
        anchors { left: av.right; leftMargin: 12; right: parent.right; rightMargin: 12
                  verticalCenter: parent.verticalCenter }
        spacing: 2
        Text { text: model.name; color: Theme.ink
               font.pixelSize: 15; font.bold: true; width: parent.width; elide: Text.ElideRight }
        Text { text: model.industry + "  ·  " + model.city + ", " + model.state
               color: Theme.muted; font.pixelSize: 12; width: parent.width; elide: Text.ElideRight }
    }
    Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.leftMargin: 16
                width: parent.width - 16; height: 1; color: "#EEF1F6" }
    MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                onClicked: store.selectClient(model.id) }
}
