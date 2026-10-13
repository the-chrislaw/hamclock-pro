import QtQuick
import HamClock

// Small pill: coloured dot + word, so state never relies on colour alone.
Rectangle {
    property string status: ""
    implicitWidth: row.implicitWidth + 12 * Theme.scale
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
            text: Theme.stateLabel(parent.parent.status)
            color: Theme.stateColor(parent.parent.status)
            font.pixelSize: Theme.fontCaption
            font.weight: Font.DemiBold
        }
    }
}
