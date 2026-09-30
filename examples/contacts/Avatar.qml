import QtQuick 2.15

/// Round monogram avatar with a soft top-lit gradient.
/// The color is stable per name, so a contact looks the same everywhere.
Rectangle {
    id: avatar
    property string first: ""
    property string last: ""
    property int size: 40

    width: size; height: size; radius: size / 2

    readonly property color base: {
        var colors = ["#FF3B30","#FF9500","#FFCC00","#34C759","#30B0C7",
                      "#007AFF","#5856D6","#AF52DE","#FF2D55","#A2845E"];
        var seed = first + last, h = 0;
        for (var i = 0; i < seed.length; i++) h = (h * 31 + seed.charCodeAt(i)) >>> 0;
        return colors[h % colors.length];
    }

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.lighter(avatar.base, 1.22) }
        GradientStop { position: 1.0; color: avatar.base }
    }

    Text {
        anchors.centerIn: parent
        text: (avatar.first.charAt(0) + avatar.last.charAt(0)).toUpperCase()
        color: "white"
        font.pixelSize: Math.round(avatar.size * 0.4)
        font.weight: Font.DemiBold
    }
}
