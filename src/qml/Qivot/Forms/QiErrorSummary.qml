// Every problem at once, after a submit: QiErrorSummary { form: signup }
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: box
    property var form
    property string title: qsTr("A few things to fix")
    readonly property var lines: form && form.summary.length ? form.summary.split("\n") : []
    Layout.fillWidth: true
    visible: !!form && form.submitted && lines.length > 0
    implicitHeight: visible ? content.height + 28 : 0
    radius: QiTheme.radius
    color: QiTheme.errorSoft
    border.color: Qt.rgba(QiTheme.error.r, QiTheme.error.g, QiTheme.error.b, 0.35)

    Rectangle {
        width: 3
        radius: 2
        color: QiTheme.error
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; margins: 10 }
    }
    // A Column, not a layout: wrapped text in a layout sizes itself in a loop in Qt 5.
    Column {
        id: content
        x: 26
        y: 14
        width: box.width - 40
        spacing: 6
        Label { text: box.title; font.pixelSize: 14; font.weight: Font.DemiBold; color: QiTheme.error; width: parent.width }
        Repeater {
            model: box.lines
            Label { text: modelData; color: QiTheme.text; font.pixelSize: 13; wrapMode: Text.Wrap; width: content.width }
        }
    }
}
