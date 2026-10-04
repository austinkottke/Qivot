// Several lines of text for a QiForm field.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

QiFormRow {
    id: root
    property alias input: area
    property alias placeholderText: area.placeholderText
    property int rows: 3

    TextArea {
        id: area
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(implicitHeight, root.rows * 22 + 24)
        wrapMode: TextEdit.Wrap
        selectByMouse: true
        font.pixelSize: QiTheme.fontSize
        color: QiTheme.text
        placeholderTextColor: QiTheme.placeholder
        selectionColor: QiTheme.accentSoft
        selectedTextColor: QiTheme.text
        padding: 12
        leftPadding: 14
        text: root.value === undefined || root.value === null ? "" : String(root.value)
        onTextChanged: if (root.form && activeFocus && text !== String(root.value === undefined || root.value === null ? "" : root.value)) root.form.set(root.field, text)
        onActiveFocusChanged: if (!activeFocus && root.form) root.form.touch(root.field)
        background: QiFieldBackground {
            focused: area.activeFocus
            hovered: area.hovered
            hasError: root.hasError
            hasWarning: root.hasWarning
        }
    }
}
