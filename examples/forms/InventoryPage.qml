// An editable table: every row is a QiForm over one Product, errors under each cell.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0
import Qivot.Forms 1.0

DemoPage {
    tag: "A form per row"
    title: "Inventory"
    note: "Each row is its own form, loaded by id and saved on its own. SKUs are upper-cased and unique, prices over 5,000 get a warning, and stock can't go negative."

    property var rows: demo.products()

    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Repeater {
            model: [ { t: "SKU", w: 130 }, { t: "Name", w: -1 }, { t: "Price", w: 110 }, { t: "Stock", w: 90 }, { t: "", w: 96 } ]
            Label {
                text: modelData.t
                color: QiTheme.textFaint
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
                font.capitalization: Font.AllUppercase
                Layout.preferredWidth: modelData.w > 0 ? modelData.w : -1
                Layout.fillWidth: modelData.w < 0
            }
        }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: QiTheme.border }

    ListView {
        id: list
        objectName: "inventoryList"
        Layout.fillWidth: true
        Layout.preferredHeight: contentHeight
        interactive: false
        spacing: 10
        model: rows
        delegate: RowLayout {
            width: list.width
            spacing: 12
            QiForm {
                id: product
                model: "Product"
                Component.onCompleted: if (modelData !== null) load(modelData)
            }
            QiTextField   { form: product; field: "sku"; label: ""; showValid: false; Layout.preferredWidth: 130; Layout.fillWidth: false; Layout.alignment: Qt.AlignTop }
            QiTextField   { form: product; field: "name"; label: ""; showValid: false; Layout.alignment: Qt.AlignTop }
            QiNumberField { form: product; field: "price"; label: ""; showValid: false; Layout.preferredWidth: 110; Layout.fillWidth: false; Layout.alignment: Qt.AlignTop }
            QiNumberField { form: product; field: "stock"; label: ""; showValid: false; Layout.preferredWidth: 90; Layout.fillWidth: false; Layout.alignment: Qt.AlignTop }
            QiButton {
                Layout.preferredWidth: 96
                Layout.alignment: Qt.AlignTop
                primary: product.dirty
                text: product.dirty ? "Save" : (product.isNew ? "New" : "Saved ✓")
                enabled: product.dirty
                onClicked: product.submit()
            }
        }
    }

    QiButton {
        primary: false
        text: "+  Add a product"
        onClicked: { var r = rows.slice(); r.push(null); rows = r }
    }
}
