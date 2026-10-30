import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Next 48 h of passes at DE: satellite, when, peak elevation, rise -> set direction. Roomy
    // slots add local time, duration and a filter row; compact ones show the first rows only.
    // Rows that fit the stage (stage units: the card scales the stage by `fit`), never clipped.
    FontMetrics { id: rowFont; font.pixelSize: Theme.fontCaption * 1.2 }
    FontMetrics { id: filterFont; font.pixelSize: Theme.fontCaption }
    readonly property real stageHeight: passColumn.parent ? passColumn.parent.height : 0
    readonly property real pitch: Math.ceil(rowFont.height) + Theme.gap * 0.3
    readonly property real headerHeight: compact ? 0 : Math.ceil(filterFont.height) + Theme.gap * 0.5
    readonly property int shown: Math.max(0, Math.min(widget ? widget.rows.length : 0,
        Math.floor((stageHeight - headerHeight + Theme.gap * 0.3) / pitch)))
    // Narrow slots: today's passes show the UTC time alone so the direction column keeps its room.
    function when(row) { return row.live ? "now" : compact && row.day === "Today" ? row.utc : row.day + " " + row.utc }
    ColumnLayout {
        id: passColumn
        width: parent.width
        spacing: Theme.gap * 0.5
        Row {
            visible: !root.compact
            spacing: Theme.gap
            Repeater {
                model: [{k: "all", t: "All"}, {k: "amateur", t: "Amateur"}, {k: "visible", t: "Visible"}, {k: "starred", t: "★ Starred"}, {k: "alerts", t: "Alerts"}]
                delegate: Text {
                    required property var modelData
                    readonly property bool on: root.widget && root.widget.filter === modelData.k
                    text: modelData.t
                    color: on ? Theme.accent : Theme.secondary
                    font.pixelSize: Theme.fontCaption; font.weight: on ? Font.DemiBold : Font.Normal
                    MouseArea { anchors.fill: parent; anchors.margins: -Theme.gap * 0.6; onClicked: root.widget.setFilter(modelData.k) }
                }
            }
        }
        Text {
            visible: root.widget && root.widget.rows.length === 0
            text: root.widget ? root.widget.emptyReason : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        GridLayout {
            Layout.fillWidth: true
            columns: root.compact ? 3 : 5
            columnSpacing: Theme.gap * 1.2
            rowSpacing: Theme.gap * 0.3
            Repeater {
                model: root.widget ? root.shown * (root.compact ? 3 : 5) : 0
                Text {
                    required property int index
                    readonly property int cols: root.compact ? 3 : 5
                    readonly property var row: root.widget.rows[Math.floor(index / cols)] || ({})
                    readonly property int col: index % cols
                    text: col === 0 ? (row.starred ? "★ " : "") + row.name
                        : col === 1 ? root.when(row)
                        : col === 2 ? row.maxEl + "° " + row.from + "→" + row.to
                        : col === 3 ? row.local + " loc" : row.duration + (row.visible ? " ★" : "") + (row.mag !== "" ? " ~" + row.mag + "m" : "")
                    color: row.live ? Theme.accent : col === 0 ? Theme.ink : Theme.secondary
                    font.pixelSize: Theme.fontCaption * (col === 0 ? 1.2 : 1)
                    font.weight: col === 0 || row.live ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                    Layout.fillWidth: col === 2
                    MouseArea {
                        visible: col === 0
                        anchors.fill: parent; anchors.margins: -Theme.gap * 0.4
                        onClicked: root.widget.toggleStar(row.name)
                    }
                }
            }
        }
    }
}
