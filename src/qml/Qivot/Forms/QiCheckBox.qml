// A yes/no field, its label beside the box.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

QiFormRow {
    id: root
    property string text: ""
    property alias box: box
    label: ""
    showValid: false

    CheckBox {
        id: box
        Layout.fillWidth: true
        text: root.text.length ? root.text : (root.form ? root.form.labels[root.field] : "")
        checked: !!root.value
        font.pixelSize: QiTheme.fontSize
        spacing: 10
        padding: 0
        onToggled: if (root.form) { root.form.set(root.field, checked); root.form.touch(root.field) }

        indicator: Rectangle {
            implicitWidth: 22
            implicitHeight: 22
            x: box.leftPadding
            y: (box.height - height) / 2
            radius: 6
            color: box.checked ? QiTheme.accent : QiTheme.field
            border.width: box.checked ? 0 : 1.5
            border.color: root.hasError ? QiTheme.error : box.hovered ? QiTheme.borderStrong : QiTheme.border
            Behavior on color { ColorAnimation { duration: QiTheme.duration } }
            Label {
                anchors.centerIn: parent
                text: "✓"
                color: QiTheme.accentText
                font.pixelSize: 14
                font.bold: true
                visible: box.checked
            }
        }
        contentItem: Label {
            leftPadding: box.indicator.width + box.spacing
            text: box.text
            color: QiTheme.text
            font: box.font
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }
}
