import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Flow {
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Azimuth"
            available: root.widget && root.widget.azimuth !== ""
            value: available ? root.widget.azimuth : ""
        }
        Metric {
            label: "Elevation"
            visible: root.widget && root.widget.elevation !== ""
            value: visible ? root.widget.elevation : ""
        }
    }
}
