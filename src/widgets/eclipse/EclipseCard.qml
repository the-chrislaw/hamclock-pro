import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Labels and values. When the body is short the least important rows are dropped (not
    // clipped); a tall pane shows every row, values wrapping; the card grows and centres the table.
    // One row's real pitch (line height plus spacing): a larger estimate drops a row that fits.
    readonly property real rowH: Math.ceil(Theme.fontCaption * 1.3 * 1.2) + Theme.gap
    readonly property int rowsFit: tall ? 99 : Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    function v(s) { return s ? s : "—" }
    // Display order; `rank` is the order in which rows are kept when space runs out.
    readonly property var allRows: {
        const w = root.widget
        if (!w) return []
        return [
            { label: "Next", value: v(w.kind), bold: true, rank: 0 },
            { label: "Greatest", value: v(w.when), rank: 1 },
            { label: "In", value: v(w.countdown), rank: 2 },
            { label: "At DE", value: w.visibility, bold: true, rank: 3, hide: w.visibility === "" },
            { label: "Local", value: w.local, rank: 5, hide: w.local === "" },
            { label: "Partial", value: w.contacts, rank: 6, hide: w.contacts === "" },
            { label: w.centralLabel, value: w.central, bold: true, rank: 7, hide: w.central === "" },
            { label: "Path", value: v(w.detail), rank: 4 }
        ].filter(r => !r.hide)
    }
    readonly property var rows: {
        const keep = allRows.slice().sort((a, b) => a.rank - b.rank).slice(0, Math.max(0, rowsFit)).map(r => r.rank)
        return allRows.filter(r => keep.indexOf(r.rank) >= 0)
    }
    GridLayout {
        id: grid
        width: parent.width
        // Rows keep their natural pitch (a spread table stops reading as one); the card grows the block and centres it.
        columns: 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Repeater {
            model: root.rows.length * 2
            Text {
                required property int index
                readonly property var row: root.rows[Math.floor(index / 2)] || ({})
                readonly property bool isLabel: index % 2 === 0
                text: (isLabel ? row.label : row.value) || ""
                color: isLabel ? Theme.secondary : Theme.ink
                font.pixelSize: isLabel ? Theme.fontCaption : Theme.fontCaption * 1.3
                font.weight: !isLabel && row.bold ? Font.DemiBold : Font.Normal
                // Long values shrink a little in a narrow slot and wrap in a tall one.
                fontSizeMode: isLabel || root.tall ? Text.FixedSize : Text.HorizontalFit
                minimumPixelSize: Theme.fontCaption * 0.85
                wrapMode: root.tall && !isLabel ? Text.WordWrap : Text.NoWrap
                elide: isLabel || root.tall ? Text.ElideNone : Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                Layout.fillWidth: !isLabel
                Layout.preferredWidth: isLabel ? implicitWidth : 0
                Layout.maximumHeight: root.tall ? Theme.fontBody * 4 : Number.POSITIVE_INFINITY
            }
        }
    }
}
