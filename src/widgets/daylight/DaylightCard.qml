import QtQuick
import HamClock

Card {
    id: root
    // Sunrise (amber) and sunset (blue) minutes after 00:00 UTC through the year, today marked.
    readonly property var pts: root.widget ? root.widget.points : []
    onPtsChanged: plot.requestPaint()
    Text {
        visible: root.widget && root.widget.hasStation && root.pts.length === 0
        text: "—"
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption * 1.3
    }
    Canvas {
        id: plot
        anchors.fill: parent
        visible: root.pts.length > 1
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const p = root.pts
            if (!p || p.length < 2 || !root.widget) return
            const w = width, h = height, n = root.widget.daysInYear
            const x = d => d / n * w
            const y = m => h - m / 1440 * h
            ctx.strokeStyle = Theme.hairline; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(0, h / 2); ctx.lineTo(w, h / 2); ctx.stroke()
            const series = [["rise", "#e0a030"], ["set", "#4a7fc0"]]
            for (const s of series) {
                ctx.strokeStyle = s[1]; ctx.lineWidth = 2
                ctx.beginPath()
                let pen = false, prev = 0
                for (const q of p) {
                    const v = q[s[0]]
                    if (v < 0) { pen = false; continue }
                    // Break the path where the time wraps past 00:00 UTC instead of drawing a vertical stroke.
                    if (pen && Math.abs(v - prev) > 720) pen = false
                    prev = v
                    if (pen) ctx.lineTo(x(q.day - 1), y(v)); else ctx.moveTo(x(q.day - 1), y(v))
                    pen = true
                }
                ctx.stroke()
            }
            ctx.strokeStyle = Theme.ink; ctx.lineWidth = 1
            ctx.beginPath(); ctx.moveTo(x(root.widget.today), 0); ctx.lineTo(x(root.widget.today), h); ctx.stroke()
        }
    }
}
