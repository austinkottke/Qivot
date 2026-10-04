// A page: its title and note over a card that holds the form.
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot.Forms 1.0

ScrollView {
    id: page
    property string title
    property string note
    property string tag
    default property alias content: column.data
    clip: true
    // Laid out by the view's width, not what the scrollbar leaves: otherwise the
    // scrollbar coming and going re-wraps the text, which moves the scrollbar (Qt 5).
    contentWidth: width

    Item {
        width: page.width
        implicitHeight: body.height + 80

        // Columns, not layouts, out here: wrapped text in a layout sizes itself in a loop in Qt 5.
        Column {
            id: body
            width: Math.min(page.width - 48, 660)
            x: (page.width - width) / 2
            y: 40
            spacing: 22

            Column {
                width: parent.width
                spacing: 8
                Label {
                    text: page.tag
                    visible: text.length > 0
                    color: QiTheme.accent
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    font.capitalization: Font.AllUppercase
                }
                Label { text: page.title; color: QiTheme.text; font.pixelSize: 30; font.weight: Font.Bold }
                Label {
                    width: parent.width
                    text: page.note
                    color: QiTheme.textSecondary
                    font.pixelSize: 15
                    lineHeight: 1.25
                    wrapMode: Text.Wrap
                }
            }

            Rectangle {
                width: parent.width
                height: column.implicitHeight + 64
                radius: 18
                color: QiTheme.surface
                border.color: QiTheme.border

                ColumnLayout {
                    id: column
                    x: 32
                    y: 32
                    width: parent.width - 64
                    spacing: 20
                }
            }
        }
    }
}
