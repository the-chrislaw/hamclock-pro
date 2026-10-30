import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout (wide pane, tall pane, or a narrow one with room below, e.g. 1440p rows): summary lines, then the reports list fills the leftover height.
    readonly property bool full: !compact || tall || roomy
    ColumnLayout {
        width: parent.width
        // A few reports: no long list to stretch, so the card grows and centres the block.
        height: root.full && widget && widget.count > 4 ? parent.height : implicitHeight
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
            visible: root.full && widget && widget.hasData && widget.modes !== ""
            text: widget ? widget.modes : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && widget.farthest !== ""
            text: widget ? widget.farthest : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            // Wraps (two lines in a small slot) rather than cutting the distance or band short.
            wrapMode: Text.WordWrap
            maximumLineCount: root.tall ? 100 : 2
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.heard !== ""
            text: widget ? widget.heard : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        // As many reports as fit, whole rows only.
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
                model: root.full && widget ? widget.recent : []
                delegate: RowLayout {
                    width: ListView.view.width
                    height: list.rowHeight
                    spacing: Theme.gap
                    Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                           Layout.preferredWidth: Theme.fontBody * 5.5 }
                    Text { text: modelData.grid; color: Theme.secondary; font.pixelSize: Theme.fontBody
                           Layout.fillWidth: true; elide: Text.ElideRight; objectName: "elideOk" }
                    Text { text: modelData.band + " " + modelData.mode; color: Theme.ink; font.pixelSize: Theme.fontBody }
                    Text { text: modelData.snr; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                    Text { visible: !root.compact; text: modelData.age; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                }
            }
        }
    }
}
