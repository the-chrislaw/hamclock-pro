import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || (compact && height >= 260 * Theme.scale)
    // Stacked: one row per band (MHz, then call over place), rows spread over the height.
    ColumnLayout {
        visible: root.stacked
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.6
        Repeater {
            model: root.stacked && root.widget ? root.widget.rows : []
            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: Theme.fontBody * 4
                spacing: Theme.gap * 1.5
                Text {
                    text: modelData.band
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    Layout.alignment: Qt.AlignVCenter
                }
                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Text {
                        text: modelData.call
                        color: Theme.ink
                        font.pixelSize: Theme.fontCaption * 1.3
                        font.weight: Font.DemiBold
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.location
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
    // One row per band: MHz, call, then the place (hidden in a compact slot).
    GridLayout {
        visible: !root.stacked
        width: parent.width
        columns: root.compact ? 2 : 3
        columnSpacing: Theme.gap * 1.5
        rowSpacing: root.compact ? Theme.gap * 0.4 : Theme.gap
        Repeater {
            model: root.widget && !root.stacked ? root.widget.rows.length * (root.compact ? 2 : 3) : 0
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
                // The place takes what is left after the call, never squeezing the call.
                Layout.preferredWidth: col === 2 ? Theme.fontCaption : -1
                objectName: col === 2 ? "elideOk" : ""
            }
        }
    }
}
