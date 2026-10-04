// A text field for a QiForm field: QiTextField { form: signup; field: "email" }
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

QiFormRow {
    id: root
    property alias input: input
    property alias placeholderText: input.placeholderText
    property alias echoMode: input.echoMode
    property alias inputMethodHints: input.inputMethodHints
    property alias readOnly: input.readOnly
    /// Room on the right for something of yours (QiPasswordField's Show).
    property int trailingSpace: 0

    TextField {
        id: input
        Layout.fillWidth: true
        selectByMouse: true
        font.pixelSize: QiTheme.fontSize
        color: QiTheme.text
        placeholderTextColor: QiTheme.placeholder
        selectionColor: QiTheme.accentSoft
        selectedTextColor: QiTheme.text
        leftPadding: 14
        rightPadding: 14 + root.trailingSpace + (tick.visible ? 22 : 0)
        verticalAlignment: TextInput.AlignVCenter
        text: root.value === undefined || root.value === null ? "" : String(root.value)
        maximumLength: root.form && root.form.maxLengths[root.field] > 0 ? root.form.maxLengths[root.field] : 32767
        onTextEdited: if (root.form) root.form.set(root.field, text)
        onActiveFocusChanged: {
            if (activeFocus) return
            cursorPosition = 0                    // show the start of the text, not where typing ended
            if (root.form) root.form.touch(root.field)
        }
        onTextChanged: if (!activeFocus) cursorPosition = 0
        onAccepted: if (root.form) root.form.touch(root.field)
        background: QiFieldBackground {
            focused: input.activeFocus
            hovered: input.hovered
            hasError: root.hasError
            hasWarning: root.hasWarning
            active: input.enabled
        }

        Label {
            id: tick
            anchors.right: parent.right
            anchors.rightMargin: 14 + root.trailingSpace
            anchors.verticalCenter: parent.verticalCenter
            text: "✓"
            color: QiTheme.success
            font.pixelSize: 15
            font.bold: true
            visible: root.isValid && !input.activeFocus
        }
    }
}
