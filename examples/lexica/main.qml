import QtQuick 2.15
import QtQuick.Controls 2.15
import Qivot 1.0

ApplicationWindow {
    id: win
    visible: true
    width: 960; height: 760
    color: "#0B0D13"
    title: "Lexica — Qivot full-text search"

    readonly property color accent: "#7CE0FF"

    SearchStore { id: store }

    function fmt(n) { return Number(n).toLocaleString(Qt.locale(), 'f', 0) }

    // Escape HTML, then wrap each typed word (as a prefix) in an accent color.
    function esc(s) { return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;') }
    function highlight(src, termsStr) {
        var out = esc(src)
        var words = termsStr.toLowerCase().split(/[^a-z0-9]+/)
        for (var i = 0; i < words.length; i++) {
            if (!words[i]) continue
            var esc1 = words[i].replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
            out = out.replace(new RegExp('(' + esc1 + '[a-z0-9]*)', 'ig'),
                              '<font color="' + accent + '">$1</font>')
        }
        return out
    }

    // ---------------- header + search box ----------------
    Column {
        id: head
        x: 40; y: 34; width: parent.width - 80; spacing: 16

        Column {
            spacing: 3
            Text { text: "Lexica"; color: "white"; font.pixelSize: 30; font.bold: true }
            Text { text: "Full-text search over " + fmt(store.totalCount) +
                         " entries — ranked, as you type."
                   color: "#7E8291"; font.pixelSize: 14 }
        }

        // search field
        Rectangle {
            width: parent.width; height: 56; radius: 14
            color: "#151824"
            border.color: search.activeFocus ? accent : "#242838"
            border.width: search.activeFocus ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: 150 } }

            Text { id: mag; text: "⌕"; color: "#6B7080"; font.pixelSize: 26
                   anchors { left: parent.left; leftMargin: 18; verticalCenter: parent.verticalCenter } }

            TextField {
                id: search
                anchors { left: mag.right; leftMargin: 10; right: clearBtn.left
                          verticalCenter: parent.verticalCenter }
                placeholderText: "Search… try \"cosmic ocean\", \"silent\", or \"memory\""
                color: "white"; font.pixelSize: 18
                placeholderTextColor: "#5A5F6E"
                background: null
                selectByMouse: true
                onTextChanged: debounce.restart()
                Component.onCompleted: forceActiveFocus()
            }
            Text {
                id: clearBtn
                anchors { right: parent.right; rightMargin: 18; verticalCenter: parent.verticalCenter }
                text: "✕"; color: "#6B7080"; font.pixelSize: 18
                visible: search.text.length > 0
                MouseArea { anchors.fill: parent; anchors.margins: -8
                            onClicked: { search.text = ""; store.search("") } }
            }
        }

        // stats / example chips
        Item {
            width: parent.width; height: 26
            // result stats
            Text {
                visible: store.terms.length > 0
                anchors.verticalCenter: parent.verticalCenter
                text: store.matchCount === 0
                      ? "No matches"
                      : fmt(store.matchCount) + " match" + (store.matchCount === 1 ? "" : "es") +
                        (store.matchCount > store.shown ? "  ·  showing top " + store.shown : "") +
                        "   ·   " + store.elapsedMs.toFixed(store.elapsedMs < 10 ? 2 : 1) + " ms"
                color: store.matchCount === 0 ? "#C56B7A" : "#7E8291"
                font.pixelSize: 13
                font.family: "Menlo, Consolas, monospace"
            }
            // example chips when empty
            Row {
                visible: store.terms.length === 0
                spacing: 8
                Text { text: "Try:"; color: "#5A5F6E"; font.pixelSize: 13
                       anchors.verticalCenter: parent.verticalCenter }
                Repeater {
                    model: ["cosmic ocean", "silent forest", "memory", "electric engine", "frozen"]
                    Rectangle {
                        radius: 13; height: 26; color: chipM.containsMouse ? "#22283A" : "#171B28"
                        border.color: "#242838"; width: chipT.width + 22
                        Text { id: chipT; anchors.centerIn: parent; text: modelData
                               color: accent; font.pixelSize: 12 }
                        MouseArea { id: chipM; anchors.fill: parent; hoverEnabled: true
                                    onClicked: { search.text = modelData; store.search(modelData) } }
                    }
                }
            }
        }
    }

    // ---------------- results ----------------
    ListView {
        id: list
        anchors { left: parent.left; right: parent.right; top: head.bottom; bottom: parent.bottom
                  leftMargin: 40; rightMargin: 40; topMargin: 20; bottomMargin: 12 }
        clip: true
        model: store.results
        cacheBuffer: 800
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            width: list.width; height: 84
            color: rowM.containsMouse ? "#12151F" : "transparent"
            radius: 10

            Column {
                anchors { left: parent.left; right: pill.left; verticalCenter: parent.verticalCenter
                          leftMargin: 14; rightMargin: 12 }
                spacing: 4
                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: win.highlight(title, store.terms)
                    textFormat: Text.StyledText
                    color: "#F0F1F6"; font.pixelSize: 17; font.bold: true
                }
                Text {
                    width: parent.width; elide: Text.ElideRight; maximumLineCount: 1
                    text: win.highlight(body, store.terms)
                    textFormat: Text.StyledText
                    color: "#9498A6"; font.pixelSize: 13
                }
            }

            // category pill
            Rectangle {
                id: pill
                anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                radius: 11; height: 22; width: pillT.width + 22; color: "#1B2030"
                border.color: "#2A3042"
                Text { id: pillT; anchors.centerIn: parent; text: category
                       color: "#8FA0C4"; font.pixelSize: 11; font.bold: true }
            }

            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1
                        color: "#161923"; visible: index < list.count - 1 }

            MouseArea { id: rowM; anchors.fill: parent; hoverEnabled: true }
        }

        // empty state
        Item {
            anchors.fill: parent
            visible: store.terms.length === 0
            Column {
                anchors.centerIn: parent; spacing: 10
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "⌕"
                       color: "#2C3243"; font.pixelSize: 64 }
                Text { anchors.horizontalCenter: parent.horizontalCenter
                       text: "Start typing to search " + fmt(store.totalCount) + " entries"
                       color: "#5A5F6E"; font.pixelSize: 15 }
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded; width: 6
            contentItem: Rectangle { implicitWidth: 4; radius: 2; color: "#40FFFFFF" }
        }
    }

    // 110ms debounce so we search after you pause, not on every keystroke.
    Timer { id: debounce; interval: 110; onTriggered: store.search(search.text) }
}
