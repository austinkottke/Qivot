// A field's row in a form: its label, the control (the content), and its error,
// warning or hint underneath. The other Qi… controls are built on it; use it
// directly to put any control of your own in a form.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: row
    /// The QiForm, and the field in it.
    property var form
    property string field
    property string label: form && form.labels[field] !== undefined ? form.labels[field] : field
    /// Shown under the control when there's no error or warning.
    property string hint: ""
    /// Show a tick once the field is filled in, visited and fine.
    property bool showValid: true
    readonly property string error: form && form.errors[field] !== undefined ? String(form.errors[field]) : ""
    readonly property string warning: form && form.warnings[field] !== undefined ? String(form.warnings[field]) : ""
    readonly property bool required: !!(form && form.required[field])
    readonly property var value: form ? form.values[field] : undefined
    readonly property bool hasError: error.length > 0
    readonly property bool hasWarning: !hasError && warning.length > 0
    readonly property bool visited: !!(form && form.visited[field])
    readonly property bool filled: value !== undefined && value !== null && String(value).length > 0
    readonly property bool isValid: showValid && visited && filled && !hasError && !hasWarning
    default property alias content: holder.data

    spacing: 7
    Layout.fillWidth: true
    Layout.preferredWidth: 100          // side by side, fields share the row evenly (not by their text's width)
    Layout.alignment: Qt.AlignTop      // fields side by side line up at their labels

    RowLayout {
        visible: row.label.length > 0
        spacing: 4
        Label {
            text: row.label
            color: QiTheme.textSecondary
            font.pixelSize: QiTheme.labelSize
            font.weight: Font.DemiBold
        }
        Label {
            text: "*"
            visible: row.required
            color: QiTheme.accent
            font.pixelSize: QiTheme.labelSize
            font.weight: Font.Bold
        }
    }

    ColumnLayout {
        id: holder
        Layout.fillWidth: true
        spacing: 0
    }

    // Error, warning or hint, fading in.
    Item {
        id: message
        readonly property string text: row.error || row.warning || row.hint
        Layout.fillWidth: true
        implicitHeight: text.length > 0 ? messageText.implicitHeight : 0
        Behavior on implicitHeight { NumberAnimation { duration: QiTheme.duration; easing.type: Easing.OutCubic } }
        clip: true

        RowLayout {
            width: parent.width
            spacing: 6
            opacity: message.text.length > 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: QiTheme.duration } }
            Rectangle {
                visible: row.hasError || row.hasWarning
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 2
                width: 14; height: 14; radius: 7
                color: row.hasError ? QiTheme.error : QiTheme.warning
                Label { anchors.centerIn: parent; text: "!"; color: QiTheme.surface; font.pixelSize: 10; font.bold: true }
            }
            Label {
                id: messageText
                Layout.fillWidth: true
                text: message.text
                wrapMode: Text.Wrap
                font.pixelSize: QiTheme.messageSize
                color: row.hasError ? QiTheme.error : row.hasWarning ? QiTheme.warning : QiTheme.textFaint
            }
        }
    }
}
