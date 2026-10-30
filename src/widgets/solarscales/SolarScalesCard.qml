import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property var list: root.widget ? root.widget.scales : []
    // Compact slot that is not tall: scale words only. A tall pane shows the full rows, spread over the height.
    readonly property bool brief: root.compact && !root.stacked
    ColumnLayout {
        width: parent.width
        height: root.stacked ? parent.height : implicitHeight
        spacing: Theme.gap * 0.6
        Repeater {
            model: root.list
            Item {
                required property var modelData
                Layout.fillWidth: true
                // Fixed line budget (a tall pane lets the detail wrap to two lines) so the row never loops on its text.
                readonly property real blockHeight: Math.round(Theme.fontBody * 1.2)
                    + (root.brief ? 0 : 2 * Theme.scale + Math.ceil(Theme.fontCaption * 1.3) * (root.stacked ? 2 : 1))
                Layout.preferredHeight: blockHeight
                Layout.fillHeight: root.stacked
                Layout.maximumHeight: root.stacked ? Math.max(blockHeight, Theme.fontBody * 4) : blockHeight
                readonly property color tone: modelData.level >= 3 ? Theme.bad : modelData.level >= 1 ? Theme.warn : Theme.good
                Item {
                    // Content block, centred in a tall pane's spread row.
                    width: parent.width
                    height: parent.blockHeight
                    // Bound y (an anchor set to undefined keeps a stale centred y after a resize).
                    y: root.stacked ? (parent.height - height) / 2 : 0
                    Text {
                        id: tag
                        text: modelData.letter + modelData.level
                        color: parent.parent.tone
                        font.pixelSize: Theme.fontBody
                        font.bold: true
                        width: Theme.fontBody * 2.4
                    }
                    Text {
                        id: name
                        anchors { left: tag.right; right: parent.right }
                        height: Math.round(Theme.fontBody * 1.2)
                        text: root.brief ? (modelData.word.length ? modelData.word : "none")
                                         : modelData.name + (modelData.word.length ? " · " + modelData.word : "")
                        color: Theme.ink
                        font.pixelSize: Theme.fontBody
                        elide: Text.ElideRight
                    }
                    Text {
                        id: detail
                        visible: !root.brief
                        anchors { left: tag.right; right: parent.right; top: name.bottom; topMargin: 2 * Theme.scale }
                        text: "24 h max " + modelData.letter + modelData.max24 +
                              (modelData.outlook.length ? " · tomorrow " + modelData.outlook : "")
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
            visible: root.widget && root.widget.hasData && !root.brief
            text: "Observed " + root.widget.observed
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
        }
    }
}
