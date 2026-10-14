import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Flow {
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Fix"
            available: root.widget && root.widget.fixText !== ""
            value: available ? root.widget.fixText : ""
            caption: available && !root.compact && root.widget.satellites !== "" ? root.widget.satellites + " satellites" : ""
        }
        Metric {
            label: "Grid"
            visible: root.widget && root.widget.grid !== ""
            value: visible ? root.widget.grid : ""
            caption: visible && !root.compact ? root.widget.position : ""
        }
    }
}
