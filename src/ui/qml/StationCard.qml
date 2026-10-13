import QtQuick
import QtQuick.Layouts
import HamClock

// Callsign, UTC and DE local clocks, DE/DX locators and path summary.
Card {
    id: root
    title: station.configured ? station.callsign : "Your station"
    signal setupRequested()
    signal settingsRequested()

    ColumnLayout {
        width: parent.width
        spacing: Theme.gap * 0.8

        ColumnLayout {
            spacing: 0
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: Theme.fontValue
                text: station.utcTime
                color: Theme.ink
                font.pixelSize: Theme.fontClock
                font.weight: Font.Light
                Accessible.name: "UTC time " + text
            }
            Text {
                text: station.utcDate + " · UTC"
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
            }
        }
        RowLayout {
            spacing: Theme.gap * 0.6
            Text {
                visible: station.deTime.length > 0
                text: station.deTime
                color: Theme.ink
                font.pixelSize: Theme.fontValue
            }
            Text {
                text: station.deTime.length > 0 ? "local" : ""
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
            }
        }
        Text {
            text: station.deZoneLabel
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle { color: Theme.divider; implicitHeight: 1; Layout.fillWidth: true }

        Repeater {
            model: station.configured ? [
                { label: "DE · Home", grid: station.deGrid, coords: station.deCoordinates },
                { label: "DX · Target", grid: station.dxGrid, coords: station.dxCoordinates }
            ] : []
            delegate: ColumnLayout {
                spacing: 1
                Layout.fillWidth: true
                Text { text: modelData.label; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text { text: modelData.grid; color: Theme.ink; font.pixelSize: Theme.fontTitle * 1.3; font.weight: Font.DemiBold }
                Text { text: modelData.coords; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
            }
        }
        Text {
            visible: station.configured
            text: station.distanceText + "  ·  " + station.bearingText
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        ColumnLayout {
            visible: !station.configured
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            Text {
                text: "Add your callsign and location to see locators, paths and local time."
                color: Theme.secondary
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            AppButton { text: "Set up"; primary: true; Layout.fillWidth: true; onClicked: root.setupRequested() }
        }
        AppButton {
            visible: Theme.dense && station.configured
            text: "⚙ Settings"
            Layout.fillWidth: true
            onClicked: root.settingsRequested()
        }
    }
}
