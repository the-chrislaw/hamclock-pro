import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // One row per band: MHz, call, then the place (hidden in a compact slot).
    GridLayout {
        width: parent.width
        columns: root.compact ? 2 : 3
        columnSpacing: Theme.gap * 1.5
        rowSpacing: root.compact ? Theme.gap * 0.4 : Theme.gap
        Repeater {
            model: root.widget ? root.widget.rows.length * (root.compact ? 2 : 3) : 0
            Text {
                required property int index
                readonly property int cols: root.compact ? 2 : 3
                readonly property var row: root.widget.rows[Math.floor(index / cols)]
                readonly property int col: index % cols
                text: col === 0 ? row.band : col === 1 ? row.call : row.location
                color: col === 1 ? Theme.ink : Theme.secondary
                font.pixelSize: Theme.fontCaption * (col === 1 ? 1.3 : 1)
                font.weight: col === 1 ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
                Layout.fillWidth: col === cols - 1
            }
        }
    }
}
