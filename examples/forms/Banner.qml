// A message after something worked.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot.Forms 1.0

Rectangle {
    id: banner
    property string text
    Layout.fillWidth: true
    visible: text.length > 0
    implicitHeight: visible ? label.implicitHeight + 26 : 0
    radius: QiTheme.radius
    color: QiTheme.successSoft
    border.color: Qt.rgba(QiTheme.success.r, QiTheme.success.g, QiTheme.success.b, 0.35)
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 200 } }
    RowLayout {
        x: 16
        y: 13
        width: parent.width - 32
        spacing: 10
        Rectangle {
            width: 20; height: 20; radius: 10
            color: QiTheme.success
            Label { anchors.centerIn: parent; text: "✓"; color: "white"; font.pixelSize: 12; font.bold: true }
        }
        Label { id: label; text: banner.text; color: QiTheme.text; font.pixelSize: 14; wrapMode: Text.Wrap; Layout.fillWidth: true }
    }
}
