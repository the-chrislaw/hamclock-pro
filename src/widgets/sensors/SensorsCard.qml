import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property var w: root.widget
    // One entry per reading: shown, value, caption (full layouts) and short caption (compact slot).
    // CPU temp always shows (an em dash when the host has no sensor); the others only when present.
    readonly property var readings: !w ? [] : [
        { label: "CPU temp", shown: true, available: w.hasCpuTemp, value: w.hasCpuTemp ? w.cpuTemp : "",
          caption: w.hasCpuTemp ? w.cpuNote + (w.cpuPeak !== "" ? " · " + w.cpuPeak : "") : "", shortCaption: "" },
        { label: "Air temp", shown: w.envTemp !== "", available: true, value: w.envTemp, caption: w.envTempRange, shortCaption: "" },
        { label: "Pressure", shown: w.envPressure !== "", available: true, value: w.envPressure, caption: w.envTrend, shortCaption: w.envTrend },
        { label: "Humidity", shown: w.envHumidity !== "", available: true, value: w.envHumidity,
          caption: w.envDewPoint !== "" ? "Dew point " + w.envDewPoint : "", shortCaption: "" },
        { label: "Wi-Fi", shown: w.hasWifi, available: true, value: w.wifi, caption: w.wifiLow, shortCaption: "" },
        // "Synced, <1 ms, stratum 2, server": status is the value, the rest a caption that elides.
        { label: "Clock", shown: w.hasClock, available: true, value: w.clock.split(", ")[0],
          caption: w.clock.split(", ").slice(1).join(", "), shortCaption: w.clock.split(", ").slice(1).join(", ") },
        { label: "Disk", shown: w.hasDisk, available: true, value: w.disk, caption: w.diskFree, shortCaption: "" }
    ].filter(function (r) { return r.shown })
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || (compact && height >= 260 * Theme.scale)
    // Stacked: the readings in one or two columns, rows spread over the height; the design width is the
    // grid's own, so the card grows it until it fills the pane (never past the compact width).
    designWidth: stacked ? Math.min(260 * Theme.scale, Math.max(150 * Theme.scale, grid.implicitWidth))
                      : (compact ? 260 : 420) * Theme.scale
    onDesignWidthChanged: Qt.callLater(refit)
    GridLayout {
        id: grid
        visible: root.stacked
        width: parent.width
        height: parent.height
        // A few readings stack in one column; more share two.
        columns: root.readings.length <= 3 ? 1 : 2
        columnSpacing: 12 * Theme.scale
        rowSpacing: Theme.gap
        Repeater {
            model: root.stacked ? root.readings : []
            // Each reading sits at the top of its cell so the labels in a row line up.
            delegate: Item {
                required property var modelData
                implicitWidth: reading.implicitWidth
                implicitHeight: reading.implicitHeight
                // The clock line is long: it takes a full row.
                Layout.columnSpan: modelData.label === "Clock" ? grid.columns : 1
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: implicitHeight * 1.6
                Metric {
                    id: reading
                    width: parent.width
                    label: modelData.label
                    available: modelData.available
                    value: modelData.value
                    caption: modelData.caption
                }
            }
        }
    }
    Flow {
        visible: !root.stacked
        width: parent.width
        spacing: 12 * Theme.scale
        Repeater {
            model: root.stacked ? [] : root.readings
            delegate: Metric {
                required property var modelData
                label: modelData.label
                available: modelData.available
                value: modelData.value
                caption: root.compact ? modelData.shortCaption : modelData.caption
                width: modelData.label === "Clock" ? Math.min(implicitWidth, parent.width) : implicitWidth
            }
        }
    }
}
