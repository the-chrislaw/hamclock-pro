import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Short value: drop the " UTC" suffix (the DE/DX headers say UTC) and shorten durations.
    function brief(t) { return t.replace(/ UTC/g, "").replace(/ h /, "h ").replace(/ min/, "m") }
    // Rows that fit in the body: header + 3 sun rows + Moon title are fixed;
    // the later Moon rows are dropped (Now first, then rise/set, then phase)
    // instead of being clipped. Estimate is non-circular (no layout feedback).
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.4 + Theme.gap
    readonly property int rowsFit: Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    readonly property bool showPhase: rowsFit >= 6
    readonly property bool showRiseSet: rowsFit >= 7
    readonly property bool showNow: rowsFit >= 8
    readonly property string notes: widget ? [widget.deNote ? "DE: " + widget.deNote : "",
                                              widget.dxNote ? "DX: " + widget.dxNote : ""].filter(Boolean).join("   ") : ""

    // Moon disc drawn from the elongation: lit limb on the right when waxing, then rotated by
    // the bright-limb direction at DE so it is tilted as in the sky (this also flips it for a
    // southern observer). Drawn at device resolution so it stays crisp when the card grows it.
    component MoonDisc: Item {
        id: discBox
        Canvas {
            width: discBox.width * root.fit
            height: discBox.height * root.fit
            scale: 1 / root.fit
            transformOrigin: Item.TopLeft
            readonly property real elong: root.widget ? root.widget.moonElongation : 0
            readonly property real tilt: root.widget ? root.widget.moonTilt : 0
            readonly property color ink: Theme.ink
            onElongChanged: requestPaint()
            onTiltChanged: requestPaint()
            onInkChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                ctx.translate(width / 2, height / 2);
                // counter-clockwise tilt away from the default (lit right when waxing, left when waning)
                ctx.rotate(-(tilt - (elong < 180 ? -90 : 90)) * Math.PI / 180);
                ctx.translate(-width / 2, -height / 2);
                const r = Math.min(width, height) / 2 - root.fit, cx = width / 2, cy = height / 2;
                const k = Math.cos(elong * Math.PI / 180);   // 1 new .. -1 full
                const litRight = elong < 180;
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                ctx.fillStyle = Qt.rgba(ink.r, ink.g, ink.b, 0.15); ctx.fill();
                // lit region: half disc on the lit side, bounded by the terminator ellipse
                ctx.beginPath();
                const dir = litRight ? 1 : -1;
                ctx.moveTo(cx, cy - r);
                ctx.arc(cx, cy, r, -Math.PI / 2, Math.PI / 2, !litRight);
                const steps = 60;
                for (let i = 0; i <= steps; ++i) {
                    const a = Math.PI / 2 - Math.PI * i / steps;      // bottom to top
                    // terminator bulges toward the lit side for a crescent (k > 0)
                    ctx.lineTo(cx + dir * k * r * Math.cos(a), cy + r * Math.sin(a));
                }
                ctx.closePath();
                ctx.fillStyle = "#e8d9a0"; ctx.fill();
                ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                ctx.strokeStyle = Qt.rgba(ink.r, ink.g, ink.b, 0.5); ctx.lineWidth = root.fit; ctx.stroke();
            }
        }
    }

    GridLayout {
        id: grid
        visible: !root.tall
        width: parent.width
        columns: 3
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Text { text: ""; Layout.fillWidth: true; Layout.preferredWidth: 0 }
        Text { text: "DE (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
        Text { text: "DX (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }

        Text { text: "Sunrise"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deRise ? root.brief(root.widget.deRise) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxRise ? root.brief(root.widget.dxRise) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text { text: "Sunset"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deSet ? root.brief(root.widget.deSet) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxSet ? root.brief(root.widget.dxSet) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text { text: "Day length"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deDay ? root.brief(root.widget.deDay) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }
        Text { text: root.widget && root.widget.dxDay ? root.brief(root.widget.dxDay) : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; Layout.preferredWidth: 0; Layout.minimumWidth: 0; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text {
            // Without room for the phase row, the phase rides on the Moon title line.
            text: "Moon (DE)" + (!root.showPhase && root.widget && root.widget.moonPhase ? " · " + root.widget.moonPhase + " " + root.widget.moonIllumination : "")
            color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.8
            elide: Text.ElideRight; Layout.fillWidth: true; Layout.preferredWidth: 0
            Layout.columnSpan: root.showPhase ? 2 : 3; Layout.topMargin: Theme.gap
        }
        MoonDisc {
            visible: root.showPhase && !!root.widget && !!root.widget.moonPhase
            Layout.rowSpan: 2; Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.preferredWidth: Math.min(rowH * 1.6, 72); Layout.preferredHeight: Layout.preferredWidth
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
            text: brief((root.widget && root.widget.moonRise ? root.widget.moonRise : "—") + " / " + (root.widget && root.widget.moonSet ? root.widget.moonSet : "—"))
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            Layout.fillWidth: true; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight
        }
        Text { visible: root.showRiseSet; text: "Next season"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showRiseSet
            text: root.widget && root.widget.nextSeason ? root.brief(root.widget.nextSeason) : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
            fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text { visible: root.showNow; text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.showNow; text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2; Layout.fillWidth: true; fontSizeMode: Text.HorizontalFit; minimumPixelSize: Theme.fontCaption * 0.7; elide: Text.ElideRight }

        Text {
            visible: text.length > 0
            text: root.notes
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            Layout.columnSpan: 3; Layout.fillWidth: true; elide: Text.ElideRight
        }
    }

    // Tall pane: the sun table on top, then the Moon: a large disc taking the leftover height,
    // its phase, rise/set, position and the next season below. The wrapper reports a fixed
    // natural height so the disc can grow into whatever the pane has.
    Item {
        visible: root.tall
        width: parent.width
        height: parent.height
        implicitHeight: tallColumn.implicitHeight
        ColumnLayout {
            id: tallColumn
            anchors.fill: parent
            spacing: Theme.gap * 0.6
            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: Theme.gap
                rowSpacing: Theme.gap * 0.5
                Text { text: ""; Layout.fillWidth: true; Layout.preferredWidth: 0 }
                Text { text: "DE (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                Text { text: "DX (UTC)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                Repeater {
                    // label, DE value, DX value per row
                    model: ["Sunrise", "deRise", "dxRise", "Sunset", "deSet", "dxSet", "Day length", "deDay", "dxDay"]
                    Text {
                        required property var modelData
                        required property int index
                        readonly property bool isLabel: index % 3 === 0
                        text: isLabel ? modelData : root.widget && root.widget[modelData] ? root.brief(root.widget[modelData]) : "—"
                        color: isLabel ? Theme.secondary : Theme.ink
                        font.pixelSize: isLabel ? Theme.fontCaption : Theme.fontCaption * 1.3
                        Layout.fillWidth: isLabel
                    }
                }
            }
            Text {
                text: "Moon (DE)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                Layout.topMargin: Theme.gap * 0.4
            }
            Item {
                id: discArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 56 * Theme.scale
                MoonDisc {
                    visible: !!root.widget && !!root.widget.moonPhase
                    anchors.centerIn: parent
                    // At most two thirds of the width: a very tall pane does not turn it into a poster.
                    width: Math.min(parent.width * 0.66, parent.height)
                    height: width
                }
            }
            Text {
                text: root.widget && root.widget.moonPhase ? root.widget.moonPhase + " " + root.widget.moonIllumination : "—"
                color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: Theme.gap
                rowSpacing: Theme.gap * 0.5
                Text { text: "Rise/set"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text {
                    text: root.brief((root.widget && root.widget.moonRise ? root.widget.moonRise : "—") + " / " + (root.widget && root.widget.moonSet ? root.widget.moonSet : "—"))
                    color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Text { text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text {
                    text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"
                    color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Text { text: "Season"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
                Text {
                    text: root.widget && root.widget.nextSeason ? root.widget.nextSeason : "—"
                    color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3
                    wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
            }
            Text {
                visible: text.length > 0
                text: root.notes
                color: Theme.secondary; font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
        }
    }
}
