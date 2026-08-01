import QtQuick 2.15
import QtQuick.Controls 2.15

Popup {
    id: root
    property int  projectId: 0
    property bool lockProject: false
    anchors.centerIn: Overlay.overlay
    width: 420; padding: 0; modal: true
    background: Rectangle { radius: 16; color: "white"; border.color: Theme.border }
    enter: Transition { NumberAnimation { property: "scale"; from: 0.9; to: 1; duration: 220; easing.type: Easing.OutBack }
                        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 120 } }
    Overlay.modal: Rectangle { color: "#33101828" }

    Column {
        width: parent.width
        Rectangle { width: parent.width; height: 56; color: "#FAFBFD"; radius: 16
            Text { anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                   text: "Log expense"; color: Theme.ink; font.pixelSize: 16; font.bold: true } }
        Column {
            width: parent.width; padding: 20; spacing: 12

            Column { spacing: 5; width: parent.width - 40; visible: root.lockProject
                Text { text: "Project"; color: Theme.muted; font.pixelSize: 12 }
                Text { text: store.projectName(root.projectId); color: Theme.ink; font.pixelSize: 14; font.bold: true } }
            Column { spacing: 5; width: parent.width - 40; visible: !root.lockProject
                Text { text: "Project"; color: Theme.muted; font.pixelSize: 12 }
                ComboBox { id: cbProject; width: parent.width; model: store.projects
                           textRole: "name"; valueRole: "id" } }

            Column { spacing: 5; width: parent.width - 40
                Text { text: "Employee"; color: Theme.muted; font.pixelSize: 12 }
                ComboBox { id: cbEmployee; width: parent.width; model: store.employees
                           textRole: "name"; valueRole: "id" } }
            Row { spacing: 12; width: parent.width - 40
                Column { spacing: 5; width: (parent.width - 12) / 2
                    Text { text: "Date"; color: Theme.muted; font.pixelSize: 12 }
                    TextField { id: fDate; width: parent.width; text: store.todayIso()
                                background: Rectangle { radius: 8; color: Theme.field } } }
                Column { spacing: 5; width: (parent.width - 12) / 2
                    Text { text: "Amount ($)"; color: Theme.muted; font.pixelSize: 12 }
                    TextField { id: fAmount; width: parent.width; placeholderText: "120"
                                validator: DoubleValidator { bottom: 0 }
                                background: Rectangle { radius: 8; color: Theme.field } } } }
            Row { spacing: 12; width: parent.width - 40
                Column { spacing: 5; width: (parent.width - 12) / 2
                    Text { text: "Category"; color: Theme.muted; font.pixelSize: 12 }
                    ComboBox { id: cbCategory; width: parent.width
                               model: [ "Travel","Meals","Printing","Mileage","Lodging","Software","Supplies","Other" ] } }
                Column { spacing: 5; width: (parent.width - 12) / 2; topPadding: 18
                    CheckBox { id: cbBillable; checked: true; text: "Billable" } }
            }
            Row { spacing: 12; anchors.right: parent.right; anchors.rightMargin: 20
                Rectangle { width: 90; height: 40; radius: 10; color: Theme.field
                    Text { anchors.centerIn: parent; text: "Cancel"; color: Theme.muted; font.pixelSize: 14 }
                    MouseArea { anchors.fill: parent; onClicked: root.close() } }
                Rectangle { width: 130; height: 40; radius: 10; color: Theme.accent
                    Text { anchors.centerIn: parent; text: "Log expense"; color: "white"; font.pixelSize: 14; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: {
                        var pid = root.lockProject ? root.projectId : cbProject.currentValue
                        store.addExpense(pid, cbEmployee.currentValue, fDate.text,
                                         parseFloat(fAmount.text) || 0, cbCategory.currentText, cbBillable.checked)
                        fAmount.text = ""
                        root.close()
                    } } }
            }
        }
    }
}
