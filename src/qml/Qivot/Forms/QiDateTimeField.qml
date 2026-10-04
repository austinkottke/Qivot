import QtQuick 2.15
// A date and time as YYYY-MM-DD HH:MM.
QiTextField {
    placeholderText: "YYYY-MM-DD HH:MM"
    inputMethodHints: Qt.ImhDate | Qt.ImhTime
}