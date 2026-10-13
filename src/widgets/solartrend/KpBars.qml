import QtQuick
import HamClock

// 3-hour Kp bars, oldest first. `bars` entries are {kp, predicted, level};
// forecast bars are drawn translucent. Scale is fixed 0-9 so bars compare
// across refreshes; a faint line marks Kp 5 (NOAA G1 storm threshold).
Canvas {
    id: root
    property var bars: []
    implicitHeight: 56 * Theme.scale
    Accessible.ignored: true
    onBarsChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var n = bars ? bars.length : 0
        if (n < 1 || width <= 0 || height <= 0) return
        var gap = Math.max(1, Math.round(2 * Theme.scale))
        var bw = Math.max(1, (width - gap * (n - 1)) / n)
        function y(v) { return height * (1 - v / 9) }
        ctx.strokeStyle = Theme.divider
        ctx.lineWidth = 1
        ctx.beginPath(); ctx.moveTo(0, y(5)); ctx.lineTo(width, y(5)); ctx.stroke()
        var colors = [Theme.accent, Theme.warn, Theme.bad]
        for (var i = 0; i < n; ++i) {
            ctx.globalAlpha = bars[i].predicted ? 0.4 : 1
            ctx.fillStyle = colors[bars[i].level]
            var top = y(Math.max(bars[i].kp, 0.25))
            ctx.fillRect(i * (bw + gap), top, bw, height - top)
        }
    }
    Connections { target: Theme; function onDarkChanged() { root.requestPaint() } }
}
