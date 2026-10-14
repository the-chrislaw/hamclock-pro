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

        // Callsign banner: tap to cycle call / ON AIR / 73 DE call.
        Text {
            visible: station.configured
            Layout.fillWidth: true
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Theme.fontValue
            text: station.bannerText
            color: Theme.accent
            font.pixelSize: Theme.fontTitle * 1.6
            font.weight: Font.Bold
            Accessible.role: Accessible.Button
            Accessible.name: "Callsign banner " + text
            Accessible.onPressAction: station.cycleBanner()
            TapHandler { onTapped: station.cycleBanner() }
        }

        // Tap the clock to swap the big clock between UTC and DE local time.
        ColumnLayout {
            id: clockBox
            property bool local: false
            readonly property bool showLocal: local && station.deClock.length > 0
            spacing: 0
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: Theme.fontValue
                text: clockBox.showLocal ? station.deClock : station.utcTime
                color: Theme.ink
                font.pixelSize: Theme.fontClock
                font.weight: Font.Light
                Accessible.name: (clockBox.showLocal ? "Local time " : "UTC time ") + text
            }
            Text {
                text: clockBox.showLocal ? station.deDate + " · local" : station.utcDate + " · UTC"
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
            }
            Text {
                visible: station.deTime.length > 0
                text: clockBox.showLocal ? station.utcTime + " UTC" : station.deTime + " local"
                color: Theme.secondary
                font.pixelSize: Theme.fontBody
            }
            TapHandler { onTapped: clockBox.local = !clockBox.local }
            Accessible.role: Accessible.Button
            Accessible.onPressAction: clockBox.local = !clockBox.local
        }
        // Tap to cycle day of year / JD / MJD / sidereal / UNIX.
        Text {
            Layout.fillWidth: true
            text: station.auxTime
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Accessible.role: Accessible.Button
            Accessible.name: "Auxiliary time " + text
            Accessible.onPressAction: station.cycleAuxTime()
            TapHandler { onTapped: station.cycleAuxTime() }
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
                { label: "DE · Home", grid: station.deGrid, coords: station.deCoordinates, sun: station.deSun },
                { label: "DX · Target", grid: station.dxGrid, coords: station.dxCoordinates, sun: station.dxSun }
            ] : []
            delegate: ColumnLayout {
                spacing: 1
                Layout.fillWidth: true
                Text { text: modelData.label; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text { text: modelData.grid; color: Theme.ink; font.pixelSize: Theme.fontTitle * 1.3; font.weight: Font.DemiBold }
                Text { text: modelData.coords; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text {
                    visible: modelData.sun.length > 0
                    text: "☀ " + modelData.sun
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }
        Text {
            visible: station.configured
            text: station.pathText
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            TapHandler { onTapped: station.toggleLongPath() }
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
