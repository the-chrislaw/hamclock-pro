import QtQuick
import HamClock

// Minimal history line. `values` are oldest first; drawn only with 2+ points.
// With `zeroLine` a faint baseline marks 0 (e.g. Bz north/south).
Canvas {
    id: root
    property var values: []
    property color lineColor: Theme.accent
    property bool zeroLine: false
    implicitHeight: 36 * Theme.scale
    Accessible.ignored: true
    onValuesChanged: requestPaint()
    onLineColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var n = values ? values.length : 0
        if (n < 2 || width <= 0 || height <= 0) return
        var lo = Math.min.apply(null, values), hi = Math.max.apply(null, values)
        if (zeroLine) { lo = Math.min(lo, 0); hi = Math.max(hi, 0) }
        if (hi - lo < 1e-9) { hi += 1; lo -= 1 }
        var pad = 2
        function y(v) { return pad + (height - 2 * pad) * (1 - (v - lo) / (hi - lo)) }
        if (zeroLine) {
            ctx.strokeStyle = Theme.divider
            ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(0, y(0)); ctx.lineTo(width, y(0)); ctx.stroke()
        }
        ctx.strokeStyle = lineColor
        ctx.lineWidth = Math.max(1.5, 1.5 * Theme.scale)
        ctx.lineJoin = "round"
        ctx.beginPath()
        for (var i = 0; i < n; ++i) {
            var x = pad + (width - 2 * pad) * i / (n - 1)
            if (i === 0) ctx.moveTo(x, y(values[i])); else ctx.lineTo(x, y(values[i]))
        }
        ctx.stroke()
    }
    Connections { target: Theme; function onDarkChanged() { root.requestPaint() } }
}
