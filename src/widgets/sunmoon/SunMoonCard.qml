import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Compact value: drop the " UTC" suffix (the DE/DX headers say UTC) and shorten durations.
    function compact(t) { return t.replace(/ UTC/g, "").replace(/ h /, "h ").replace(/ min/, "m") }
    // Rows that fit in the body: header + 3 sun rows + Moon title are fixed;
    // the later Moon rows are dropped (Now first, then rise/set, then phase)
    // instead of being clipped. Estimate is non-circular (no layout feedback).
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.4 + Theme.gap
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
        Text { text: ""; Layout.fillWidth: true; Layout.preferredWidth: 0 }
        Text { text: "DE (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
        Text { text: "DX (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }

        Text { text: "Sunrise"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deRise ? root.compact(root.widget.deRise) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxRise ? root.compact(root.widget.dxRise) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text { text: "Sunset"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deSet ? root.compact(root.widget.deSet) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxSet ? root.compact(root.widget.dxSet) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text { text: "Day length"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deDay ? root.compact(root.widget.deDay) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxDay ? root.compact(root.widget.dxDay) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text {
            text: "Moon (DE)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            Layout.columnSpan: root.showPhase ? 2 : 3; Layout.topMargin: Theme.gap
        }
        // Moon disc drawn from the elongation: lit limb on the right when
        // waxing, then rotated by the bright-limb direction at DE so it is
        // tilted as in the sky (this also flips it for a southern observer).
        Canvas {
            id: disc
            visible: root.showPhase && !!root.widget && !!root.widget.moonPhase
            Layout.rowSpan: 2; Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredWidth: Math.min(rowH * 1.6, 72); Layout.preferredHeight: Layout.preferredWidth
            readonly property real elong: root.widget ? root.widget.moonElongation : 0
            readonly property real tilt: root.widget ? root.widget.moonTilt : 0
            readonly property color ink: Theme.ink
            onElongChanged: requestPaint()
            onTiltChanged: requestPaint()
            onInkChanged: requestPaint()
            onWidthChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.translate(width / 2, height / 2);
                // counter-clockwise tilt away from the default (lit right when waxing, left when waning)
                ctx.rotate(-(tilt - (elong < 180 ? -90 : 90)) * Math.PI / 180);
                ctx.translate(-width / 2, -height / 2);
                const r = Math.min(width, height) / 2 - 1, cx = width / 2, cy = height / 2;
                const k = Math.cos(elong * Math.PI / 180);   // 1 new .. -1 full
                const waxing = elong < 180;
                const litRight = waxing;
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
            text: compact((root.widget && root.widget.moonRise ? root.widget.moonRise : "—") + " / " + (root.widget && root.widget.moonSet ? root.widget.moonSet : "—"))
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            Layout.fillWidth: true; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight
        }
        Text { visible: root.showRiseSet; text: "Next season"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showRiseSet
            text: root.widget && root.widget.nextSeason ? root.widget.nextSeason : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text { visible: root.showNow; text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.showNow; text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2; Layout.fillWidth: true; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text {
            visible: text.length > 0
            text: root.widget ? [root.widget.deNote ? "DE: " + root.widget.deNote : "",
                                 root.widget.dxNote ? "DX: " + root.widget.dxNote : ""].filter(Boolean).join("   ") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            Layout.columnSpan: 3; Layout.fillWidth: true; elide: Text.ElideRight
        }
    }
}
