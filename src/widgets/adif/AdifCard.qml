import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Log" : "QSOs in log"
            available: widget && widget.hasData
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? widget.recent[0].call + " · " + widget.recent[0].band + " " + widget.recent[0].mode
                     : (available ? "Log is empty" : "")
            Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.recent
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 6 }
                Text { text: modelData.band + " " + modelData.mode; color: Theme.secondary; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
                Text { text: modelData.age; color: Theme.secondary; font.pixelSize: Theme.fontBody }
            }
        }
    }
}
