// The look of the Qivot.Forms controls: light and dark palettes that follow the
// system, and the sizes. Change it in one place:
//   QiTheme.mode = QiTheme.Dark          // or QiTheme.Light, QiTheme.Auto (default)
//   QiTheme.accent = "#0a84ff"
pragma Singleton
import QtQuick 2.15

QtObject {
    id: theme

    enum Mode { Auto, Light, Dark }
    property int mode: QiTheme.Auto

    // What the system says (Qt 6.5+ asks the platform; earlier Qt reads the palette).
    property SystemPalette system: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool systemDark: Qt.styleHints.colorScheme !== undefined && Qt.styleHints.colorScheme !== 0
                                       ? Qt.styleHints.colorScheme === 2
                                       : system.window.hslLightness < 0.5
    readonly property bool dark: mode === QiTheme.Dark || (mode === QiTheme.Auto && systemDark)

    // --- Colours ----------------------------------------------------------
    property color accent: dark ? "#7b93ff" : "#4f62e8"
    readonly property color accentHover: Qt.lighter(accent, dark ? 1.12 : 1.08)
    readonly property color accentPressed: Qt.darker(accent, 1.15)
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.22 : 0.16)
    readonly property color accentText: "#ffffff"

    readonly property color background:   dark ? "#0e1015" : "#f4f5f8"
    readonly property color surface:      dark ? "#161920" : "#ffffff"
    readonly property color surfaceHover: dark ? "#1c2029" : "#f7f8fb"
    readonly property color field:        dark ? "#0f1218" : "#ffffff"
    readonly property color fieldHover:   dark ? "#131720" : "#fbfbfd"
    readonly property color fieldDisabled: dark ? "#14171d" : "#f1f2f5"
    readonly property color border:       dark ? "#272c37" : "#dfe2e8"
    readonly property color borderStrong: dark ? "#363c4a" : "#c7ccd5"

    readonly property color text:          dark ? "#e8eaf0" : "#141824"
    readonly property color textSecondary: dark ? "#a0a7b5" : "#525a6b"
    readonly property color textFaint:     dark ? "#6b7280" : "#9aa1ad"
    readonly property color placeholder:   dark ? "#5c6371" : "#a3a9b4"

    property color error:   dark ? "#ec8a8a" : "#d33a3a"     // a softer coral on dark backgrounds
    property color warning: dark ? "#f2b155" : "#b4690e"
    property color success: dark ? "#43d39a" : "#18915e"
    readonly property color errorSoft:   Qt.rgba(error.r, error.g, error.b, dark ? 0.06 : 0.07)
    /// A field's border when it has an error: quieter than the message on dark.
    readonly property color errorBorder: Qt.rgba(error.r, error.g, error.b, dark ? 0.6 : 1)
    readonly property color warningSoft: Qt.rgba(warning.r, warning.g, warning.b, dark ? 0.14 : 0.09)
    readonly property color successSoft: Qt.rgba(success.r, success.g, success.b, dark ? 0.14 : 0.09)

    // --- Sizes ------------------------------------------------------------
    property int radius: 10
    property int fieldHeight: 44
    property int fontSize: 15
    property int labelSize: 13
    property int messageSize: 12
    property int duration: 140
}
