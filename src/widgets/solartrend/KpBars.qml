import QtQuick
import HamClock

// 3-hour Kp bars, oldest first. `bars` entries are {kp, predicted, level};
// forecast bars are drawn translucent. Scale is fixed 0-9 so bars compare
// across refreshes; a faint line marks Kp 5 (NOAA G1 storm threshold).
// `pixelScale` (the card's fit) paints at the drawn size so the bars stay crisp when the pane scales up.
Item {
    id: root
    property var bars: []
    property real pixelScale: 1
    implicitHeight: 56 * Theme.scale
    Accessible.ignored: true
    onBarsChanged: canvas.requestPaint()
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
            var bars = root.bars
            var n = bars ? bars.length : 0
            var w = root.width, h = root.height
            if (n < 1 || w <= 0 || h <= 0) return
            ctx.scale(k, k)
            var gap = Math.max(1, Math.round(2 * Theme.scale))
            var bw = Math.max(1, (w - gap * (n - 1)) / n)
            function y(v) { return h * (1 - v / 9) }
            ctx.strokeStyle = Theme.divider
            ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(0, y(5)); ctx.lineTo(w, y(5)); ctx.stroke()
            var colors = [Theme.accent, Theme.warn, Theme.bad]
            for (var i = 0; i < n; ++i) {
                ctx.globalAlpha = bars[i].predicted ? 0.4 : 1
                ctx.fillStyle = colors[bars[i].level]
                var top = y(Math.max(bars[i].kp, 0.25))
                ctx.fillRect(i * (bw + gap), top, bw, h - top)
            }
        }
    }
    Connections { target: Theme; function onDarkChanged() { canvas.requestPaint() } }
}
