import QtQuick
import QtQuick.Layouts
import HamClock

// Callsign, UTC and DE local clocks, DE/DX locators and path summary.
Card {
    id: root
    // The banner already shows the callsign; do not repeat it in the title.
    title: !station.configured ? "Your station" : station.bannerIsCallsign ? "Station" : station.callsign
    property bool wide: false   // phone strip: DE and DX side by side
    // Phones: no title, settings as a gear beside the callsign, one-line date and
    // DE/DX summaries, so the panes and map get the room.
    property bool compact: false
    showTitle: !compact || !station.configured
    signal settingsRequested()
    signal timersRequested()
    onHeightChanged: tightLater.restart()
    // A short column (7-inch panels, when sunrise/sunset lines wrap) drops the coordinates and the
    // time zone hint rather than cutting off the lines below them. `dropped` is what they take
    // (their implicit height stays known while hidden), so hiding them cannot flip `tight` back.
    readonly property real dropped: zoneHint.implicitHeight + 2 * Theme.fontCaption * 1.3
    property bool tight: false
    Timer { id: tightLater; interval: 0; onTriggered: root.updateTight() }
    function updateTight() {
        const room = stationColumn.parent ? stationColumn.parent.height : 0
        tight = !compact && room > 0 && stationColumn.implicitHeight + (tight ? dropped : 0) > room + 1
    }

    ColumnLayout {
        id: stationColumn
        width: parent.width
        onImplicitHeightChanged: tightLater.restart()
        onParentChanged: tightLater.restart()
        spacing: Theme.gap * 0.8

        RowLayout {
        visible: station.configured
        Layout.fillWidth: true
        spacing: Theme.gap
        // Callsign banner: tap to cycle call / ON AIR / 73 DE call.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: bannerLabel.implicitHeight
            readonly property string fill: station.bannerBackground(appSettings.bannerBackground)
            color: fill.length ? fill : "transparent"
            radius: Theme.gap * 0.5
        Text {
            id: bannerLabel
            anchors.fill: parent
            horizontalAlignment: Text.AlignLeft
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Theme.fontValue
            text: station.bannerText
            // utcTime ticks every second, which animates the rainbow style.
            color: { station.utcTime; const c = station.bannerColor(appSettings.bannerStyle); return c.length ? c : Theme.accent }
            font.pixelSize: Theme.fontTitle * 1.6
            font.weight: Font.Bold
            Accessible.role: Accessible.Button
            Accessible.name: "Callsign banner " + text
            Accessible.onPressAction: station.cycleBanner()
            TapHandler { onTapped: station.cycleBanner() }
        }
        }
        AppButton {
            visible: root.compact
            text: "⚙"
            font.pixelSize: Theme.fontTitle * 1.3
            Accessible.name: "Settings"
            onClicked: root.settingsRequested()
        }
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
                // Low enough for the widest time ("12:05:01 AM", 10-12 o'clock in 12 h mode) in the
                // narrow 800x480 column; fontValue here cut it off with the Pi's wide font.
                minimumPixelSize: Theme.fontTitle
                text: clockBox.showLocal ? station.deClock : station.utcTime
                color: Theme.ink
                font.pixelSize: Theme.fontClock
                font.weight: Font.Light
                Accessible.name: (clockBox.showLocal ? "Local time " : "UTC time ") + text
            }
            Text {
                text: (clockBox.showLocal ? station.deDate + " · local" : station.utcDate + " · UTC")
                      + (root.compact && station.deTime.length > 0
                         ? "  ·  " + (clockBox.showLocal ? station.utcTime + " UTC" : station.deTime + " local") : "")
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            Text {
                visible: !root.compact && station.deTime.length > 0
                text: clockBox.showLocal ? station.utcTime + " UTC" : station.deTime + " local"
                color: Theme.secondary
                font.pixelSize: Theme.fontBody
            }
            TapHandler { onTapped: clockBox.local = !clockBox.local }
            // Long-press opens the stopwatch and countdown.
            TapHandler { onLongPressed: root.timersRequested() }
            Accessible.role: Accessible.Button
            Accessible.onPressAction: clockBox.local = !clockBox.local
        }
        // Tap to cycle day of year / JD / MJD / sidereal / UNIX.
        Text {
            visible: !root.compact
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
            id: zoneHint
            visible: !root.compact && !root.tight
            text: station.deZoneLabel
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle { visible: !root.compact; color: Theme.divider; implicitHeight: 1; Layout.fillWidth: true }

        GridLayout {
            Layout.fillWidth: true
            columns: root.wide ? 2 : 1
            columnSpacing: Theme.gap
            rowSpacing: Theme.gap * 0.8
        Repeater {
            // A count, not a list of the values: the values change every second (tick), and a
            // new list would rebuild these items each time, losing taps and showing unwrapped
            // lines until the next layout pass.
            model: !station.configured ? 0 : (station.hasDx ? 2 : 1)
            delegate: ColumnLayout {
                readonly property bool isDx: index === 1
                readonly property string label: isDx ? "DX · Target" : "DE · Home"
                readonly property string sun: isDx ? station.dxSun : station.deSun
                readonly property string local: isDx ? station.dxLocal : ""
                spacing: 1
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                Text { visible: !root.compact; text: parent.label; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text { text: (root.compact ? parent.label.split(" ")[0] + "  " : "") + (parent.isDx ? station.dxGrid : station.deGrid)
                       color: Theme.ink; font.pixelSize: Theme.fontTitle * (root.compact ? 1.05 : 1.3); font.weight: Font.DemiBold }
                Text { visible: !root.compact && !root.tight; text: parent.isDx ? station.dxCoordinates : station.deCoordinates
                       color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text {
                    visible: parent.local.length > 0
                    text: parent.local
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                Text {
                    visible: parent.sun.length > 0
                    text: "☀ " + parent.sun
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    Accessible.role: Accessible.Button
                    Accessible.name: text
                    Accessible.onPressAction: station.toggleSunAt()
                    TapHandler { onTapped: station.toggleSunAt() }
                }
            }
        }
        }
        Text {
            visible: station.configured
            text: root.compact ? station.pathText.split("\n").join(" · ") : station.pathText
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
                text: appSettings.loadError.length ? "Your saved station could not be read. Open Settings to see why."
                                                   : "Add callsign and location to see paths and local time."
                color: appSettings.loadError.length ? Theme.bad : Theme.secondary
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            AppButton { text: "Set up"; primary: true; Layout.fillWidth: true; onClicked: root.setupRequested() }
        }
        AppButton {
            visible: Theme.dense && station.configured && !root.compact
            text: "⚙ Settings"
            Layout.fillWidth: true
            onClicked: root.settingsRequested()
        }
    }
}
