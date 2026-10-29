import QtQuick
import QtQuick.Layouts
import HamClock

// Band x continent spot counts from the DX cluster; cell shade follows the count.
Card {
    id: root
    // Stacked full layout: a tall pane, or a compact desktop pane with height to spare (two-row grids at
    // 1280x800 and 2560x1440). The tuned 7-inch dense layout keeps its compact summary.
    readonly property bool stacked: root.tall || (root.compact && !Theme.dense && root.height >= 240 * Theme.scale
                                                     && root.width >= 240 * Theme.scale)
    readonly property var rows: widget ? widget.rows : []
    readonly property bool ok: rows.length > 0
    // Compact slot that is not tall: the busiest band only. Otherwise the grid fills the height.
    readonly property bool brief: root.compact && !root.stacked
    ColumnLayout {
        width: parent.width
        height: root.brief ? implicitHeight : parent.height
        spacing: 2 * Theme.scale
        Text {
            visible: !root.ok
            Layout.fillWidth: true
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
        }
        Text {
            visible: root.ok
            Layout.fillWidth: true
            text: root.widget ? "Busiest " + root.widget.busiestText : ""
            color: Theme.ink
            font.pixelSize: root.brief ? Theme.fontBody : Theme.fontValue
            font.weight: Font.DemiBold
            wrapMode: root.brief ? Text.NoWrap : Text.WordWrap
            elide: Text.ElideRight
        }
        Item {
            id: grid
            visible: root.ok && !root.brief
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.gap * 0.4
            Layout.preferredHeight: (root.rows.length + 1) * Theme.fontCaption * 1.4
            readonly property real gap: 2 * Theme.scale
            readonly property real labelWidth: 40 * Theme.scale
            readonly property var continents: root.widget ? root.widget.continents : []
            readonly property real cellWidth: Math.max(8, (width - labelWidth - continents.length * gap)
                                                          / Math.max(1, continents.length))
            // Rows share the height (never more, so nothing runs under the footer), capped at ~2 lines.
            // A short pane (phone strip) keeps the busiest bands that fit at a readable row height, in band order.
            readonly property int maxRows: Math.max(1, Math.floor(height / (Theme.fontCaption * 1.4)) - 1)
            readonly property var shown: {
                if (root.rows.length <= maxRows) return root.rows
                const keep = root.rows.slice().sort((a, b) => (b.total || 0) - (a.total || 0)).slice(0, maxRows)
                return root.rows.filter(r => keep.indexOf(r) >= 0)
            }
            readonly property real rowHeight: Math.min(Theme.fontCaption * 2.6, height / (shown.length + 1))
            Column {
                width: parent.width
                Row {
                    height: grid.rowHeight
                    spacing: grid.gap
                    Item { width: grid.labelWidth; height: 1 }
                    Repeater {
                        model: grid.continents
                        delegate: Text {
                            width: grid.cellWidth
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            horizontalAlignment: Text.AlignHCenter
                            color: Theme.secondary
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                }
                Repeater {
                    model: root.ok && !root.brief ? grid.shown : []
                    delegate: Row {
                        id: bandRow
                        readonly property var rowData: modelData
                        height: grid.rowHeight
                        spacing: grid.gap
                        Text { width: grid.labelWidth; text: modelData.band; color: Theme.ink
                               anchors.verticalCenter: parent.verticalCenter
                               font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                        Repeater {
                            model: grid.continents
                            delegate: Rectangle {
                                readonly property int n: bandRow.rowData[modelData] || 0
                                width: grid.cellWidth
                                height: grid.rowHeight - grid.gap
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 2 * Theme.scale
                                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b,
                                               n > 0 ? 0.2 + 0.8 * n / Math.max(1, root.widget.maxCell) : 0.07)
                                Text { anchors.centerIn: parent; visible: parent.n > 0; text: parent.n
                                       color: Theme.ink; font.pixelSize: Theme.fontCaption }
                            }
                        }
                    }
                }
            }
        }
        Text {
            visible: root.ok && !root.brief
            Layout.fillWidth: true
            text: (grid.shown.length < root.rows.length ? "Busiest " + grid.shown.length + " of " + root.rows.length + " bands · " : "")
                  + "Recent DX cluster spots by band and continent of the spotted station"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
        }
    }
}
