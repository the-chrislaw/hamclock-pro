import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout (wide pane, tall pane, or a narrow one with room below, e.g. 1440p rows): the spot list fills the leftover height.
    readonly property bool full: !compact || tall || roomy
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.compact ? "On the air" : "Activators spotted"
            available: widget && widget.hasData
            value: available ? widget.count.toString() : ""
            caption: available && widget.count > 0
                     ? widget.recent[0].call + " · " + widget.recent[0].ref + " · " + widget.recent[0].freq
                     : (available ? "No current spots" : "")
            Layout.fillWidth: true
        }
        // As many spots as fit, whole rows only.
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
                    // A narrow pane drops the programme column: the reference already names it.
                    Text { visible: !root.compact; text: modelData.program; color: Theme.secondary; font.pixelSize: Theme.fontBody
                           Layout.preferredWidth: Math.max(implicitWidth, Theme.fontBody * 2.6) }
                    Text { text: modelData.call; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                           Layout.preferredWidth: Theme.fontBody * 5.8 }  // room for a portable call (VK3OAK/P)
                    Text { text: modelData.ref; color: Theme.ink; font.pixelSize: Theme.fontBody
                           elide: Text.ElideRight; Layout.fillWidth: true; objectName: "elideOk" }
                    Text { text: modelData.freq + (root.compact ? "" : " " + modelData.mode); color: Theme.secondary; font.pixelSize: Theme.fontBody }
                    Text { visible: !root.compact; text: modelData.age; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                }
            }
        }
    }
}
