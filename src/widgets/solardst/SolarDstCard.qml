import QtQuick
import HamClock

Card {
    id: root
    readonly property bool ok: root.widget && root.widget.hasData
    readonly property int level: ok ? root.widget.dst : 0
    readonly property color tone: level <= -100 ? Theme.bad : level <= -50 ? Theme.warn : level <= -30 ? Theme.warn : Theme.good
    // Plain Column/Item (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Item {
            width: parent.width
            height: Math.round(Theme.fontBody * 1.3)
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
            width: parent.width
            height: root.compact ? 28 * Theme.scale : 56 * Theme.scale
            values: root.ok ? root.widget.history : []
            lineColor: root.tone
        }
        Text {
            visible: root.ok && !root.compact
            width: parent.width
            text: "24 h low " + (root.ok ? root.widget.min24 : 0) + " nT · observed " + (root.ok ? root.widget.observed : "")
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
