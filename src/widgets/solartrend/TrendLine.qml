import QtQuick
import HamClock

// Minimal history line. `values` are oldest first; drawn only with 2+ points.
// With `zeroLine` a faint baseline marks 0 (e.g. Bz north/south).
// `pixelScale` (the card's fit) paints at the drawn size so the line stays crisp when the pane scales up.
Item {
    id: root
    property var values: []
    property color lineColor: Theme.accent
    property bool zeroLine: false
    property real pixelScale: 1
    implicitHeight: 36 * Theme.scale
    Accessible.ignored: true
    onValuesChanged: canvas.requestPaint()
    onLineColorChanged: canvas.requestPaint()
    onZeroLineChanged: canvas.requestPaint()
    Canvas {
        id: canvas
        readonly property real k: Math.max(1, root.pixelScale)
        width: root.width * k
        height: root.height * k
        scale: 1 / k
        transformOrigin: Item.TopLeft
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var values = root.values
            var n = values ? values.length : 0
            var w = root.width, h = root.height
            if (n < 2 || w <= 0 || h <= 0) return
            ctx.scale(k, k)
            var lo = Math.min.apply(null, values), hi = Math.max.apply(null, values)
            if (root.zeroLine) { lo = Math.min(lo, 0); hi = Math.max(hi, 0) }
            if (hi - lo < 1e-9) { hi += 1; lo -= 1 }
            var pad = 2
            function y(v) { return pad + (h - 2 * pad) * (1 - (v - lo) / (hi - lo)) }
            if (root.zeroLine) {
                ctx.strokeStyle = Theme.divider
                ctx.lineWidth = 1
                ctx.beginPath(); ctx.moveTo(0, y(0)); ctx.lineTo(w, y(0)); ctx.stroke()
            }
            ctx.strokeStyle = root.lineColor
            ctx.lineWidth = Math.max(1.5, 1.5 * Theme.scale)
            ctx.lineJoin = "round"
            ctx.beginPath()
            for (var i = 0; i < n; ++i) {
                var x = pad + (w - 2 * pad) * i / (n - 1)
                if (i === 0) ctx.moveTo(x, y(values[i])); else ctx.lineTo(x, y(values[i]))
            }
            ctx.stroke()
        }
    }
    Connections { target: Theme; function onDarkChanged() { canvas.requestPaint() } }
}
