import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property bool ok: root.widget && root.widget.hasData
    readonly property int level: ok ? root.widget.dst : 0
    readonly property color tone: level <= -100 ? Theme.bad : level <= -50 ? Theme.warn : level <= -30 ? Theme.warn : Theme.good
    // Every pane: the Dst line takes the leftover height.
    readonly property bool fillPlot: true
    readonly property bool showDetail: root.ok && (!root.compact || root.stacked)
    ColumnLayout {
        width: parent.width
        height: root.fillPlot ? parent.height : implicitHeight
        spacing: Theme.gap * 0.6
        // Tall pane: value with its label and storm word as caption.
        Metric {
            visible: root.stacked
            label: "Dst index"
            available: root.ok
            value: available ? root.level + " nT" : ""
            caption: available ? root.widget.word : ""
            valueColor: root.tone
            Layout.fillWidth: true
        }
        Item {
            visible: !root.stacked
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(Theme.fontBody * 1.3)
            Text {
                id: value
                text: root.ok ? root.level + " nT" : "—"
                color: root.ok ? root.tone : Theme.secondary
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
            Text {
                anchors { left: value.right; leftMargin: Theme.gap; right: parent.right }
                text: root.ok ? root.widget.word : ""
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                elide: Text.ElideRight
            }
        }
        DstLine {
            visible: root.ok
            pixelScale: root.fit
            values: root.ok ? root.widget.history : []
            lineColor: root.tone
            Layout.fillWidth: true
            Layout.preferredHeight: root.compact && !root.stacked ? 28 * Theme.scale : 56 * Theme.scale
            Layout.fillHeight: root.fillPlot
        }
        Text {
            visible: root.showDetail
            text: root.ok && Math.min.apply(null, root.widget.history) <= -30 ? "48 h · lines at 0 and −50 nT (storm)" : "48 h · line at 0 nT"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            visible: root.showDetail
            text: "24 h low " + (root.ok ? root.widget.min24 : 0) + " nT · observed " + (root.ok ? root.widget.observed : "")
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }
}
