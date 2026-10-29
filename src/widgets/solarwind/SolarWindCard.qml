import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool bzSouth: widget && widget.hasBz && widget.bz <= -5
    // Desktop panes (not the tuned 7-inch dense layout): the sparklines take the leftover height.
    readonly property bool fillPlots: !Theme.dense
    readonly property bool hasSpeed: widget !== null && widget.hasSpeed
    readonly property bool hasBz: widget !== null && widget.hasBz
    readonly property color speedColor: hasSpeed && widget.speed >= 800 ? Theme.bad
                                      : hasSpeed && widget.speed >= 600 ? Theme.warn : Theme.ink
    readonly property color bzColor: hasBz && widget.bz <= -10 ? Theme.bad
                                   : hasBz && widget.bz <= -5 ? Theme.warn : Theme.ink
    readonly property string speedCaption: hasSpeed ? widget.speedText + (widget.hasDensity
                                           ? " · " + widget.density.toFixed(1) + " p/cm³" : "") : ""
    readonly property string bzCaption: hasBz ? widget.bzText + (widget.hasBt
                                        ? " · Bt " + widget.bt.toFixed(1) + " nT" : "") : ""
    // Tall pane: both values with captions, each sparkline sharing the leftover height.
    ColumnLayout {
        visible: root.tall
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.6
        Metric {
            label: "Wind speed"
            available: root.hasSpeed
            value: available ? Math.round(root.widget.speed) + " km/s" : ""
            caption: root.speedCaption
            valueColor: root.speedColor
            Layout.fillWidth: true
        }
        Sparkline {
            values: root.widget ? root.widget.speedHistory : []
            pixelScale: root.fit
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 24 * Theme.scale
        }
        Metric {
            label: "IMF Bz"
            available: root.hasBz
            value: available ? root.widget.bz.toFixed(1) + " nT" : ""
            caption: root.bzCaption
            valueColor: root.bzColor
            Layout.topMargin: Theme.gap * 0.4
            Layout.fillWidth: true
        }
        Sparkline {
            values: root.widget ? root.widget.bzHistory : []
            pixelScale: root.fit
            zeroLine: true
            lineColor: root.bzSouth ? Theme.warn : Theme.accent
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 24 * Theme.scale
        }
    }
    // Widest compact value ("1888 km/s"): the value column never elides a real reading.
    TextMetrics {
        id: widestValue
        text: "1888 km/s"
        font.pixelSize: Math.round(Theme.fontValue * 0.62)
        font.weight: Font.DemiBold
    }
    // Compact slot: speed and Bz as one row each (label, value, sparkline).
    ColumnLayout {
        visible: root.compact && !root.tall
        width: parent.width
        height: root.fillPlots ? parent.height : implicitHeight
        spacing: Theme.gap * 0.4
        Repeater {
            model: [
                { label: "Wind", ok: root.hasSpeed,
                  value: root.hasSpeed ? Math.round(root.widget.speed) + " km/s" : "",
                  color: root.speedColor, history: root.widget ? root.widget.speedHistory : [], zero: false },
                { label: "Bz", ok: root.hasBz,
                  value: root.hasBz ? root.widget.bz.toFixed(1) + " nT" : "",
                  color: root.bzColor, history: root.widget ? root.widget.bzHistory : [], zero: true }
            ]
            Item {
                required property var modelData
                readonly property real textWidth: Math.max(84 * Theme.scale, Math.ceil(widestValue.advanceWidth) + 2)
                Layout.fillWidth: true
                Layout.preferredHeight: 36 * Theme.scale
                Layout.fillHeight: root.fillPlots
                Text {
                    id: rowLabel
                    text: modelData.label
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                }
                Text {
                    anchors.top: rowLabel.bottom
                    width: parent.textWidth
                    text: modelData.ok ? modelData.value : "—"
                    color: modelData.ok ? modelData.color : Theme.secondary
                    font.pixelSize: Math.round(Theme.fontValue * 0.62)
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Sparkline {
                    x: parent.textWidth + Theme.gap * 0.6
                    width: parent.width - x
                    height: parent.height
                    pixelScale: root.fit
                    values: modelData.history
                    zeroLine: modelData.zero
                }
            }
        }
    }
    // Full layout: side by side when wide, stacked otherwise; sparklines fill the height.
    GridLayout {
        visible: !root.compact
        width: parent.width
        height: parent.height
        columns: width < 360 * Theme.scale ? 1 : 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Metric {
                label: "Wind speed"
                available: root.hasSpeed
                value: available ? Math.round(root.widget.speed) + " km/s" : ""
                caption: root.speedCaption
                valueColor: root.speedColor
                Layout.fillWidth: true
            }
            Sparkline {
                values: root.widget ? root.widget.speedHistory : []
                pixelScale: root.fit
                Layout.topMargin: Theme.gap * 0.5
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Metric {
                label: "IMF Bz"
                available: root.hasBz
                value: available ? root.widget.bz.toFixed(1) + " nT" : ""
                caption: root.bzCaption
                valueColor: root.bzColor
                Layout.fillWidth: true
            }
            Sparkline {
                values: root.widget ? root.widget.bzHistory : []
                pixelScale: root.fit
                Layout.topMargin: Theme.gap * 0.5
                zeroLine: true
                lineColor: root.bzSouth ? Theme.warn : Theme.accent
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
    }
}
