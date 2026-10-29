import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout in a roomy or tall pane: the next-nearest sondes fill the leftover height
    // (as many rows as fit, never a clipped one); a small slot shows the nearest one only.
    readonly property bool full: !compact || tall
    FontMetrics { id: rowFont; font.pixelSize: Theme.fontBody }
    readonly property real rowHeight: Math.ceil(rowFont.height)
    readonly property real rowSpacing: Theme.gap * 0.5
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.full ? "Nearest radiosonde" : "Nearest sonde"
            available: widget && widget.nearest !== ""
            value: available ? widget.nearest : ""
            Layout.fillWidth: true
        }
        Text {
            // Metric hides its caption while unavailable, so say why here.
            visible: text.length > 0
            text: widget && widget.nearest === "" && widget.hasData
                ? (widget.hasStation ? "None heard in the last 30 min" : "Set your station in Settings") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: 2; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearestDetail !== ""
            // A small slot leaves out the landing distance (the map shows it) so the rest fits.
            text: !widget ? "" : root.full ? widget.nearestDetail
                : widget.nearestDetail.split(" · ").filter(p => !p.startsWith("lands ")).join(" · ")
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: root.full ? 100 : 4; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && root.full
            text: widget ? widget.count + " radiosondes heard worldwide" : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Item {
            id: list
            visible: root.full
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Regular panes list three; a tall pane as many as were heard and the height allows.
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
                        Text { text: modelData.dist; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                               Layout.preferredWidth: Theme.fontBody * 6 }
                        Text { text: modelData.text; color: Theme.ink; font.pixelSize: Theme.fontBody
                               elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                }
            }
        }
    }
}
