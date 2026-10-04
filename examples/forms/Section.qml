// A section heading inside a card.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot.Forms 1.0

ColumnLayout {
    property alias text: title.text
    property string detail
    Layout.fillWidth: true
    Layout.topMargin: 6
    spacing: 4
    Label { id: title; color: QiTheme.text; font.pixelSize: 17; font.weight: Font.DemiBold }
    Label { text: parent.detail; visible: text.length > 0; color: QiTheme.textFaint; font.pixelSize: 13; wrapMode: Text.Wrap; Layout.fillWidth: true }
}
