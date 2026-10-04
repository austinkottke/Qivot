// A button that matches the forms: primary (filled) by default, or secondary.
import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
    id: button
    property bool primary: true
    property bool busy: false
    font.pixelSize: QiTheme.fontSize
    font.weight: Font.DemiBold
    leftPadding: 20
    rightPadding: 20
    implicitHeight: QiTheme.fieldHeight

    contentItem: Label {
        text: button.busy ? qsTr("Working…") : button.text
        font: button.font
        color: button.primary ? QiTheme.accentText : QiTheme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: button.enabled ? 1 : 0.55
    }
    background: Rectangle {
        radius: QiTheme.radius
        color: button.primary
               ? (button.down ? QiTheme.accentPressed : button.hovered ? QiTheme.accentHover : QiTheme.accent)
               : (button.down ? QiTheme.border : button.hovered ? QiTheme.surfaceHover : QiTheme.surface)
        border.width: button.primary ? 0 : 1
        border.color: QiTheme.border
        opacity: button.enabled ? 1 : 0.5
        Behavior on color { ColorAnimation { duration: QiTheme.duration } }
    }
}
