// A password field, with a button to show what's typed.
import QtQuick 2.15
import QtQuick.Controls 2.15

QiTextField {
    id: root
    property bool revealed: false
    echoMode: revealed ? TextInput.Normal : TextInput.Password
    trailingSpace: reveal.implicitWidth + 4

    Label {
        id: reveal
        parent: root.input
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: root.revealed ? qsTr("Hide") : qsTr("Show")
        color: area.containsMouse ? QiTheme.accentHover : QiTheme.accent
        font.pixelSize: 13
        font.weight: Font.DemiBold
        MouseArea {
            id: area
            anchors.fill: parent
            anchors.margins: -6
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.revealed = !root.revealed
        }
    }
}
