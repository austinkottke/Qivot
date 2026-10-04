// A field's box: its border and fill for focus, hover, an error or a warning.
import QtQuick 2.15

Rectangle {
    id: box
    property bool focused: false
    property bool hovered: false
    property bool hasError: false
    property bool hasWarning: false
    property bool active: true

    implicitHeight: QiTheme.fieldHeight
    radius: QiTheme.radius
    color: !active ? QiTheme.fieldDisabled
         : hasError ? Qt.tint(QiTheme.field, QiTheme.errorSoft)
         : hovered && !focused ? QiTheme.fieldHover : QiTheme.field
    border.width: focused || (hasError && !QiTheme.dark) ? 1.5 : 1
    border.color: hasError ? QiTheme.errorBorder
                : hasWarning ? QiTheme.warning
                : focused ? QiTheme.accent
                : hovered ? QiTheme.borderStrong : QiTheme.border
    Behavior on border.color { ColorAnimation { duration: QiTheme.duration } }
    Behavior on color { ColorAnimation { duration: QiTheme.duration } }

    // The focus ring.
    Rectangle {
        anchors.fill: parent
        anchors.margins: -4
        radius: box.radius + 4
        color: "transparent"
        border.width: 3
        border.color: box.hasError ? QiTheme.errorSoft : QiTheme.accentSoft
        opacity: box.focused ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: QiTheme.duration } }
    }
}
