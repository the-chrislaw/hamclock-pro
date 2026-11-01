import QtQuick
import HamClock

Card {
    id: root
    // Sunrise (amber) and sunset (blue) minutes after 00:00 (UTC, or DE local time when the zone is known) through the year, today marked.
    readonly property var pts: root.widget ? root.widget.points : []
    onPtsChanged: plot.requestPaint()
    readonly property string tzTag: root.widget && root.widget.localTime ? "L" : "Z"
    onTzTagChanged: plot.requestPaint()
    // Minutes after 00:00 with sunset carried past midnight (UTC, or a far-east zone) so each
    // curve is continuous; -1 for no sunrise or sunset (polar day or night).
    function unwrapped(q, key) {
        const v = q[key]
        if (v < 0) return -1
        return key === "set" && q.rise >= 0 && v < q.rise ? v + 1440 : v
    }
    // Vertical range: the year's earliest sunrise to latest sunset, padded to whole hours.
    function span(points) {
        let lo = 1e9, hi = -1e9
        for (const q of points) {
            for (const key of ["rise", "set"]) {
                const v = unwrapped(q, key)
                if (v >= 0) { lo = Math.min(lo, v); hi = Math.max(hi, v) }
            }
        }
        if (hi < lo) return { lo: 0, hi: 1440 }
        return { lo: Math.floor(lo / 60 - 1) * 60, hi: Math.ceil(hi / 60 + 1) * 60 }
    }
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
            const k = root.fit, n = root.widget.daysInYear
            const fs = Math.max(8, Math.round(Theme.fontCaption * 0.8)) * k
            const roomy = height / k > 90 && width / k > 120
            ctx.font = fs + "px sans-serif"
            // Labels get their own gutter (left) and strip (bottom) so no curve runs through them.
            const left = roomy ? Math.ceil(ctx.measureText("18Z").width) + 4 * k : 0
            const bottom = roomy ? fs + 4 * k : 0
            const w = width - left, h = height - bottom
            const range = root.span(p)
            const x = d => left + d / n * w
            const y = m => (range.hi - m) / (range.hi - range.lo) * h
            ctx.fillStyle = Theme.secondary
            ctx.strokeStyle = Theme.secondary; ctx.lineWidth = k
            for (let hr = Math.ceil(range.lo / 360) * 6; hr * 60 <= range.hi; hr += 6) {
                ctx.globalAlpha = 0.35
                ctx.beginPath(); ctx.moveTo(left, y(hr * 60)); ctx.lineTo(left + w, y(hr * 60)); ctx.stroke()
                ctx.globalAlpha = 1
                if (roomy) ctx.fillText(((hr % 24) + 24) % 24 + root.tzTag, 0, y(hr * 60) + fs * 0.35)
            }
            if (roomy) {
                const mdays = [31, n === 366 ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]
                let d0 = 0
                for (let m = 0; m < 12; ++m) {
                    ctx.fillText("JFMAMJJASOND"[m], x(d0) + 2 * k, height - 2 * k)
                    d0 += mdays[m]
                }
            }
            for (const s of [["rise", "#e0a030"], ["set", "#4a7fc0"]]) {
                ctx.strokeStyle = s[1]; ctx.lineWidth = 2 * k
                ctx.beginPath()
                let pen = false
                for (const q of p) {
                    const v = root.unwrapped(q, s[0])
                    if (v < 0) { pen = false; continue }
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
