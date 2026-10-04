import QtQuick 2.15
// A number, typed (so "12.50" and "1,5" both work, and "abc" says so).
QiTextField {
    inputMethodHints: Qt.ImhFormattedNumbersOnly
}