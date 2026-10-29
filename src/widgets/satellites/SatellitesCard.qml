import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HamClock

Card {
    id: root
    fillBody: tleEditor.visible
    // Next passes at DE: satellite, when, peak elevation (elevation hidden in a compact slot).
    // Roomy slots also get a polar sky plot of the first pass (north up, zenith at the centre).
    readonly property bool showSky: width > 240 * Theme.scale && height > 150 * Theme.scale && widget && widget.skyTrack.length > 1
    readonly property var d: widget ? widget.detail : ({})
    readonly property bool showDetail: d && d.name !== undefined
    readonly property var rows: widget ? widget.rows : []
    // Stage units: the space the content really has (the card scales the stage by `fit`).
    readonly property real stageWidth: passRow.parent ? passRow.parent.width : 0
    readonly property real stageHeight: passRow.parent ? passRow.parent.height : 0
    readonly property real rowH: Math.ceil(Theme.fontCaption * 1.3 * 1.3)
    // Frequencies need room: only in a wide stage.
    readonly property bool showFreq: !compact && stageWidth >= 470 * Theme.scale
    readonly property int cols: compact ? 2 : showFreq ? 4 : 3
    // Regular layouts list at most five passes, fewer when the stage is short (phones).
    readonly property int listed: Math.max(0, Math.min(5, rows.length,
        Math.floor((stageHeight - (alertLine.visible ? alertLine.height + Theme.gap * 0.4 : 0)
                    - (compact ? 0 : rowH + Theme.gap)) / (rowH + (compact ? Theme.gap * 0.4 : Theme.gap)))))

    // Polar sky plot of the first pass (north up, zenith at the centre). The canvas is drawn at
    // device resolution (fit times the stage size) so it stays crisp when the card grows it.
    component SkyPlot: Item {
        id: plot
        readonly property var track: root.widget ? root.widget.skyTrack : []
        Canvas {
            id: canvas
            width: plot.width * root.fit
            height: plot.height * root.fit
            scale: 1 / root.fit
            transformOrigin: Item.TopLeft
            readonly property var track: plot.track
            onTrackChanged: requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                const k = root.fit;
                const r = Math.min(width, height) / 2 - 2 * k, cx = width / 2, cy = height / 2;
                ctx.strokeStyle = Theme.secondary; ctx.lineWidth = k;
                for (const f of [1, 2 / 3, 1 / 3]) { ctx.beginPath(); ctx.arc(cx, cy, r * f, 0, 2 * Math.PI); ctx.stroke(); }
                ctx.beginPath(); ctx.moveTo(cx - r, cy); ctx.lineTo(cx + r, cy); ctx.moveTo(cx, cy - r); ctx.lineTo(cx, cy + r); ctx.stroke();
                if (track.length < 2) return;
                ctx.strokeStyle = Theme.accent; ctx.lineWidth = 2 * k; ctx.beginPath();
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
        id: alertLine
        visible: !root.showDetail && !root.tall && root.widget && root.widget.alert !== ""
        text: root.widget ? root.widget.alert : ""
        color: Theme.accent; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
        elide: Text.ElideRight; width: parent.width
    }
    RowLayout {
        id: passRow
        visible: !root.showDetail && !root.tall
        y: alertLine.visible ? alertLine.height + Theme.gap * 0.4 : 0
        width: parent.width
        spacing: Theme.gap * 1.5
        GridLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true
            columns: root.cols
            columnSpacing: Theme.gap * 1.5
            rowSpacing: root.compact ? Theme.gap * 0.4 : Theme.gap
            Repeater {
                model: root.tall ? 0 : root.listed * root.cols
                Text {
                    required property int index
                    readonly property var row: root.rows[Math.floor(index / root.cols)] || ({})
                    readonly property int col: index % root.cols
                    text: col === 0 ? row.name : col === 1 ? row.when : col === 2 ? "max " + row.maxEl + "°" : row.freq
                    color: row.soon ? Theme.accent : col === 0 ? Theme.ink : Theme.secondary
                    font.pixelSize: Theme.fontCaption * (col === 0 ? 1.3 : 1)
                    font.weight: col === 0 || row.live ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                    Layout.fillWidth: col === root.cols - 1
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -Theme.gap
                        onClicked: root.widget.selectRow(Math.floor(index / root.cols))
                    }
                }
            }
        }
        SkyPlot {
            visible: root.showSky
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Math.min(root.stageHeight * 0.45, 110 * Theme.scale)
            Layout.preferredHeight: Layout.preferredWidth
        }
    }
    // Tall pane: alert, a large sky plot, then as many passes as fit; the schedule line last.
    // The wrapper reports a fixed natural height so the plot and list can take whatever is left.
    Item {
        visible: root.tall && !root.showDetail && !tleEditor.visible
        width: parent.width
        height: parent.height
        implicitHeight: (root.showSky ? 120 * Theme.scale : 0) + 3 * root.rowH + 2 * Theme.fontCaption * 1.4 + 3 * Theme.gap
        ColumnLayout {
            id: tallColumn
            anchors.fill: parent
            spacing: Theme.gap * 0.6
            Text {
                visible: root.widget && root.widget.alert !== ""
                text: root.widget ? root.widget.alert : ""
                color: Theme.accent; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            SkyPlot {
                visible: root.showSky
                Layout.alignment: Qt.AlignHCenter
                // Half the height at most, so at least a few passes stay listed below.
                Layout.preferredWidth: Math.min(tallColumn.width, tallColumn.height * 0.5)
                Layout.preferredHeight: Layout.preferredWidth
            }
            Item {
                id: tallList
                Layout.fillWidth: true
                Layout.fillHeight: true
                readonly property real pitch: root.rowH + Theme.gap * 0.3
                readonly property int fits: Math.max(0, Math.floor((height + Theme.gap * 0.3) / pitch))
                Column {
                    width: parent.width
                    spacing: Theme.gap * 0.3
                    Repeater {
                        model: root.tall ? root.rows.slice(0, tallList.fits) : []
                        RowLayout {
                            required property var modelData
                            required property int index
                            width: parent.width
                            height: root.rowH
                            spacing: Theme.gap
                            readonly property color tint: modelData.soon ? Theme.accent : Theme.secondary
                            Text {
                                text: modelData.name
                                color: modelData.soon ? Theme.accent : Theme.ink
                                font.pixelSize: Theme.fontCaption * 1.3; font.weight: Font.DemiBold
                                Layout.preferredWidth: Theme.fontCaption * 5.2
                            }
                            Text {
                                text: modelData.when
                                color: parent.tint; font.pixelSize: Theme.fontCaption
                                font.weight: modelData.live ? Font.DemiBold : Font.Normal
                                Layout.fillWidth: true
                            }
                            Text { text: modelData.maxEl + "°"; color: parent.tint; font.pixelSize: Theme.fontCaption }
                            TapHandler { onTapped: root.widget.selectRow(index) }
                        }
                    }
                }
            }
            Text {
                visible: root.widget && root.widget.schedule !== ""
                text: root.widget ? root.widget.schedule : ""
                color: Theme.secondary; font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
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
            visible: root.showDetail && root.d.mutual !== undefined && !root.compact
            text: root.showDetail && root.d.mutual ? "DE+DX " + root.d.mutual : ""
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
            visible: root.showDetail && !root.compact && root.d.statusUrl
            text: "Satellite status (SatNOGS DB) ↗"
            color: Theme.accent; font.pixelSize: Theme.fontCaption
            MouseArea {
                anchors.fill: parent
                anchors.margins: -Theme.gap
                onClicked: Qt.openUrlExternally(root.d.statusUrl)
            }
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
    Text {
        visible: !root.showDetail && !root.compact && !root.tall && root.widget && root.widget.schedule !== ""
        y: passRow.y + passRow.height + Theme.gap
        x: tleLink.width + Theme.gap * 2
        width: parent.width - x
        text: root.widget ? root.widget.schedule : ""
        color: Theme.secondary; font.pixelSize: Theme.fontCaption
        elide: Text.ElideRight
    }
    // Own element sets (pasted two/three-line TLEs, saved to user-tles.txt); roomy slots only.
    Text {
        id: tleLink
        visible: !root.showDetail && !root.compact && !root.tall && !tleEditor.visible && root.widget
        y: passRow.y + passRow.height + Theme.gap
        text: "My TLEs…"
        color: Theme.accent; font.pixelSize: Theme.fontCaption
        MouseArea {
            anchors.fill: parent
            anchors.margins: -Theme.gap
            onClicked: { tleText.text = root.widget.userTles(); tleError.text = ""; tleEditor.visible = true }
        }
    }
    ColumnLayout {
        id: tleEditor
        visible: false
        anchors.fill: parent
        spacing: Theme.gap * 0.5
        Text {
            text: "Paste your own element sets (they override the download)"
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            TextArea {
                id: tleText
                color: Theme.ink; font.family: "monospace"; font.pixelSize: Theme.fontCaption
                wrapMode: TextEdit.NoWrap; selectByMouse: true
            }
        }
        Text {
            id: tleError
            visible: text !== ""
            color: Theme.accent; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        RowLayout {
            spacing: Theme.gap
            AppButton {
                text: "Save"; primary: true
                onClicked: { tleError.text = root.widget.saveUserTles(tleText.text); if (tleError.text === "") tleEditor.visible = false }
            }
            AppButton {
                text: "Clear"
                onClicked: { tleError.text = root.widget.saveUserTles(""); if (tleError.text === "") tleEditor.visible = false }
            }
            AppButton { text: "Cancel"; onClicked: tleEditor.visible = false }
        }
    }
}
