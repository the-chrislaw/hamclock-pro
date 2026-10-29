import QtQuick
import HamClock

// Small pill: coloured dot + word, so state never relies on colour alone.
Rectangle {
    property string status: ""
    // Dot only (no word): used when the card title needs the width. The word stays in Accessible.name.
    property bool dotOnly: false
    Accessible.role: Accessible.StaticText
    Accessible.name: Theme.stateLabel(status)
    // Width with the word shown, independent of dotOnly, so a parent can decide without a binding loop.
    readonly property real fullWidth: (7 + 5) * Theme.scale + wordMetrics.advanceWidth + 12 * Theme.scale
    TextMetrics { id: wordMetrics; text: Theme.stateLabel(status); font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
    implicitWidth: dotOnly ? (7 + 12) * Theme.scale : fullWidth
    implicitHeight: 20 * Theme.scale
    radius: height / 2
    color: Qt.rgba(Theme.stateColor(status).r, Theme.stateColor(status).g, Theme.stateColor(status).b, 0.14)
    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5 * Theme.scale
        Rectangle {
            width: 7 * Theme.scale; height: width; radius: width / 2
            color: Theme.stateColor(parent.parent.status)
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: !parent.parent.dotOnly
            text: Theme.stateLabel(parent.parent.status)
            color: Theme.stateColor(parent.parent.status)
            font.pixelSize: Theme.fontCaption
            font.weight: Font.DemiBold
        }
    }
}
