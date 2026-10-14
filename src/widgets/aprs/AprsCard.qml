import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "APRS" : "Stations within 100 km"
            available: widget && widget.state === "live"
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? "Nearest " + widget.nearest[0].call + " · " + widget.nearest[0].km + " km"
                     : (available ? "Listening" : "")
            Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.nearest
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 7 }
                Text { text: modelData.km + " km"; color: Theme.secondary; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
