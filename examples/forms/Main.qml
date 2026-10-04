import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot.Forms 1.0

ApplicationWindow {
    id: window
    width: 980
    height: 900
    minimumWidth: 640
    visible: true
    title: "Qivot Forms"
    color: QiTheme.background
    Component.onCompleted: if (startTheme === "light") QiTheme.mode = QiTheme.Light; else if (startTheme === "dark") QiTheme.mode = QiTheme.Dark
    palette.window: QiTheme.background
    palette.windowText: QiTheme.text
    palette.text: QiTheme.text
    palette.base: QiTheme.field
    palette.button: QiTheme.surface
    palette.buttonText: QiTheme.text
    palette.highlight: QiTheme.accent

    header: Rectangle {
        implicitHeight: 64
        color: QiTheme.surface
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: QiTheme.border }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 16

            RowLayout {
                spacing: 10
                Rectangle {
                    width: 28; height: 28; radius: 8
                    gradient: Gradient {
                        GradientStop { position: 0; color: QiTheme.accent }
                        GradientStop { position: 1; color: Qt.darker(QiTheme.accent, 1.35) }
                    }
                    Label { anchors.centerIn: parent; text: "Q"; color: "white"; font.bold: true; font.pixelSize: 15 }
                }
                Label { text: "Qivot Forms"; color: QiTheme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
            }

            Item { Layout.fillWidth: true }

            Segmented {
                id: tabs
                objectName: "tabs"
                model: [ "Sign up", "Checkout", "Booking", "Wizard", "Inventory" ]
            }

            Item { Layout.fillWidth: true }

            Segmented {
                small: true
                model: [ "Auto", "Light", "Dark" ]
                currentIndex: QiTheme.mode
                onActivated: function(index) { QiTheme.mode = index }
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex
        SignUpPage {}
        CheckoutPage {}
        BookingPage {}
        WizardPage {}
        InventoryPage {}
    }
}
