import QtQuick
import HamClock

Card {
    id: root
    readonly property bool ok: root.widget && root.widget.hasData
    function tone(p) { return p >= 40 ? Theme.bad : p >= 15 ? Theme.warn : Theme.good }
    // Plain Column/Item (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Item {
            width: parent.width
            height: Math.round(Theme.fontBody * 1.3)
            Text {
                id: north
                text: root.ok ? "N " + root.widget.northPeak + "%" : "—"
                color: root.ok ? root.tone(root.widget.northPeak) : Theme.secondary
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
            Text {
                anchors { left: north.right; leftMargin: Theme.gap * 2 }
                text: root.ok ? "S " + root.widget.southPeak + "%" : ""
                color: root.ok ? root.tone(root.widget.southPeak) : Theme.secondary
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
        }
        Text {
            width: parent.width
            text: root.ok ? (root.compact ? "Peak aurora · " : "Peak aurora probability · map style Aurora · ") + root.widget.observed : "Aurora nowcast unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
