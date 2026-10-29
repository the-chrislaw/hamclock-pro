import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact desktop pane with height to spare (two-row grids at
    // 1280x800 and 2560x1440). The tuned 7-inch dense layout keeps its compact summary.
    readonly property bool stacked: root.tall || (root.compact && !Theme.dense && root.height >= 240 * Theme.scale
                                                     && root.width >= 240 * Theme.scale)
    readonly property var list: root.widget ? root.widget.ranked : []
    // Compact slot that is not tall: scale words only. Otherwise the full effect text, wrapping.
    readonly property bool brief: root.compact && !root.stacked
    ColumnLayout {
        width: parent.width
        // Tall pane: the rows spread over the height.
        height: root.stacked ? parent.height : implicitHeight
        spacing: Theme.gap * 0.6
        Text {
            Layout.fillWidth: true
            visible: root.widget && root.widget.hasData
            text: root.widget ? (root.brief ? root.widget.headline.split(":")[0] : root.widget.headline) : ""
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.bold: true
            wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
            elide: Text.ElideRight
        }
        Repeater {
            model: root.list
            Item {
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(Math.round(Theme.fontBody * 1.2), effect.implicitHeight)
                Layout.fillHeight: root.stacked
                Layout.maximumHeight: root.stacked ? Math.max(effect.implicitHeight, Theme.fontBody * 3) : Layout.preferredHeight
                readonly property color tone: modelData.level >= 3 ? Theme.bad : modelData.level >= 1 ? Theme.warn : Theme.good
                Text {
                    id: tag
                    anchors.verticalCenter: root.stacked ? parent.verticalCenter : undefined
                    text: modelData.letter + modelData.level
                    color: parent.tone
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    width: Theme.fontBody * 2.4
                }
                Text {
                    id: effect
                    anchors { left: tag.right; right: parent.right; verticalCenter: root.stacked ? parent.verticalCenter : undefined }
                    text: root.brief ? (modelData.word.length ? modelData.word : "none") : modelData.effect
                    color: Theme.secondary
                    font.pixelSize: Theme.fontBody
                    wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
                    maximumLineCount: 3
                    elide: Text.ElideRight
                }
            }
        }
    }
}
