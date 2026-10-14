import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Alerts" : "HamAlert spots"
            available: widget && widget.state === "live"
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? widget.recent[0].dx + " · " + widget.recent[0].band + " " + widget.recent[0].mode
                     : (available ? "Waiting for alerts" : "")
            Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.recent
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.time; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                Text { text: modelData.dx; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 6 }
                Text { text: modelData.frequency + " " + modelData.mode; color: Theme.secondary; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
