import QtQuick
import HamClock

// 48 h Dst history, oldest first. Faint lines mark 0 and the -50 nT storm threshold.
Canvas {
    id: root
    property var values: []
    property color lineColor: Theme.accent
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
        var lo = Math.min(Math.min.apply(null, values), -60), hi = Math.max(Math.max.apply(null, values), 10)
        var pad = 2
        function y(v) { return pad + (height - 2 * pad) * (1 - (v - lo) / (hi - lo)) }
        ctx.strokeStyle = Theme.divider
        ctx.lineWidth = 1
        ctx.beginPath(); ctx.moveTo(0, y(0)); ctx.lineTo(width, y(0))
        ctx.moveTo(0, y(-50)); ctx.lineTo(width, y(-50)); ctx.stroke()
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
