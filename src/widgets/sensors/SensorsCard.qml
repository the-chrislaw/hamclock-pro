import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Flow {
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "CPU temp"
            available: root.widget && root.widget.hasCpuTemp
            value: available ? root.widget.cpuTemp : ""
            caption: available && !root.compact ? root.widget.cpuNote + (root.widget.cpuPeak !== "" ? " · " + root.widget.cpuPeak : "") : ""
        }
        Metric {
            label: "Air temp"
            visible: root.widget && root.widget.envTemp !== ""
            value: visible ? root.widget.envTemp : ""
            caption: visible && !root.compact ? root.widget.envTempRange : ""
        }
        Metric {
            label: "Pressure"
            visible: root.widget && root.widget.envPressure !== ""
            value: visible ? root.widget.envPressure : ""
            caption: visible && root.widget.envTrend !== "" ? root.widget.envTrend : ""
        }
        Metric {
            label: "Humidity"
            visible: root.widget && root.widget.envHumidity !== ""
            value: visible ? root.widget.envHumidity : ""
            caption: visible && !root.compact && root.widget.envDewPoint !== "" ? "Dew point " + root.widget.envDewPoint : ""
        }
        Metric {
            label: "Wi-Fi"
            visible: root.widget && root.widget.hasWifi
            value: visible ? root.widget.wifi : ""
            caption: visible && !root.compact ? root.widget.wifiLow : ""
        }
        Metric {
            label: "Clock"
            visible: root.widget && root.widget.hasClock
            // "Synced, <1 ms, stratum 2, server": status is the value, the rest a caption that elides.
            value: visible ? root.widget.clock.split(", ")[0] : ""
            caption: visible ? root.widget.clock.split(", ").slice(1).join(", ") : ""
            width: Math.min(implicitWidth, (root.width / root.fit))
        }
        Metric {
            label: "Disk"
            visible: root.widget && root.widget.hasDisk
            value: visible ? root.widget.disk : ""
            caption: visible && !root.compact ? root.widget.diskFree : ""
        }
    }
}
