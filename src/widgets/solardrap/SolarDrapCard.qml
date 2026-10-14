import QtQuick
import HamClock

Card {
    id: root
    readonly property bool ok: root.widget && root.widget.hasData
    readonly property real mhz: ok ? root.widget.peakMhz : 0
    function tone(f) { return f >= 15 ? Theme.bad : f >= 5 ? Theme.warn : Theme.good }
    // Plain Column/Item (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Text {
            width: parent.width
            text: root.ok ? "Max " + root.mhz.toFixed(1) + " MHz" : "—"
            color: root.ok ? root.tone(root.mhz) : Theme.secondary
            font.pixelSize: Theme.fontBody
            font.bold: true
            elide: Text.ElideRight
        }
        Text {
            width: parent.width
            text: root.ok ? (root.compact ? "" : root.widget.message + " · ") + "D-RAP · " + root.widget.observed : "D-RAP unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
