import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout (wide pane, tall pane, or a narrow one with room below, e.g. 1440p rows): the alert list fills the leftover height.
    readonly property bool full: !compact || tall || roomy
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
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
        // As many alerts as fit, whole rows only.
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
                    Text { text: modelData.time; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                    Text { text: modelData.dx; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                           Layout.preferredWidth: Theme.fontBody * 6 }
                    Text { text: modelData.frequency + " " + modelData.mode + (modelData.ota ? " · " + modelData.ota : "")
                           color: Theme.secondary; font.pixelSize: Theme.fontBody
                           elide: Text.ElideRight; Layout.fillWidth: true; objectName: "elideOk" }
                }
            }
        }
    }
}
