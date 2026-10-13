import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool bzSouth: widget && widget.hasBz && widget.bz <= -5
    // Compact slot: speed and Bz as one row each (label, value, sparkline).
    Column {
        visible: root.compact
        width: parent.width
        spacing: Theme.gap * 0.4
        Repeater {
            model: [
                { label: "Wind", ok: root.widget && root.widget.hasSpeed,
                  value: root.widget && root.widget.hasSpeed ? Math.round(root.widget.speed) + " km/s" : "",
                  color: root.widget && root.widget.hasSpeed && root.widget.speed >= 800 ? Theme.bad
                       : root.widget && root.widget.hasSpeed && root.widget.speed >= 600 ? Theme.warn : Theme.ink,
                  history: root.widget ? root.widget.speedHistory : [], zero: false },
                { label: "Bz", ok: root.widget && root.widget.hasBz,
                  value: root.widget && root.widget.hasBz ? root.widget.bz.toFixed(1) + " nT" : "",
                  color: root.widget && root.widget.hasBz && root.widget.bz <= -10 ? Theme.bad
                       : root.widget && root.widget.hasBz && root.widget.bz <= -5 ? Theme.warn : Theme.ink,
                  history: root.widget ? root.widget.bzHistory : [], zero: true }
            ]
            Item {
                required property var modelData
                readonly property real textWidth: 84 * Theme.scale
                width: parent.width
                height: 36 * Theme.scale
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
                    values: modelData.history
                    zeroLine: modelData.zero
                }
            }
        }
    }
    GridLayout {
        visible: !root.compact
        width: parent.width
        columns: width < 360 * Theme.scale ? 1 : 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Metric {
                label: "Wind speed"
                available: root.widget && root.widget.hasSpeed
                value: available ? Math.round(root.widget.speed) + " km/s" : ""
                caption: available ? root.widget.speedText + (root.widget.hasDensity
                                     ? " · " + root.widget.density.toFixed(1) + " p/cm³" : "") : ""
                valueColor: available && root.widget.speed >= 800 ? Theme.bad
                          : available && root.widget.speed >= 600 ? Theme.warn : Theme.ink
                Layout.fillWidth: true
            }
            Sparkline {
                values: root.widget ? root.widget.speedHistory : []
                Layout.topMargin: Theme.gap * 0.5
                Layout.fillWidth: true
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            Metric {
                label: "IMF Bz"
                available: root.widget && root.widget.hasBz
                value: available ? root.widget.bz.toFixed(1) + " nT" : ""
                caption: available ? root.widget.bzText + (root.widget.hasBt
                                     ? " · Bt " + root.widget.bt.toFixed(1) + " nT" : "") : ""
                valueColor: available && root.widget.bz <= -10 ? Theme.bad
                          : available && root.widget.bz <= -5 ? Theme.warn : Theme.ink
                Layout.fillWidth: true
            }
            Sparkline {
                values: root.widget ? root.widget.bzHistory : []
                Layout.topMargin: Theme.gap * 0.5
                zeroLine: true
                lineColor: root.bzSouth ? Theme.warn : Theme.accent
                Layout.fillWidth: true
            }
        }
    }
}
