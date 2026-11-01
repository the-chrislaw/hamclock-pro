import QtQuick
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property var list: root.widget ? root.widget.alerts : []
    // A tall pane shows the full layout (kind, issue time) for as many alerts as the full card.
    readonly property bool brief: root.compact && !root.stacked
    readonly property int maxRows: brief ? 2 : 5
    // Plain Column/Item rows (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Text {
            visible: root.widget && root.widget.hasData && root.list.length === 0
            width: parent.width
            text: "Nothing in force"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
        }
        Repeater {
            model: root.list.slice(0, root.maxRows)
            Item {
                required property var modelData
                width: parent.width
                height: body.height
                readonly property color tone: modelData.level >= 2 ? Theme.bad : modelData.level === 1 ? Theme.warn : Theme.ink
                Text {
                    id: tag
                    text: modelData.scale.length ? modelData.scale : (modelData.kind === "Extended warning" ? "W" : modelData.kind.charAt(0))
                    color: parent.tone
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    width: Theme.fontBody * 2
                }
                Column {
                    id: body
                    anchors { left: tag.right; right: parent.right }
                    Text {
                        width: parent.width
                        text: modelData.headline
                        color: Theme.ink
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                        maximumLineCount: root.stacked ? 3 : 2
                        elide: Text.ElideRight
                    }
                    Text {
                        visible: !root.brief
                        width: parent.width
                        text: modelData.kind + " " + modelData.issued + (modelData.until.length ? " · " + modelData.until : "")
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
                        wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }
            }
        }
        Text {
            visible: root.list.length > root.maxRows
            text: "+" + (root.list.length - root.maxRows) + " more"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
        }
    }
}
