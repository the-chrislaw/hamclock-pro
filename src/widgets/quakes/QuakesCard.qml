import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Quakes " + (widget && widget.hasData ? widget.count : "") + " / 24 h" : "Strongest in 24 h"
            available: widget && widget.hasData && widget.count > 0
            value: available ? "M " + widget.strongestMagnitude.toFixed(1) : ""
            caption: available ? widget.strongestText + " · " + widget.strongestPlace : (widget && widget.hasData ? "No M2.5+ quakes" : "")
            valueColor: available && widget.strongestMagnitude >= 7 ? Theme.bad
                      : available && widget.strongestMagnitude >= 6 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearestText !== ""
            text: widget ? "Nearest " + widget.nearestText : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.recent
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.mag; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 2.2 }
                Text { text: modelData.place; color: Theme.ink; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
                Text { text: modelData.age; color: Theme.secondary; font.pixelSize: Theme.fontBody }
            }
        }
    }
}
