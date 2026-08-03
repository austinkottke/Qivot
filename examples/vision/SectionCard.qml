import QtQuick 2.15

// A white rounded card with a title and a vertical content column. Child items
// declared inside a SectionCard flow into that column.
Rectangle {
    id: root
    property string title: ""
    property int    bodySpacing: 10
    default property alias content: col.data

    radius: 16; color: Theme.card; border.width: 1; border.color: Theme.border
    height: header.height + col.height + 46

    Text {
        id: header; text: root.title; color: Theme.ink; font.pixelSize: 16; font.bold: true
        anchors { left: parent.left; leftMargin: 20; top: parent.top; topMargin: 18 }
    }
    Column {
        id: col; spacing: root.bodySpacing
        anchors { left: parent.left; right: parent.right; top: header.bottom
                  leftMargin: 20; rightMargin: 20; topMargin: 12 }
    }
}
