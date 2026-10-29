import QtQuick
import HamClock

Card {
    id: root
    // Sunrise (amber) and sunset (blue) minutes after 00:00 (UTC, or DE local time when the zone is known) through the year, today marked.
    readonly property var pts: root.widget ? root.widget.points : []
    onPtsChanged: plot.requestPaint()
    readonly property string tzTag: root.widget && root.widget.localTime ? "L" : "Z"
    onTzTagChanged: plot.requestPaint()
    Text {
        visible: root.widget && root.widget.hasStation && root.pts.length === 0
        text: "—"
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption * 1.3
    }
    Canvas {
        id: plot
        // Drawn at device resolution (fit times the stage) so lines and labels stay crisp when the card grows it.
        width: parent.width * root.fit
        height: parent.height * root.fit
        scale: 1 / root.fit
        transformOrigin: Item.TopLeft
        visible: root.pts.length > 1
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections { target: root; function onFitChanged() { plot.requestPaint() } }
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const p = root.pts
            if (!p || p.length < 2 || !root.widget) return
            const w = width, h = height, n = root.widget.daysInYear, k = root.fit
            const x = d => d / n * w
            const y = m => h - m / 1440 * h
            ctx.strokeStyle = Theme.hairline; ctx.lineWidth = k
            // Hour guides (UTC) with labels on the left, month initials along the bottom when there is room.
            const fs = Math.max(8, Math.round(Theme.fontCaption * 0.8)) * k
            const roomy = h / k > 90 && w / k > 120
            ctx.font = fs + "px sans-serif"; ctx.fillStyle = Theme.secondary
            for (const hr of [6, 12, 18]) {
                ctx.globalAlpha = 0.35
                ctx.beginPath(); ctx.moveTo(0, y(hr * 60)); ctx.lineTo(w, y(hr * 60)); ctx.stroke()
                ctx.globalAlpha = 1
                if (roomy) ctx.fillText(hr + root.tzTag, 2 * k, y(hr * 60) - 2 * k)
            }
            if (roomy) {
                const mdays = [31, n === 366 ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
                let d0 = 0
                for (let m = 0; m < 12; ++m) {
                    ctx.fillText("JFMAMJJASOND"[m], x(d0) + 2 * k, h - 2 * k)
                    d0 += mdays[m]
                }
            }
            const series = [["rise", "#e0a030"], ["set", "#4a7fc0"]]
            for (const s of series) {
                ctx.strokeStyle = s[1]; ctx.lineWidth = 2 * k
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
            ctx.strokeStyle = Theme.ink; ctx.lineWidth = k
            ctx.beginPath(); ctx.moveTo(x(root.widget.today), 0); ctx.lineTo(x(root.widget.today), h); ctx.stroke()
        }
    }
}
