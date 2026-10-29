import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout in a roomy or tall pane: the later activations fill the leftover height
    // (as many rows as fit, never a clipped one); a small slot shows the next one only.
    readonly property bool full: !compact || tall
    FontMetrics { id: rowFont; font.pixelSize: Theme.fontBody }
    readonly property real rowHeight: Math.ceil(rowFont.height)
    readonly property real rowSpacing: Theme.gap * 0.5
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.full ? "Next satellite activation" : "Next activation"
            available: widget && widget.hasNext
            value: available ? widget.nextWhen : ""
            caption: available ? widget.nextTitle : (widget && widget.hasData ? "No upcoming activations" : "")
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nextDetail !== ""
            text: widget ? widget.nextDetail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: root.full ? 100 : 2; Layout.fillWidth: true
        }
        Item {
            id: list
            visible: root.full
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Regular panes list three; a tall pane as many as the feed has and the height allows.
            readonly property var rows: root.full && widget ? widget.later.slice(0, root.tall ? undefined : 3) : []
            readonly property int fits: Math.max(0, Math.floor((height + root.rowSpacing) / (root.rowHeight + root.rowSpacing)))
            implicitHeight: Math.min(rows.length, 3) * (root.rowHeight + root.rowSpacing)
            Column {
                width: parent.width
                spacing: root.rowSpacing
                Repeater {
                    model: list.rows.slice(0, list.fits)
                    delegate: RowLayout {
                        required property var modelData
                        width: parent.width
                        height: root.rowHeight
                        spacing: Theme.gap
                        Text { text: modelData.when; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                               Layout.preferredWidth: Theme.fontBody * 6 }
                        // Activation titles ("call on satellite") can run long; the row ends them with "…".
                        Text { objectName: "elideOk"; text: modelData.title; color: Theme.ink; font.pixelSize: Theme.fontBody
                               elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                }
            }
        }
    }
}
