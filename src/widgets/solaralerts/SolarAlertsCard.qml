import QtQuick
import HamClock

Card {
    id: root
    readonly property var list: root.widget ? root.widget.alerts : []
    readonly property int maxRows: root.compact ? 2 : 5
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
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                    Text {
                        visible: !root.compact
                        width: parent.width
                        text: modelData.kind + " " + modelData.issued + (modelData.until.length ? " · " + modelData.until : "")
                        color: Theme.secondary
                        font.pixelSize: Theme.fontCaption
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
