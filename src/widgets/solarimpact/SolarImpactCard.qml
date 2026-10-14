import QtQuick
import HamClock

Card {
    id: root
    readonly property var list: root.widget ? root.widget.ranked : []
    Column {
        width: parent.width
        spacing: Theme.gap * 0.6
        Text {
            width: parent.width
            visible: root.widget && root.widget.hasData
            text: root.widget ? (root.compact ? root.widget.headline.split(":")[0] : root.widget.headline) : ""
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.bold: true
            elide: Text.ElideRight
        }
        Repeater {
            model: root.list
            Item {
                required property var modelData
                width: parent.width
                height: Math.round(Theme.fontBody * 1.2)
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
                    text: root.compact ? (modelData.word.length ? modelData.word : "none") : modelData.effect
                    color: Theme.secondary
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                }
            }
        }
    }
}
