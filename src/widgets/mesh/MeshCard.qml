import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Mesh" : "Meshtastic nodes with a position"
            available: widget && widget.state === "live"
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? "Nearest " + widget.nearest[0].name + " · " + widget.nearest[0].km + " km"
                     : (available ? "Waiting for the node list" : "")
            Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.nearest
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.name; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 5 }
                Text { text: modelData.km + " km"; color: Theme.secondary; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
