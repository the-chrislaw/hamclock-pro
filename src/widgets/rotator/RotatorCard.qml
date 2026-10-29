import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property real azimuthDeg: root.widget && root.widget.azimuth !== "" ? parseFloat(root.widget.azimuth) : NaN
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || (compact && height >= 260 * Theme.scale)
    // Stacked: azimuth and elevation over a compass dial that takes the leftover height.
    designWidth: stacked ? Math.min(260 * Theme.scale, Math.max(150 * Theme.scale, values.implicitWidth))
                      : (compact ? 260 : 420) * Theme.scale
    onDesignWidthChanged: Qt.callLater(refit)
    ColumnLayout {
        visible: root.stacked
        width: parent.width
        height: parent.height
        spacing: Theme.gap
        RowLayout {
            id: values
            Layout.fillWidth: true
            spacing: 12 * Theme.scale
            Metric {
                label: "Azimuth"
                available: root.widget && root.widget.azimuth !== ""
                value: available ? root.widget.azimuth : ""
            }
            Metric {
                label: "Elevation"
                available: root.widget && root.widget.elevation !== ""
                value: available ? root.widget.elevation : ""
            }
        }
        Canvas {
            id: dial
            visible: !isNaN(root.azimuthDeg)
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 60 * Theme.scale
            Layout.preferredHeight: 110 * Theme.scale
            readonly property real azimuth: root.azimuthDeg
            onAzimuthChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Accessible.role: Accessible.Graphic
            Accessible.name: "Compass, antenna pointing " + Math.round(azimuth) + " degrees"
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                const label = Theme.fontCaption
                const r = Math.min(width, height) / 2 - label - 2
                if (r <= 4 || isNaN(azimuth)) return
                const cx = width / 2, cy = height / 2
                ctx.strokeStyle = Theme.divider
                ctx.lineWidth = 1.5
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.stroke()
                for (let d = 0; d < 360; d += 30) {
                    const a = d * Math.PI / 180, inner = d % 90 === 0 ? r * 0.82 : r * 0.9
                    ctx.beginPath()
                    ctx.moveTo(cx + Math.sin(a) * inner, cy - Math.cos(a) * inner)
                    ctx.lineTo(cx + Math.sin(a) * r, cy - Math.cos(a) * r)
                    ctx.stroke()
                }
                ctx.fillStyle = Theme.secondary
                ctx.font = "600 " + label + "px sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                const marks = [["N", 0], ["E", 90], ["S", 180], ["W", 270]]
                for (const m of marks) {
                    const a = m[1] * Math.PI / 180, rr = r + label * 0.75
                    ctx.fillText(m[0], cx + Math.sin(a) * rr, cy - Math.cos(a) * rr)
                }
                const a = azimuth * Math.PI / 180
                ctx.strokeStyle = Theme.accent
                ctx.lineWidth = 3
                ctx.lineCap = "round"
                ctx.beginPath(); ctx.moveTo(cx, cy)
                ctx.lineTo(cx + Math.sin(a) * r * 0.86, cy - Math.cos(a) * r * 0.86); ctx.stroke()
                ctx.fillStyle = Theme.accent
                ctx.beginPath(); ctx.arc(cx, cy, 3.5, 0, 2 * Math.PI); ctx.fill()
            }
        }
    }
    Flow {
        visible: !root.stacked
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Azimuth"
            available: root.widget && root.widget.azimuth !== ""
            value: available ? root.widget.azimuth : ""
        }
        Metric {
            label: "Elevation"
            visible: root.widget && root.widget.elevation !== ""
            value: visible ? root.widget.elevation : ""
        }
    }
}
