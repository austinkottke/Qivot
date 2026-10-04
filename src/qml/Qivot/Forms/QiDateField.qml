import QtQuick 2.15
// A date as YYYY-MM-DD ("2026-02-30" is caught as not a date).
QiTextField {
    placeholderText: "YYYY-MM-DD"
    inputMethodHints: Qt.ImhDate
}