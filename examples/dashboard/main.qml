import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 440; height: 820
    color: "#0B0B10"
    title: "Query Playground — Qivot"

    DashboardStore { id: store }

    // ---- Header ----
    Column {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top
                  leftMargin: 20; rightMargin: 20; topMargin: 18 }
        spacing: 3
        Text { text: "Query Playground"; color: "white"; font.pixelSize: 30; font.bold: true }
        Text { text: "One small query per step — code and live result"
               color: "#8A8A93"; font.pixelSize: 13 }
    }

    // ---- Steps ----
    ListView {
        id: steps
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom
                  topMargin: 14; leftMargin: 16; rightMargin: 16 }
        clip: true; model: store.recipes; spacing: 12; bottomMargin: 20

        delegate: Rectangle {
            width: steps.width; radius: 16; color: "#16161D"
            height: content.height + 28
            property bool isAsync: modelData.step === 8

            Column {
                id: content
                anchors { left: parent.left; right: parent.right; top: parent.top
                          leftMargin: 16; rightMargin: 16; topMargin: 14 }
                spacing: 10

                // step number + title
                Row {
                    spacing: 12; width: parent.width
                    Rectangle {
                        width: 28; height: 28; radius: 14; color: "#0A84FF"
                        anchors.verticalCenter: parent.verticalCenter
                        Text { anchors.centerIn: parent; text: modelData.step
                               color: "white"; font.pixelSize: 14; font.bold: true }
                    }
                    Text {
                        width: parent.width - 40; text: modelData.title
                        color: "white"; font.pixelSize: 16; font.bold: true
                        wrapMode: Text.WordWrap; anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // code
                Rectangle {
                    width: parent.width; radius: 8; color: "#0C0C12"; border.color: "#20202A"
                    height: code.height + 20
                    Text {
                        id: code
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
                        text: modelData.code; color: "#79C0FF"
                        font.family: "Menlo, monospace"; font.pixelSize: 12
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere; textFormat: Text.PlainText
                    }
                }

                // result — a plain string, or (step 8) a Run button + async result
                Row {
                    visible: !isAsync; spacing: 8
                    Text { text: "→"; color: "#30D158"; font.pixelSize: 14; font.bold: true }
                    Text { text: modelData.result; color: "#30D158"; font.pixelSize: 14; font.bold: true
                           width: content.width - 24; wrapMode: Text.WordWrap }
                }

                Row {
                    visible: isAsync; spacing: 12
                    Button {
                        text: store.asyncBusy ? "Running…" : "Run"; enabled: !store.asyncBusy
                        onClicked: store.runAsync()
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 13; font.bold: true
                                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        background: Rectangle { implicitWidth: 92; implicitHeight: 36; radius: 18
                                               color: parent.enabled ? "#0A84FF" : "#22222A" }
                    }
                    Text { text: store.asyncResult; color: "#30D158"; font.pixelSize: 13; font.bold: true
                           anchors.verticalCenter: parent.verticalCenter
                           width: content.width - 120; wrapMode: Text.WordWrap }
                }
            }
        }
    }
}
