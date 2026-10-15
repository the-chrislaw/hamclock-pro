import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Heard by" : "Stations hearing me"
            available: widget && widget.hasData
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0 && widget.bestSnr !== ""
                     ? "best " + widget.bestSnr
                     : (available ? "Not heard in the last 15 min" : "")
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && widget.bands !== ""
            text: widget ? widget.bands : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && widget.farthest !== ""
            text: widget ? widget.farthest : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.recent
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 5.5 }
                Text { text: modelData.grid; color: Theme.secondary; font.pixelSize: Theme.fontBody
                       Layout.fillWidth: true; elide: Text.ElideRight }
                Text { text: modelData.band + " " + modelData.mode; color: Theme.ink; font.pixelSize: Theme.fontBody }
                Text { text: modelData.snr; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                Text { text: modelData.age; color: Theme.secondary; font.pixelSize: Theme.fontBody }
            }
        }
    }
}
