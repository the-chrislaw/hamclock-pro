import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    Flow {
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Aircraft"
            available: root.widget && root.widget.tracked !== ""
            value: available ? root.widget.tracked : ""
        }
        Metric {
            label: "With position"
            visible: root.widget && root.widget.withPosition !== ""
            value: visible ? root.widget.withPosition : ""
        }
    }
}
