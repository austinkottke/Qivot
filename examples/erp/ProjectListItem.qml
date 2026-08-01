import QtQuick 2.15

// One row in the project directory.
Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : 300
    height: 66
    color: model.id === store.selectedProjectId ? "#EAF0FE" : (hover.containsMouse ? "#F6F8FB" : "white")
    Behavior on color { ColorAnimation { duration: 160 } }

    Rectangle { height: parent.height; color: Theme.accent
                width: model.id === store.selectedProjectId ? 3 : 0
                Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } } }

    Rectangle {
        id: dot
        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
        width: 10; height: 10; radius: 5; color: Theme.projectStatusColor(model.status)
    }
    Column {
        anchors { left: dot.right; leftMargin: 14; right: parent.right; rightMargin: 12
                  verticalCenter: parent.verticalCenter }
        spacing: 2
        Text { text: model.name; color: Theme.ink
               font.pixelSize: 14; font.bold: true; width: parent.width; elide: Text.ElideRight }
        Text { text: model.code + "  ·  " + store.clientName(model.client)
               color: Theme.muted; font.pixelSize: 12; width: parent.width; elide: Text.ElideRight }
    }
    Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.leftMargin: 16
                width: parent.width - 16; height: 1; color: "#EEF1F6" }
    MouseArea { id: hover; anchors.fill: parent; hoverEnabled: true
                onClicked: store.selectProject(model.id) }
}
