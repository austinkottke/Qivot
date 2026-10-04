// Booking a room: opening hours, quarter hours, a length, and no overlaps (Booking).
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qivot 1.0
import Qivot.Forms 1.0

DemoPage {
    tag: "Dates & the database"
    title: "Book a room"
    note: "Weekdays from 08:00 to 20:00, on the quarter hour, 30 minutes to 4 hours — and never over another booking of the same room. Grace already has Atlas on the next weekday, 10:00–11:00. More than 12 people is a warning."

    QiForm {
        id: booking
        objectName: "bookingForm"
        model: "Booking"
        onSaved: { booked.revision++; result.text = "Booked " + booking.values.room + " for " + booking.values.organiser + "." }
        Component.onCompleted: {
            set("room", "Atlas")
            set("starts_at", demo.nextWeekday(14))
            set("ends_at", demo.nextWeekday(15))
            set("people", 4)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiComboBox  { form: booking; field: "room"; choices: demo.rooms() }
        QiTextField { form: booking; field: "organiser"; placeholderText: "Who's booking" }
    }
    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        QiDateTimeField { form: booking; field: "starts_at" }
        QiDateTimeField { form: booking; field: "ends_at" }
        QiNumberField   { form: booking; field: "people"; Layout.preferredWidth: 130; Layout.fillWidth: false }
    }

    QiErrorSummary { form: booking }
    Banner { id: result }
    QiSubmitButton { form: booking; text: "Book room" }

    Section { text: (booking.values.room || "") + " — already booked"; detail: "Bookings for this room, from the database." }
    Repeater {
        id: booked
        property int revision: 0
        model: demo.bookings(booking.values.room || "", revision)
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 52
            radius: QiTheme.radius
            color: QiTheme.dark ? "#11141a" : "#f7f8fb"
            border.color: QiTheme.border
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 12
                Rectangle { width: 8; height: 8; radius: 4; color: QiTheme.accent }
                Label { text: modelData.when; color: QiTheme.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Label { text: modelData.organiser; color: QiTheme.textSecondary; font.pixelSize: 14 }
            }
        }
    }
    Label { visible: booked.count === 0; text: "Nothing yet."; color: QiTheme.textFaint }
}
