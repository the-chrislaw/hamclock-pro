import QtQuick
import QtQuick.Layouts
import HamClock

// Day/night HF outlook per band. A rough estimate from SFI/Kp/A, labelled as such.
Card {
    id: root
    // Stacked full layout: a tall pane, or a compact desktop pane with height to spare (two-row grids at
    // 1280x800 and 2560x1440). The tuned 7-inch dense layout keeps its compact summary.
    readonly property bool stacked: root.tall || (root.compact && !Theme.dense && root.height >= 240 * Theme.scale
                                                     && root.width >= 240 * Theme.scale)
    readonly property var rows: widget ? widget.bands : []
    function conditionColor(name) {
        return name === "Good" ? Theme.good : name === "Fair" ? Theme.warn : Theme.bad
    }
    // Tall pane: the full Day/Night table, rows spread over the height.
    readonly property bool table: !compact || stacked
    readonly property real rowHeight: Theme.fontBody * 1.5
    // Wide but short pane (phone strip): one band per row would run past the stage, so the table goes
    // two bands per row under a Day/Night header, and rows share whatever height there is.
    readonly property bool twoUp: table && !stacked && content.parent !== null
                                  && content.parent.width >= 300 * Theme.scale
                                  && Theme.fontCaption * 1.4 + rows.length * (rowHeight + 2 * Theme.scale) + Theme.gap
                                     > content.parent.height
    readonly property real bandWidth: 44 * Theme.scale
    readonly property real dayWidth: (twoUp ? 44 : 70) * Theme.scale
    ColumnLayout {
        id: content
        width: parent.width
        height: root.stacked || root.twoUp ? parent.height : implicitHeight
        spacing: Theme.gap * 0.4
        Text {
            visible: !(root.widget && root.widget.hasOutlook)
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
        }
        Text {
            visible: root.widget && root.widget.hasOutlook && root.table
            wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
            text: root.widget ? root.widget.inputsText + " · now: " + root.widget.period : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        GridLayout {
            visible: root.table && root.widget && root.widget.hasOutlook
            Layout.fillHeight: root.stacked || root.twoUp
            columns: root.twoUp ? 2 : 1
            columnSpacing: Theme.gap * 1.5
            rowSpacing: 2 * Theme.scale
            Layout.fillWidth: true
            // Two-up header: Day / Night over each column.
            Repeater {
                model: root.table && root.twoUp ? 2 : 0
                delegate: Row {
                    Layout.fillWidth: true
                    spacing: Theme.gap
                    Item { width: root.bandWidth; height: 1 }
                    Repeater {
                        model: [{ text: "Day", w: root.dayWidth }, { text: "Night", w: 0 }]
                        Text {
                            required property var modelData
                            width: modelData.w > 0 ? modelData.w : implicitWidth
                            text: modelData.text
                            color: Theme.secondary
                            font.pixelSize: Theme.fontCaption
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                        }
                    }
                }
            }
            // Regular layout: one row per band with Day/Night.
            Repeater {
                model: root.table ? root.rows : []
                delegate: Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: root.stacked || root.twoUp
                    Layout.minimumHeight: root.twoUp ? Theme.fontBody * 1.15 : -1
                    Layout.maximumHeight: Theme.fontBody * 3
                    implicitHeight: root.twoUp ? Theme.fontBody * 1.3 : root.rowHeight
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.gap
                        Text { width: root.bandWidth; text: modelData.band; color: Theme.ink
                               font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                        Text { width: root.dayWidth; text: (root.twoUp ? "" : "Day ") + modelData.day
                               color: root.conditionColor(modelData.day)
                               font.pixelSize: Theme.fontBody
                               font.weight: root.widget.period === "day" ? Font.Bold : Font.Normal }
                        Text { text: (root.twoUp ? "" : "Night ") + modelData.night
                               color: root.conditionColor(modelData.night)
                               font.pixelSize: Theme.fontBody
                               font.weight: root.widget.period === "night" ? Font.Bold : Font.Normal }
                    }
                }
            }
        }
        // Compact layout: band + current-period condition only, wrapping.
        Flow {
            visible: !root.table && root.widget && root.widget.hasOutlook
            spacing: Theme.gap
            Layout.fillWidth: true
            Repeater {
                model: root.table ? [] : root.rows
                delegate: Text {
                    readonly property string cond: root.widget.period === "night" ? modelData.night : modelData.day
                    text: modelData.band + " " + cond
                    color: root.conditionColor(cond)
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                }
            }
        }
    }
}
