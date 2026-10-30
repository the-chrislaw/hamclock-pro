import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Planets above the horizon at DE, highest first; rows that do not fit are dropped, not clipped.
    // A pane with height to spare grows the rows (Card scales short content to fill) instead of spreading them.
    // One row's real pitch (line height plus spacing): a larger estimate drops a row that fits.
    readonly property real rowH: Math.ceil(Theme.fontCaption * 1.3 * 1.2) + Theme.gap
    readonly property int rowsFit: Math.max(1, Math.floor((grid.parent ? grid.parent.height : 0) / rowH))
    readonly property int shown: root.widget ? Math.min(root.widget.rows.length, rowsFit) : 0
    Text {
        visible: root.widget && root.widget.hasStation && root.widget.rows.length === 0
        text: "None above the horizon"
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption * 1.3
    }
    GridLayout {
        id: grid
        width: parent.width
        columns: 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Repeater {
            model: root.shown * 2
            Text {
                required property int index
                readonly property var row: root.widget.rows[Math.floor(index / 2)] || ({})
                readonly property bool first: index % 2 === 0
                text: (first ? row.name : row.position) || ""
                color: first ? Theme.ink : Theme.secondary
                font.pixelSize: Theme.fontCaption * (first ? 1.3 : 1)
                font.weight: first ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: !first
            }
        }
    }
}
