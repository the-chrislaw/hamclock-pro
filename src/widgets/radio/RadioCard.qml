import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Flow {
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Frequency"
            available: root.widget && root.widget.frequency !== ""
            value: available ? root.widget.frequency : ""
            caption: available && !root.compact ? root.widget.band : ""
        }
        Metric {
            label: "Mode"
            visible: root.widget && root.widget.mode !== ""
            value: visible ? root.widget.mode : ""
            caption: visible && !root.compact ? root.widget.passband : ""
        }
    }
}
