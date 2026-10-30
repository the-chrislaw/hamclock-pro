import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout in a roomy or tall pane: the later launches fill the leftover height (as many
    // as fit, never a clipped one); a tall pane stacks each as name over time. A small slot
    // shows the next launch only, without the agency so the detail fits.
    readonly property bool full: !compact || tall || roomy
    FontMetrics { id: rowFont; font.pixelSize: Theme.fontBody }
    FontMetrics { id: whenFont; font.pixelSize: Theme.fontCaption }
    readonly property real rowHeight: Math.ceil(rowFont.height) + (tall ? Math.ceil(whenFont.height) : 0)
    readonly property real rowSpacing: Theme.gap * (tall ? 0.7 : 0.5)
    readonly property string detail: {
        if (!widget) return ""
        const parts = widget.nextDetail.split(" · ")
        return full || parts.length < 3 ? widget.nextDetail : parts.slice(1).join(" · ")
    }
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.full ? "Next launch (NET)" : "Next launch"
            available: widget && widget.hasNext
            value: available ? widget.nextCountdown : ""
            caption: available ? widget.nextName : (widget && widget.hasData ? "No upcoming launches" : "")
            Layout.fillWidth: true
        }
        Text {
            visible: root.detail !== ""
            text: root.detail
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: root.full ? 100 : 3; Layout.fillWidth: true
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
                    delegate: GridLayout {
                        required property var modelData
                        width: parent.width
                        height: root.rowHeight
                        columns: root.tall ? 1 : 2
                        rowSpacing: 0
                        columnSpacing: Theme.gap
                        Text { text: modelData.when; color: root.tall ? Theme.secondary : Theme.ink; font.bold: !root.tall
                               font.pixelSize: root.tall ? Theme.fontCaption : Theme.fontBody
                               Layout.row: root.tall ? 1 : 0
                               Layout.column: 0
                               Layout.preferredWidth: root.tall ? -1 : Theme.fontBody * 7 }
                        // Mission names run to any length; the row ends them with "…".
                        Text { objectName: "elideOk"; text: modelData.name + (modelData.ham ? "  ★" : ""); color: Theme.ink; font.pixelSize: Theme.fontBody
                               Layout.row: 0
                               Layout.column: root.tall ? 0 : 1
                               elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                }
            }
        }
    }
}
