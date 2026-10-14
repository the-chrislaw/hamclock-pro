import QtQuick
import HamClock

// GOES X-ray 24 h history on a log axis. `values` are 0..1 plot positions
// (A1 at the bottom, X10 at the top), oldest first. Faint lines mark the
// B, C, M and X class thresholds; the line turns warm above M and X.
Canvas {
    id: root
    property var values: []
    Accessible.ignored: true
    onValuesChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var n = values ? values.length : 0
        if (n < 2 || width <= 0 || height <= 0) return
        var pad = 2
        function y(f) { return pad + (height - 2 * pad) * (1 - f) }
        ctx.lineWidth = 1
        ctx.strokeStyle = Theme.divider
        ctx.fillStyle = Theme.secondary
        ctx.font = Math.round(Theme.fontCaption * 0.8) + "px sans-serif"
        var marks = [["B", 0.2], ["C", 0.4], ["M", 0.6], ["X", 0.8]]
        for (var m = 0; m < marks.length; ++m) {
            ctx.beginPath(); ctx.moveTo(0, y(marks[m][1])); ctx.lineTo(width, y(marks[m][1])); ctx.stroke()
            if (height > 48) ctx.fillText(marks[m][0], 2, y(marks[m][1]) - 2)
        }
        ctx.lineWidth = Math.max(1.5, 1.5 * Theme.scale)
        ctx.lineJoin = "round"
        for (var i = 1; i < n; ++i) {
            var peak = Math.max(values[i - 1], values[i])
            ctx.strokeStyle = peak >= 0.8 ? Theme.bad : peak >= 0.6 ? Theme.warn : Theme.accent
            ctx.beginPath()
            ctx.moveTo(pad + (width - 2 * pad) * (i - 1) / (n - 1), y(values[i - 1]))
            ctx.lineTo(pad + (width - 2 * pad) * i / (n - 1), y(values[i]))
            ctx.stroke()
        }
    }
    Connections { target: Theme; function onDarkChanged() { root.requestPaint() } }
}
