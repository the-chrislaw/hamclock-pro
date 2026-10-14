import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Next passes at DE: satellite, when, peak elevation (elevation hidden in a compact slot).
    // Roomy slots also get a polar sky plot of the first pass (north up, zenith at the centre).
    readonly property bool showSky: width > 240 * Theme.scale && height > 150 * Theme.scale && widget && widget.skyTrack.length > 1
    RowLayout {
        width: parent.width
        spacing: Theme.gap * 1.5
        GridLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            columns: root.compact ? 2 : 3
            columnSpacing: Theme.gap * 1.5
            rowSpacing: root.compact ? Theme.gap * 0.4 : Theme.gap
            Repeater {
                model: root.widget ? root.widget.rows.length * (root.compact ? 2 : 3) : 0
                Text {
                    required property int index
                    readonly property int cols: root.compact ? 2 : 3
                    readonly property var row: root.widget.rows[Math.floor(index / cols)]
                    readonly property int col: index % cols
                    text: col === 0 ? row.name : col === 1 ? row.when : "max " + row.maxEl + "°"
                    color: col === 0 ? Theme.ink : Theme.secondary
                    font.pixelSize: Theme.fontCaption * (col === 0 ? 1.3 : 1)
                    font.weight: col === 0 || row.live ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                    Layout.fillWidth: col === cols - 1
                }
            }
        }
        Canvas {
            id: sky
            visible: root.showSky
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Math.min(root.height * 0.45, 110 * Theme.scale)
            Layout.preferredHeight: Layout.preferredWidth
            readonly property var track: root.widget ? root.widget.skyTrack : []
            onTrackChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                const r = Math.min(width, height) / 2 - 2, cx = width / 2, cy = height / 2;
                ctx.strokeStyle = Theme.secondary; ctx.lineWidth = 1;
                for (const f of [1, 2 / 3, 1 / 3]) { ctx.beginPath(); ctx.arc(cx, cy, r * f, 0, 2 * Math.PI); ctx.stroke(); }
                ctx.beginPath(); ctx.moveTo(cx - r, cy); ctx.lineTo(cx + r, cy); ctx.moveTo(cx, cy - r); ctx.lineTo(cx, cy + r); ctx.stroke();
                if (track.length < 2) return;
                ctx.strokeStyle = Theme.accent; ctx.lineWidth = 2; ctx.beginPath();
                for (let i = 0; i < track.length; ++i) {
                    const rr = r * (90 - Math.max(0, track[i].el)) / 90, a = track[i].az * Math.PI / 180;
                    const x = cx + rr * Math.sin(a), y = cy - rr * Math.cos(a);
                    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
                }
                ctx.stroke();
            }
        }
    }
    Text {
        visible: root.widget && root.widget.hasData && root.widget.rows.length === 0
        text: root.widget.allStale ? "Element sets over 30 days old" : "No passes in the next 24 h"
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption
    }
}
