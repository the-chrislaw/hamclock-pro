import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool bzSouth: widget && widget.hasBz && widget.bz <= -5
    GridLayout {
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
