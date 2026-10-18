import QtQuick
import QtQuick.Layouts
import HamClock

// Big Clock: UTC and local time in the largest type that fits, opened from the
// Timers page. Tap anywhere (or press Escape) to close.
Item {
    id: root
    signal closeRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.pad
        spacing: Theme.gap

        Item { Layout.fillHeight: true }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontValue
            Layout.preferredHeight: root.height * 0.38
            text: station.utcTime
            color: Theme.ink
            font.pixelSize: root.height
            font.weight: Font.Light
            Accessible.name: "UTC time " + text
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: station.utcDate + " · UTC"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody + 4
        }
        Text {
            Layout.fillWidth: true
            visible: station.deTime.length > 0
            horizontalAlignment: Text.AlignHCenter
            fontSizeMode: Text.Fit
            minimumPixelSize: Theme.fontValue
            Layout.preferredHeight: root.height * 0.2
            text: station.deClock
            color: Theme.accent
            font.pixelSize: root.height
            font.weight: Font.Light
            Accessible.name: "Local time " + text
        }
        Text {
            Layout.fillWidth: true
            visible: station.deTime.length > 0
            horizontalAlignment: Text.AlignHCenter
            text: station.deDate + " · " + station.deZoneLabel
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: station.auxTime === station.lstTime ? station.auxTime : station.lstTime + "  ·  " + station.auxTime
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
        Item { Layout.fillHeight: true }
    }

    TapHandler { onTapped: root.closeRequested() }
    Accessible.role: Accessible.Button
    Accessible.name: "Big clock, tap to close"
    Accessible.onPressAction: root.closeRequested()
}
