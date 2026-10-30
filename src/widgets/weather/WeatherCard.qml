import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // DE and DX side by side; a tall pane stacks them, each with every value. Details wrap
    // (never overprint or cut short); air details appear only when the stage has the height.
    readonly property bool full: !compact || tall || roomy
    readonly property real stageHeight: column.parent ? column.parent.height : 0
    readonly property bool showAir: tall || (!compact && stageHeight >= 200 * Theme.scale)

    // One station's weather: label, temperature, sky, then wind/local time and air (full layout).
    component Side: ColumnLayout {
        id: side
        property string key: "de"
        readonly property var w: root.widget
        readonly property bool has: w !== null && w[key + "Temp"].length > 0
        spacing: 2 * Theme.scale
        // Label, temperature and sky as in a Metric, but plain wrapping texts: a place name takes
        // as many lines as a narrow column (or a pane the card has grown) needs, never cut short.
        Text {
            text: side.w && side.w[side.key + "City"].length > 0 ? side.key.toUpperCase() + " · " + side.w[side.key + "City"] : side.key.toUpperCase()
            color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase; font.letterSpacing: 0.6
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            text: side.has ? side.w[side.key + "Temp"] : "—"
            color: side.has ? Theme.ink : Theme.secondary
            font.pixelSize: Theme.fontValue; font.weight: Font.DemiBold
            Accessible.name: side.key.toUpperCase() + " temperature " + text
        }
        Text {
            visible: side.has
            text: side.has ? side.w[side.key + "Summary"] : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: root.full && side.has
            text: side.has ? side.w[side.key + "Wind"] + (side.w[side.key + "Local"].length > 0 ? " · " + side.w[side.key + "Local"] : "") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: root.showAir && side.has
            text: side.has ? side.w[side.key + "Air"] : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }

    ColumnLayout {
        id: column
        width: parent.width
        spacing: Theme.gap * (root.tall ? 1 : 0.5)
        GridLayout {
            Layout.fillWidth: true
            columns: root.tall ? 1 : 2
            columnSpacing: Theme.gap * 1.5
            rowSpacing: Theme.gap
            Side {
                key: "de"
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.preferredWidth: 1
            }
            Side {
                key: "dx"
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.preferredWidth: 1
            }
        }
        Flow {
            // A short pane (phone strip) leaves the links out rather than pushing them under the footer.
            visible: root.full && root.widget && root.widget.deWindyUrl.length > 0 && root.stageHeight >= 130 * Theme.scale
            Layout.fillWidth: true
            spacing: Theme.gap * 2
            // Both links open in the system browser; the app itself never fetches them.
            Text {
                text: "Windy map ↗"
                color: Theme.accent
                font.pixelSize: Theme.fontCaption
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -Theme.gap
                    onClicked: Qt.openUrlExternally(root.widget.deWindyUrl)
                }
            }
            Text {
                text: "WEFAX charts ↗"
                color: Theme.accent
                font.pixelSize: Theme.fontCaption
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -Theme.gap
                    onClicked: Qt.openUrlExternally(root.widget.wefaxUrl)
                }
            }
        }
    }
}
