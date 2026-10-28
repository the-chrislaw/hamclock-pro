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
    // Short panes (phones in portrait) tighten the rows, then split them into two columns, so every band shows.
    readonly property real bandWidth: 44 * Theme.scale
    readonly property real dayWidth: 70 * Theme.scale
    readonly property real regularPitch: Theme.fontBody * 1.5 + 2 * Theme.scale
    readonly property real densePitch: Theme.fontBody * 1.15
    readonly property real rowWidth: bandWidth + dayWidth + 2 * Theme.gap + nightMetrics.advanceWidth
    readonly property real listHeight: content.parent.height - (inputsLine.visible ? inputsLine.height + content.spacing : 0)
    readonly property bool twoColumns: rows.length * densePitch > listHeight && 2 * rowWidth + Theme.gap <= content.width
    readonly property int rowsPerColumn: twoColumns ? Math.ceil(rows.length / 2) : rows.length
    readonly property real rowPitch: rowsPerColumn * regularPitch <= listHeight ? regularPitch
        : Math.max(densePitch, Math.floor(listHeight / Math.max(1, rowsPerColumn)))
    TextMetrics { id: nightMetrics; text: "Night Good"; font.pixelSize: Theme.fontBody; font.weight: Font.Bold }
    ColumnLayout {
        id: content
        width: parent.width
        spacing: Theme.gap * 0.4
        Text {
            visible: !(root.widget && root.widget.hasOutlook)
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
        }
        Text {
            id: inputsLine
            visible: root.widget && root.widget.hasOutlook && !root.compact
            text: root.widget ? root.widget.inputsText + " · now: " + root.widget.period : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
        GridLayout {
            visible: !root.compact && root.widget && root.widget.hasOutlook
            // Top to bottom so a split list still reads 80m..10m down each column.
            flow: GridLayout.TopToBottom
            rows: Math.max(1, root.rowsPerColumn)
            columnSpacing: Theme.gap
            rowSpacing: 0
            Layout.fillWidth: true
            // Regular layout: one row per band with Day/Night.
            Repeater {
                model: root.compact ? [] : root.rows
                delegate: Item {
                    Layout.fillWidth: true
                    implicitWidth: root.rowWidth
                    implicitHeight: root.rowPitch
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.gap
                        Text { width: root.bandWidth; text: modelData.band; color: Theme.ink
                               font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold }
                        Text { width: root.dayWidth; text: "Day " + modelData.day
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
