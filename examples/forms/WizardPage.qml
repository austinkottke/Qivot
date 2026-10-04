// A three-step application (Applicant): each step checks only its own fields.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0
import Qivot.Forms 1.0

DemoPage {
    id: page
    objectName: "wizardPage"
    tag: "One step at a time"
    title: "Apply"
    note: "Next checks only that step's fields (validateFields), so step 2 never complains while you're on step 1. A US address needs a state, the UK doesn't use one; employment details only if you're employed."

    property int step: 0
    readonly property var steps: [
        [ "first_name", "last_name", "email", "born" ],
        [ "country", "state", "postcode", "phone" ],
        [ "employed", "employer", "started", "income" ]
    ]

    QiForm {
        id: wizard
        objectName: "wizardForm"
        model: "Applicant"
        onSaved: function(savedId) { result.text = "Application #" + savedId + " sent — we'll be in touch." }
    }

    // The steps.
    RowLayout {
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            model: [ "About you", "Address", "Work" ]
            RowLayout {
                Layout.fillWidth: index < 2
                spacing: 10
                readonly property bool done: index < page.step
                readonly property bool current: index === page.step
                Rectangle {
                    width: 30; height: 30; radius: 15
                    color: parent.current || parent.done ? QiTheme.accent : "transparent"
                    border.width: parent.current || parent.done ? 0 : 1.5
                    border.color: QiTheme.borderStrong
                    Behavior on color { ColorAnimation { duration: 200 } }
                    Label {
                        anchors.centerIn: parent
                        text: parent.parent.done ? "✓" : index + 1
                        color: parent.parent.current || parent.parent.done ? QiTheme.accentText : QiTheme.textSecondary
                        font.pixelSize: 13
                        font.bold: true
                    }
                }
                Label {
                    text: modelData
                    color: parent.current ? QiTheme.text : QiTheme.textSecondary
                    font.pixelSize: 14
                    font.weight: parent.current ? Font.DemiBold : Font.Normal
                }
                Rectangle {
                    visible: index < 2
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    height: 2
                    radius: 1
                    color: parent.done ? QiTheme.accent : QiTheme.border
                }
            }
        }
    }

    StackLayout {
        Layout.fillWidth: true
        currentIndex: page.step
        ColumnLayout {
            spacing: 20
            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                QiTextField { form: wizard; field: "first_name" }
                QiTextField { form: wizard; field: "last_name" }
            }
            QiTextField { form: wizard; field: "email"; placeholderText: "you@example.com" }
            QiDateField { form: wizard; field: "born" }
        }
        ColumnLayout {
            spacing: 20
            QiComboBox  { form: wizard; field: "country" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                QiTextField { form: wizard; field: "state"; visible: wizard.values.country === "United States" }
                QiTextField { form: wizard; field: "postcode" }
            }
            QiTextField { form: wizard; field: "phone"; placeholderText: "+1 202 555 0143"; inputMethodHints: Qt.ImhDialableCharactersOnly }
        }
        ColumnLayout {
            spacing: 20
            QiCheckBox  { form: wizard; field: "employed" }
            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                visible: !!wizard.values.employed
                QiTextField { form: wizard; field: "employer" }
                QiDateField { form: wizard; field: "started" }
            }
            QiNumberField { form: wizard; field: "income"; placeholderText: "0.00" }
        }
    }

    QiErrorSummary { form: wizard; visible: page.step === 2 && wizard.submitted && wizard.summary.length > 0 }
    Banner { id: result }
    RowLayout {
        Layout.fillWidth: true
        spacing: 10
        QiButton { primary: false; text: "Back"; enabled: page.step > 0; onClicked: page.step-- }
        Item { Layout.fillWidth: true }
        QiButton {
            text: page.step < 2 ? "Continue" : "Send application"
            onClicked: {
                if (!wizard.validateFields(page.steps[page.step]))
                    return
                if (page.step < 2) page.step++
                else wizard.submit()
            }
        }
    }
}
