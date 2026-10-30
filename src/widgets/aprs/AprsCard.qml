import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout (wide pane, tall pane, or a narrow one with room below, e.g. 1440p rows): the station list fills the leftover height.
    readonly property bool full: !compact || tall || height >= 260 * Theme.scale
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.compact ? "APRS" : (widget ? widget.rangeText : "")
            available: widget && widget.state === "live"
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? "Nearest " + widget.nearest[0].call + " · " + widget.nearest[0].distance
                     : (available ? "Listening" : "")
            Layout.fillWidth: true
        }
        // Nearest first, as many as fit, whole rows only.
        Item {
            visible: root.full && list.count > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: list.contentHeight
            ListView {
                id: list
                readonly property real rowHeight: Theme.fontBody * 1.5
                width: parent.width
                height: Math.min(contentHeight, Math.floor(parent.height / rowHeight) * rowHeight)
                clip: true
                interactive: false
                model: root.full && widget ? widget.nearest : []
                delegate: RowLayout {
                    width: ListView.view.width
                    height: list.rowHeight
                    spacing: Theme.gap
                    Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                           elide: Text.ElideRight; Layout.fillWidth: true }
                    Text { text: modelData.distance; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                }
            }
        }
    }
}
