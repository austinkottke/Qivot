// A row of choices with the current one raised: the page switcher, the theme toggle.
import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot.Forms 1.0

Rectangle {
    id: control
    property var model: []
    property int currentIndex: 0
    property bool small: false
    signal activated(int index)

    implicitWidth: row.implicitWidth + 8
    implicitHeight: small ? 32 : 38
    radius: height / 2
    color: QiTheme.dark ? "#0b0d11" : "#eceef3"
    border.color: QiTheme.border

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 2
        Repeater {
            model: control.model
            Rectangle {
                readonly property bool selected: index === control.currentIndex
                width: label.implicitWidth + (control.small ? 22 : 30)
                height: control.height - 8
                radius: height / 2
                color: selected ? QiTheme.surface : (area.containsMouse ? QiTheme.surfaceHover : "transparent")
                border.color: selected ? QiTheme.border : "transparent"
                Behavior on color { ColorAnimation { duration: QiTheme.duration } }
                Label {
                    id: label
                    anchors.centerIn: parent
                    text: modelData
                    color: parent.selected ? QiTheme.text : QiTheme.textSecondary
                    font.pixelSize: control.small ? 12 : 14
                    font.weight: parent.selected ? Font.DemiBold : Font.Normal
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { control.currentIndex = index; control.activated(index) }
                }
            }
        }
    }
}
