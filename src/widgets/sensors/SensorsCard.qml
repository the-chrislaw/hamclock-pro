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
            label: "Wi-Fi"
            visible: root.widget && root.widget.hasWifi
            value: visible ? root.widget.wifi : ""
        }
        Metric {
            label: "Disk"
            visible: root.widget && root.widget.hasDisk
            value: visible ? root.widget.disk : ""
        }
    }
}
