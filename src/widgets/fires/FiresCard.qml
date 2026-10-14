import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Fires" : "Active fires (US)"
            available: widget && widget.hasData
            value: !available ? "" : widget.count === 0 ? "None" : widget.summary
            caption: available && widget.count > 0 ? widget.highCount + " high confidence · last 24 h" : (available ? "No detections in the last 24 h" : "")
            valueColor: available && widget.highCount > 0 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.detail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }
}
