import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Labels and values; the lower rows are dropped (not clipped) when the body is short.
    // A tall pane shows every row, values wrapping, the card grows and centres the table.
    // One row's real pitch (line height plus spacing): a larger estimate drops a row that fits.
    readonly property real rowH: Math.ceil(Theme.fontCaption * 1.3 * 1.2) + Theme.gap
    readonly property int rowsFit: tall ? 99 : Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    function v(s) { return s ? s : "—" }
    // A narrow slot drops the bracketed extra ("(6 h 24 min)", "(405600 km)") so the value fits.
    function brief(s) { return compact && !tall ? s.replace(/ \([^)]*\)$/, "") : s }
    readonly property var rows: {
        const w = root.widget
        if (!w) return []
        return [
            { label: "Moon DE", value: v(w.dePosition) },
            { label: "Moon DX", value: v(w.dxPosition) },
            { label: "Mutual", value: brief(v(w.window)), bold: w.mutualNow, dim: !w.mutualNow },
            { label: "Distance", value: v(w.distance) },
            { label: "Path loss", value: v(w.degradation) },
            { label: "Declination", value: v(w.declination) },
            { label: "Sun", value: v(w.sun) },
            { label: "Apsis", value: brief(v(w.apsis)) }
        ].slice(0, Math.max(3, rowsFit))
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
                color: isLabel || row.dim ? Theme.secondary : Theme.ink
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
