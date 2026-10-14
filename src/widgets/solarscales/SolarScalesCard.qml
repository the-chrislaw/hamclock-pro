import QtQuick
import HamClock

Card {
    id: root
    readonly property var list: root.widget ? root.widget.scales : []
    // Plain Column/Item rows (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Repeater {
            model: root.list
            Item {
                required property var modelData
                width: parent.width
                height: Math.round(Theme.fontBody * 1.2) + (root.compact ? 0 : Theme.fontCaption * 1.3)
                readonly property color tone: modelData.level >= 3 ? Theme.bad : modelData.level >= 1 ? Theme.warn : Theme.good
                Text {
                    id: tag
                    text: modelData.letter + modelData.level
                    color: parent.tone
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    width: Theme.fontBody * 2.4
                }
                Text {
                    anchors { left: tag.right; right: parent.right }
                    text: root.compact ? (modelData.word.length ? modelData.word : "none")
                                       : modelData.name + (modelData.word.length ? " · " + modelData.word : "")
                    color: Theme.ink
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }
                Text {
                    visible: !root.compact
                    anchors { left: tag.right; right: parent.right; bottom: parent.bottom }
                    text: "24 h max " + modelData.letter + modelData.max24 +
                          (modelData.outlook.length ? " · tomorrow " + modelData.outlook : "")
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    elide: Text.ElideRight
                }
            }
        }
        Text {
            visible: root.widget && root.widget.hasData && !root.compact
            text: "Observed " + root.widget.observed
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
        }
    }
}
