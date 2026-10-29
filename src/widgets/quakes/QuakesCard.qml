import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout in a roomy or tall pane: the recent quakes fill the leftover height (as many
    // rows as fit, never a clipped one); a tall pane puts each quake's age under its place.
    // A small slot shows the strongest and the nearest only.
    readonly property bool full: !compact || tall
    FontMetrics { id: rowFont; font.pixelSize: Theme.fontBody }
    FontMetrics { id: ageFont; font.pixelSize: Theme.fontCaption }
    readonly property real rowHeight: Math.ceil(rowFont.height) + (tall ? Math.ceil(ageFont.height) : 0)
    readonly property real rowSpacing: Theme.gap * (tall ? 0.7 : 0.5)
    function magColor(m) { return m >= 7 ? Theme.bad : m >= 6 ? Theme.warn : Theme.ink }
    ColumnLayout {
        width: parent.width
        height: root.full ? parent.height : implicitHeight
        spacing: Theme.gap
        Metric {
            label: root.tall && widget && widget.hasData ? "Strongest of " + widget.count + " in 24 h"
                 : root.full ? "Strongest in 24 h" : "Quakes " + (widget && widget.hasData ? widget.count : "") + " / 24 h"
            available: widget && widget.hasData && widget.count > 0
            value: available ? "M " + widget.strongestMagnitude.toFixed(1) : ""
            caption: available ? widget.strongestText : (widget && widget.hasData ? "No M2.5+ quakes" : "")
            valueColor: available ? root.magColor(widget.strongestMagnitude) : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && widget.count > 0 && widget.strongestPlace !== ""
            text: widget ? widget.strongestPlace : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: 2; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearestText !== ""
            text: widget ? "Nearest " + widget.nearestText : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: root.full ? 3 : 2; Layout.fillWidth: true
        }
        Item {
            id: list
            visible: root.full
            Layout.fillWidth: true
            Layout.fillHeight: true
            // Regular panes list six; a tall pane as many as the feed has and the height allows.
            readonly property var rows: root.full && widget ? widget.recent.slice(0, root.tall ? undefined : 6) : []
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
                        columns: 3
                        rowSpacing: 0
                        columnSpacing: Theme.gap
                        Text { text: modelData.mag; color: root.magColor(Number(modelData.mag)); font.bold: true; font.pixelSize: Theme.fontBody
                               Layout.row: 0; Layout.column: 0; Layout.preferredWidth: Theme.fontBody * 2.2 }
                        // Places run long; the map shows every quake, so the list may shorten them.
                        Text { objectName: "elideOk"; text: modelData.place; color: Theme.ink; font.pixelSize: Theme.fontBody
                               Layout.row: 0; Layout.column: 1; Layout.columnSpan: root.tall ? 2 : 1
                               elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { text: modelData.age; color: Theme.secondary
                               font.pixelSize: root.tall ? Theme.fontCaption : Theme.fontBody
                               Layout.row: root.tall ? 1 : 0; Layout.column: root.tall ? 1 : 2 }
                    }
                }
            }
        }
    }
}
