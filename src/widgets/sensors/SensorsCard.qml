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
            caption: available && !root.compact ? root.widget.cpuNote : ""
        }
        Metric {
            label: "Air temp"
            visible: root.widget && root.widget.envTemp !== ""
            value: visible ? root.widget.envTemp : ""
        }
        Metric {
            label: "Pressure"
            visible: root.widget && root.widget.envPressure !== ""
            value: visible ? root.widget.envPressure : ""
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
        }
        Metric {
            label: "Clock"
            visible: root.widget && root.widget.hasClock
            value: visible ? root.widget.clock : ""
        }
        Metric {
            label: "Disk"
            visible: root.widget && root.widget.hasDisk
            value: visible ? root.widget.disk : ""
        }
    }
}
