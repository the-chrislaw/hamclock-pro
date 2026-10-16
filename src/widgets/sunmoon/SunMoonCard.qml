import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Rows that fit in the body: header + 3 sun rows + Moon title are fixed;
    // the later Moon rows are dropped (Now first, then rise/set, then phase)
    // instead of being clipped. Estimate is non-circular (no layout feedback).
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.25 + Theme.gap
    readonly property int rowsFit: Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    readonly property bool showPhase: rowsFit >= 6
    readonly property bool showRiseSet: rowsFit >= 7
    readonly property bool showNow: rowsFit >= 8
    GridLayout {
        id: grid
        width: parent.width
        columns: 3
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Text { text: ""; Layout.fillWidth: true }
        Text { text: "DE"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
        Text { text: "DX"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }

        Text { text: "Sunrise"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deRise ? root.widget.deRise : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxRise ? root.widget.dxRise : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text { text: "Sunset"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deSet ? root.widget.deSet : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxSet ? root.widget.dxSet : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text { text: "Day length"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deDay ? root.widget.deDay : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxDay ? root.widget.dxDay : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text {
            text: "Moon (DE)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            Layout.columnSpan: root.showPhase ? 2 : 3; Layout.topMargin: Theme.gap
        }
        // Moon disc drawn from the elongation: lit limb on the right when
        // waxing (northern observer), mirrored in the south. Orientation
        // ignores the parallactic angle.
        Canvas {
            id: disc
            visible: root.showPhase && !!root.widget && !!root.widget.moonPhase
            Layout.rowSpan: 2; Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredWidth: Math.min(rowH * 2, 96); Layout.preferredHeight: Layout.preferredWidth
            readonly property real elong: root.widget ? root.widget.moonElongation : 0
            readonly property bool south: root.widget ? root.widget.moonSouthern : false
            readonly property color ink: Theme.ink
            onElongChanged: requestPaint()
            onSouthChanged: requestPaint()
            onInkChanged: requestPaint()
            onWidthChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                const r = Math.min(width, height) / 2 - 1, cx = width / 2, cy = height / 2;
                const k = Math.cos(elong * Math.PI / 180);   // 1 new .. -1 full
                const waxing = elong < 180;
                const litRight = waxing !== south;
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                ctx.fillStyle = Qt.rgba(ink.r, ink.g, ink.b, 0.15); ctx.fill();
                // lit region: half disc on the lit side, bounded by the terminator ellipse
                ctx.beginPath();
                const dir = litRight ? 1 : -1;
                ctx.moveTo(cx, cy - r);
                ctx.arc(cx, cy, r, -Math.PI / 2, Math.PI / 2, !litRight);
                const steps = 40;
                for (let i = 0; i <= steps; ++i) {
                    const a = Math.PI / 2 - Math.PI * i / steps;      // bottom to top
                    // terminator bulges toward the lit side for a crescent (k > 0)
                    ctx.lineTo(cx + dir * k * r * Math.cos(a), cy + r * Math.sin(a));
                }
                ctx.closePath();
                ctx.fillStyle = "#e8d9a0"; ctx.fill();
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                ctx.strokeStyle = Qt.rgba(ink.r, ink.g, ink.b, 0.5); ctx.lineWidth = 1; ctx.stroke();
            }
        }
        Text {
            visible: root.showPhase
            text: root.widget && root.widget.moonPhase ? root.widget.moonPhase + " " + root.widget.moonIllumination : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.85
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text { visible: root.showRiseSet; text: "Moon rise/set"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showRiseSet
            text: (root.widget && root.widget.moonRise ? root.widget.moonRise : "—") + " / " + (root.widget && root.widget.moonSet ? root.widget.moonSet : "—")
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
        }
        Text { visible: root.showRiseSet; text: "Next season"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showRiseSet
            text: root.widget && root.widget.nextSeason ? root.widget.nextSeason : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.85
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text { visible: root.showNow; text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.showNow; text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2 }

        Text {
            visible: text.length > 0
            text: root.widget ? [root.widget.deNote ? "DE: " + root.widget.deNote : "",
                                 root.widget.dxNote ? "DX: " + root.widget.dxNote : ""].filter(Boolean).join("   ") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            Layout.columnSpan: 3
        }
    }
}
