// Validates and saves the form: QiSubmitButton { form: signup; text: "Create account" }
import QtQuick 2.15

QiButton {
    property var form
    text: qsTr("Save")
    onClicked: if (form) form.submit()
}
