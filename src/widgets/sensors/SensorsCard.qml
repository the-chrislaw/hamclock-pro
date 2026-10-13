import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Metric {
        width: parent.width
        label: "CPU temperature"
        available: root.widget && root.widget.hasCpuTemp
        value: available ? root.widget.cpuTemp : ""
        caption: available && !root.compact ? root.widget.cpuNote : ""
    }
}
