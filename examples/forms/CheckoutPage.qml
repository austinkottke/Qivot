// Checkout: an order (Purchase) and its lines (OrderLine), checked together.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0
import Qivot.Forms 1.0

DemoPage {
    objectName: "checkoutPage"
    tag: "Cross-field & nested"
    title: "Checkout"
    note: "Each basket line is its own form, checked together with the order. Billing details only when they differ; cards checked with Luhn and their expiry; delivery 2–60 days out on a working day. An unknown promo code is only a warning."

    /// Check every basket line (each its own form). True if they're all fine.
    function validateLines() {
        var ok = true
        for (var i = 0; i < lines.count; ++i)
            ok = lines.itemAt(i).form.validate() && ok
        return ok
    }

    QiForm {
        id: checkout
        objectName: "checkoutForm"
        model: "Purchase"
        Component.onCompleted: set("billing_same", true)
    }

    Section { text: "Basket" }
    Repeater {
        id: lines
        model: [ { product: "Enamel mug", price: 12.5, qty: 2 },
                 { product: "Canvas tote", price: 18.99, qty: 1 },
                 { product: "Pens, pack of 10", price: 6.75, qty: 0 } ]
        delegate: RowLayout {
            property alias form: line
            Layout.fillWidth: true
            spacing: 14
            QiForm {
                id: line
                model: "OrderLine"
                Component.onCompleted: { set("product", modelData.product); set("unit_price", modelData.price); set("quantity", modelData.qty) }
            }
            Rectangle {
                Layout.alignment: Qt.AlignTop
                width: 44; height: 44; radius: 10
                color: QiTheme.accentSoft
                Label { anchors.centerIn: parent; text: modelData.product.charAt(0); color: QiTheme.accent; font.bold: true; font.pixelSize: 17 }
            }
            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 2
                spacing: 2
                Label { text: modelData.product; color: QiTheme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
                Label { text: "£" + modelData.price.toFixed(2) + " each"; color: QiTheme.textFaint; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            QiNumberField { form: line; field: "quantity"; label: ""; showValid: false; Layout.preferredWidth: 140; Layout.fillWidth: false }
        }
    }

    Section { text: "Delivery" }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiTextField { form: checkout; field: "ship_name" }
        QiTextField { form: checkout; field: "email" }
    }
    QiTextArea { form: checkout; field: "ship_address"; rows: 2 }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiTextField { form: checkout; field: "ship_postcode"; Layout.preferredWidth: 160; Layout.fillWidth: false }
        QiComboBox  { form: checkout; field: "ship_country" }
        QiDateField { form: checkout; field: "deliver_on"; Layout.preferredWidth: 190; Layout.fillWidth: false }
    }
    QiCheckBox { form: checkout; field: "billing_same" }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 20
        visible: !checkout.values.billing_same
        QiTextArea  { form: checkout; field: "bill_address"; rows: 2 }
        QiTextField { form: checkout; field: "bill_postcode" }
    }

    Section { text: "Payment" }
    QiTextField { form: checkout; field: "card_number"; placeholderText: "4111 1111 1111 1111"; inputMethodHints: Qt.ImhDigitsOnly }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiTextField { form: checkout; field: "card_expiry"; placeholderText: "MM/YY" }
        QiPasswordField { form: checkout; field: "cvc"; placeholderText: "123" }
        QiTextField { form: checkout; field: "promo"; placeholderText: "optional" }
    }

    QiErrorSummary { form: checkout }
    Banner { id: result }
    QiButton {
        text: "Pay now"
        onClicked: {
            var ok = validateLines()
            ok = checkout.validate() && ok
            result.text = ok && checkout.submit() ? "Payment accepted — order #" + checkout.recordId + "." : ""
        }
    }
}
