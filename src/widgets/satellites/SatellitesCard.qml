import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Next passes at DE: satellite, when, peak elevation (elevation hidden in a compact slot).
    // Roomy slots also get a polar sky plot of the first pass (north up, zenith at the centre).
    readonly property bool showSky: width > 240 * Theme.scale && height > 150 * Theme.scale && widget && widget.skyTrack.length > 1
    readonly property var d: widget ? widget.detail : ({})
    readonly property bool showDetail: d && d.name !== undefined
    Text {
        id: alertLine
        visible: !root.showDetail && root.widget && root.widget.alert !== ""
        text: root.widget ? root.widget.alert : ""
        color: Theme.accent; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
        elide: Text.ElideRight; width: parent.width
    }
    RowLayout {
        visible: !root.showDetail
        y: alertLine.visible ? alertLine.height + Theme.gap * 0.4 : 0
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
                    color: row.soon ? Theme.accent : col === 0 ? Theme.ink : Theme.secondary
                    font.pixelSize: Theme.fontCaption * (col === 0 ? 1.3 : 1)
                    font.weight: col === 0 || row.live ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                    Layout.fillWidth: col === cols - 1
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -Theme.gap
                        onClicked: root.widget.selectRow(Math.floor(index / cols))
                    }
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
    // Detail box for the tapped satellite: now, next pass, transmitters with Doppler. Tap to close.
    ColumnLayout {
        visible: root.showDetail
        width: parent.width
        spacing: Theme.gap * 0.4
        Text {
            text: root.showDetail ? root.d.name + "  " + root.d.az + "° " + root.d.compass + "  el " + root.d.el + "°" +
                  (root.d.above ? "" : " (below)") + (root.d.sunlit === undefined ? "" : root.d.sunlit ? "  sunlit" : "  eclipsed") + "  " + root.d.rangeKm + " km" : ""
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.2; font.weight: Font.DemiBold
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text {
            text: root.showDetail ? "Pass " + root.d.countdown + "  max " + root.d.maxEl + "°  " + root.d.aos + " " +
                  (root.d.aosAz || "") + " > " + root.d.los + " " + (root.d.losAz || "") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text {
            visible: root.showDetail && root.d.more !== undefined && root.d.more.length > 0
            text: root.showDetail && root.d.more ? "Then " + root.d.more.map(m => m.aos + " " + m.maxEl + "°").join("  ") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Repeater {
            model: root.showDetail && !root.compact ? root.d.transmitters : []
            Text {
                required property var modelData
                text: (modelData.down ? "↓ " + modelData.down : "") + (modelData.up ? "  ↑ " + modelData.up : "") + "  " + modelData.mode
                color: Theme.ink; font.pixelSize: Theme.fontCaption
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }
        Text {
            visible: root.showDetail && !root.compact
            text: "Frequency data from SatNOGS DB (CC BY-SA 4.0); Doppler-corrected"
            color: Theme.secondary; font.pixelSize: Theme.fontCaption * 0.8
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Text {
            text: root.showDetail && root.d.tracked ? "Hide track on map" : "Show track on map"
            color: Theme.accent; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            Layout.topMargin: Theme.gap * 0.4
            MouseArea {
                anchors.fill: parent
                anchors.margins: -Theme.gap
                onClicked: root.widget.toggleTracked()
            }
        }
        Text {
            id: dxButton
            property string error: ""
            text: error !== "" ? error : "Set as DX (sub-satellite point)"
            color: error !== "" ? Theme.secondary : Theme.accent; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            elide: Text.ElideRight; Layout.fillWidth: true
            MouseArea {
                anchors.fill: parent
                anchors.margins: -Theme.gap
                onClicked: dxButton.error = root.widget.setAsDx()
            }
        }
        TapHandler { onTapped: root.widget.selectRow(-1) }
    }
    Text {
        visible: !root.showDetail && root.widget && root.widget.hasData && root.widget.rows.length === 0
        text: root.widget.allStale ? "Element sets over 30 days old" : "No passes in the next 24 h"
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption
    }
}
