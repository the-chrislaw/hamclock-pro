import QtQuick
import QtQuick.Layouts
import HamClock

// Day/night HF outlook per band. A rough estimate from SFI/Kp/A, labelled as such.
Card {
    id: root
    readonly property var rows: widget ? widget.bands : []
    function conditionColor(name) {
        return name === "Good" ? Theme.good : name === "Fair" ? Theme.warn : Theme.bad
    }
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap * 0.4
        Text {
            visible: !(root.widget && root.widget.hasOutlook)
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
        }
        Text {
            visible: root.widget && root.widget.hasOutlook && !root.compact
            text: root.widget ? root.widget.inputsText + " · now: " + root.widget.period : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        GridLayout {
            visible: !root.compact && root.widget && root.widget.hasOutlook
            columns: 3
            columnSpacing: Theme.gap
            rowSpacing: 2 * Theme.scale
            Layout.fillWidth: true
            // Regular layout: one row per band with Day/Night.
            Repeater {
                model: root.compact ? [] : root.rows
                delegate: Item {
                    Layout.columnSpan: 3
                    Layout.fillWidth: true
                    implicitHeight: Theme.fontBody * 1.5
                    Row {
                        spacing: Theme.gap
                        Text { width: 44 * Theme.scale; text: modelData.band; color: Theme.ink
                               font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                        Text { width: 70 * Theme.scale; text: "Day " + modelData.day
                               color: root.conditionColor(modelData.day)
                               font.pixelSize: Theme.fontBody
                               font.weight: root.widget.period === "day" ? Font.Bold : Font.Normal }
                        Text { text: "Night " + modelData.night
                               color: root.conditionColor(modelData.night)
                               font.pixelSize: Theme.fontBody
                               font.weight: root.widget.period === "night" ? Font.Bold : Font.Normal }
                    }
                }
            }
        }
        // Compact layout: band + current-period condition only, wrapping.
        Flow {
            visible: root.compact && root.widget && root.widget.hasOutlook
            spacing: Theme.gap
            Layout.fillWidth: true
            Repeater {
                model: root.compact ? root.rows : []
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
