// Sign up: every rule is in models.h (Member); this page only places the fields.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0
import Qivot.Forms 1.0

DemoPage {
    tag: "Live validation"
    title: "Create your account"
    note: "Errors show once you leave a field. The email is trimmed, lower-cased and checked against existing accounts — try taken@example.com. Team plans need a team name, and you must be 16 or over."

    QiForm {
        id: signup
        objectName: "signupForm"
        model: "Member"
        // A validator of your own, in JavaScript, next to the model's rules.
        validators: ({
            username: function(value, values) {
                if (value && values.name && value.toLowerCase() === values.name.toLowerCase().replace(/ /g, "_"))
                    return { message: "is just your name — fine, but easy to guess", warning: true }
                return ""
            },
            password: function(value, values) {
                return value && values.username && value.toLowerCase().indexOf(values.username.toLowerCase()) >= 0
                       ? "can't contain your username" : ""
            }
        })
        onSaved: function(savedId) { done.text = "Welcome aboard, " + signup.values.name + " — account #" + savedId + " is ready." }
    }

    Section { text: "Your details" }
    QiTextField { form: signup; field: "email"; placeholderText: "you@example.com"; inputMethodHints: Qt.ImhEmailCharactersOnly }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiTextField { form: signup; field: "name"; placeholderText: "Ada Lovelace" }
        // Any control can be wired with the attached properties instead:
        QiFormRow {
            form: signup
            field: "username"
            TextField {
                id: username
                objectName: "usernameField"
                Layout.fillWidth: true
                placeholderText: "letters, digits and _"
                font.pixelSize: QiTheme.fontSize
                color: QiTheme.text
                placeholderTextColor: QiTheme.placeholder
                leftPadding: 14
                QiForm.form: signup
                QiForm.field: "username"
                background: QiFieldBackground { focused: username.activeFocus; hovered: username.hovered; hasError: username.QiForm.hasError }
            }
        }
    }
    QiDateField { form: signup; field: "born" }

    Section { text: "Security" }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiPasswordField { form: signup; field: "password"; hint: "8+ characters, a capital, a small letter and a digit" }
        QiPasswordField { form: signup; field: "password_confirm" }
    }

    Section { text: "Plan" }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiComboBox  { form: signup; field: "plan" }
        QiTextField { form: signup; field: "team_name"; enabled: signup.values.plan === "Team"; opacity: enabled ? 1 : 0.45; placeholderText: "only for Team plans" }
    }
    QiCheckBox { form: signup; field: "terms" }

    QiErrorSummary { form: signup }
    Banner { id: done }
    RowLayout {
        spacing: 10
        QiSubmitButton { form: signup; text: "Create account" }
        QiButton { primary: false; text: "Start again"; onClicked: { signup.reset(); done.text = "" } }
    }
}
