// A choice for a QiForm field. Its entries are the field's QiOneOf values unless
// you give `choices` yourself.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

QiFormRow {
    id: root
    property var choices: form && form.choices[field] !== undefined ? form.choices[field] : []
    property string placeholder: qsTr("Choose…")
    property alias combo: combo
    showValid: false

    ComboBox {
        id: combo
        Layout.fillWidth: true
        model: root.choices
        currentIndex: root.choices ? root.choices.indexOf(root.value) : -1
        displayText: currentIndex < 0 ? root.placeholder : currentText
        font.pixelSize: QiTheme.fontSize
        onActivated: if (root.form) { root.form.set(root.field, currentText); root.form.touch(root.field) }

        contentItem: Label {
            leftPadding: 14
            rightPadding: 36
            text: combo.displayText
            color: combo.currentIndex < 0 ? QiTheme.placeholder : QiTheme.text
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        indicator: Label {
            x: combo.width - width - 14
            y: (combo.height - height) / 2
            text: "⌄"
            color: QiTheme.textSecondary
            font.pixelSize: 18
            rotation: combo.popup.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: QiTheme.duration } }
        }
        background: QiFieldBackground {
            focused: combo.activeFocus || combo.popup.visible
            hovered: combo.hovered
            hasError: root.hasError
            hasWarning: root.hasWarning
        }
        delegate: ItemDelegate {
            width: combo.width - 8
            x: 4
            height: 38
            highlighted: combo.highlightedIndex === index
            contentItem: Label {
                text: modelData
                color: QiTheme.text
                font.pixelSize: QiTheme.fontSize
                verticalAlignment: Text.AlignVCenter
                leftPadding: 6
            }
            background: Rectangle {
                radius: 7
                color: highlighted ? QiTheme.accentSoft : "transparent"
            }
        }
        popup: Popup {
            y: combo.height + 6
            width: combo.width
            padding: 4
            implicitHeight: Math.min(contentItem.implicitHeight + 8, 320)
            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
            }
            background: Rectangle {
                radius: QiTheme.radius
                color: QiTheme.surface
                border.color: QiTheme.border
            }
            enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: QiTheme.duration } }
        }
    }
}
