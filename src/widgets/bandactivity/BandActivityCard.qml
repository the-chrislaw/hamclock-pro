import QtQuick
import HamClock

// Band x continent spot counts from the DX cluster; cell shade follows the count.
Card {
    id: root
    readonly property var rows: widget ? widget.rows : []
    readonly property bool ok: rows.length > 0
    Column {
        width: parent.width
        spacing: 2 * Theme.scale
        Text {
            visible: !root.ok
            width: parent.width
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
        }
        Text {
            visible: root.ok
            width: parent.width
            text: root.widget ? "Busiest " + root.widget.busiestText : ""
            color: Theme.ink
            font.pixelSize: root.compact ? Theme.fontBody : Theme.fontValue
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        Row {
            visible: root.ok && !root.compact
            spacing: 2 * Theme.scale
            Item { width: 40 * Theme.scale; height: 1 }
            Repeater {
                model: root.widget ? root.widget.continents : []
                delegate: Text {
                    width: Math.max(8, ((root.width / root.fit) - 40 * Theme.scale - 6 * 2 * Theme.scale) / 6)
                    text: modelData
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                }
            }
        }
        Repeater {
            model: root.ok && !root.compact ? root.rows : []
            delegate: Row {
                id: bandRow
                readonly property var rowData: modelData
                spacing: 2 * Theme.scale
                Text { width: 40 * Theme.scale; text: modelData.band; color: Theme.ink
                       font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                Repeater {
                    model: root.widget.continents
                    delegate: Rectangle {
                        readonly property int n: bandRow.rowData[modelData] || 0
                        width: Math.max(8, ((root.width / root.fit) - 40 * Theme.scale - 6 * 2 * Theme.scale) / 6)
                        height: Theme.fontCaption * 1.3
                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b,
                                       n > 0 ? 0.2 + 0.8 * n / Math.max(1, root.widget.maxCell) : 0.07)
                        Text { anchors.centerIn: parent; visible: parent.n > 0; text: parent.n
                               color: Theme.ink; font.pixelSize: Theme.fontCaption }
                    }
                }
            }
        }
        Text {
            visible: root.ok && !root.compact
            text: "Recent DX cluster spots by band and continent of the spotted station"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            width: parent.width
            elide: Text.ElideRight
        }
    }
}
