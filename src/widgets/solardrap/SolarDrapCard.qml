import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    // The band table (two bands per row when short) needs about 160 px of stage.
    readonly property bool full: root.stacked && drapColumn.parent !== null && drapColumn.parent.height >= 160 * Theme.scale
    readonly property bool ok: root.widget && root.widget.hasData
    readonly property real mhz: ok ? root.widget.peakMhz : 0
    function tone(f) { return f >= 15 ? Theme.bad : f >= 5 ? Theme.warn : Theme.good }
    // HF bands (lower edge, MHz) checked against the peak absorbed frequency in the tall layout.
    readonly property var hfBands: [
        { band: "160m", mhz: 1.8 }, { band: "80m", mhz: 3.5 }, { band: "60m", mhz: 5.3 },
        { band: "40m", mhz: 7.0 }, { band: "30m", mhz: 10.1 }, { band: "20m", mhz: 14.0 },
        { band: "17m", mhz: 18.068 }, { band: "15m", mhz: 21.0 }, { band: "12m", mhz: 24.89 },
        { band: "10m", mhz: 28.0 }
    ]
    // Tall pane: the peak with its message, then every HF band marked absorbed or clear, rows spread over the height.
    ColumnLayout {
        id: drapColumn
        visible: root.full
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.4
        Metric {
            label: "Max absorbed"
            available: root.ok
            value: available ? root.mhz.toFixed(1) + " MHz" : ""
            valueColor: available ? root.tone(root.mhz) : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: root.ok
            text: root.ok ? root.widget.message : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        // Bands in one column; two (band and state only) when the pane is too short for ten readable rows.
        GridLayout {
            id: bandGrid
            readonly property bool twoUp: drapColumn.parent.height < 330 * Theme.scale
            visible: root.ok
            columns: twoUp ? 2 : 1
            columnSpacing: Theme.gap * 2
            rowSpacing: 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Repeater {
                model: root.ok ? root.hfBands : []
                Item {
                    required property var modelData
                    readonly property bool absorbed: modelData.mhz <= root.mhz
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    Layout.preferredHeight: Theme.fontBody * 1.3
                    Layout.minimumHeight: Theme.fontBody * 1.1
                    Layout.maximumHeight: Theme.fontBody * 3
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.gap
                        Text { id: bandName; width: 44 * Theme.scale; text: modelData.band; color: Theme.ink
                               font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                        Text { visible: !bandGrid.twoUp; width: 64 * Theme.scale; text: modelData.mhz.toFixed(1) + " MHz"
                               color: Theme.secondary; font.pixelSize: Theme.fontCaption; anchors.baseline: bandName.baseline }
                    }
                    Text {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        text: parent.absorbed ? "absorbed" : "clear"
                        color: parent.absorbed ? root.tone(root.mhz) : Theme.good
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                    }
                }
            }
        }
        Text {
            text: root.ok ? "≥ 1 dB loss where worst · " + root.widget.observed : "D-RAP unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    // Plain Column/Item (no Layout) as in the other compact cards.
    Column {
        visible: !root.full
        width: parent.width
        spacing: Theme.gap * 0.6
        Text {
            width: parent.width
            text: root.ok ? "Max " + root.mhz.toFixed(1) + " MHz" : "—"
            color: root.ok ? root.tone(root.mhz) : Theme.secondary
            font.pixelSize: Theme.fontBody
            font.bold: true
            elide: Text.ElideRight
        }
        Text {
            width: parent.width
            text: root.ok ? (root.compact ? "" : root.widget.message + " · ") + "D-RAP · " + root.widget.observed : "D-RAP unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: root.compact ? Text.NoWrap : Text.WordWrap
            elide: Text.ElideRight
        }
    }
}
