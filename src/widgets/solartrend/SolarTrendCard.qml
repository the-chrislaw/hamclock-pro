import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        height: parent.height  // bars take what the pane leaves instead of being clipped
        spacing: Theme.gap
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap * 1.5
            Metric {
                label: root.compact ? "Sunspots" : "Sunspot number"
                available: root.widget && root.widget.hasSunspots
                value: available ? Math.round(root.widget.sunspots) : ""
                caption: available ? root.compact ? root.widget.sunspotMonth.split(" ")[0] + " mean" : "Monthly mean, " + root.widget.sunspotMonth : ""
                Layout.fillWidth: true
            }
            Metric {
                label: root.compact ? "SFI 30d" : "Solar flux, 30 days"
                available: root.widget && root.widget.hasFlux
                value: available ? root.widget.fluxLatest : ""
                caption: available ? root.widget.fluxCaption : ""
                Layout.fillWidth: true
            }
        }
        // Compact slots keep the Kp bars (they are what a storm watcher needs)
        // and drop the 30-day flux line, which would push them out of the pane.
        TrendLine {
            visible: !root.compact
            values: root.widget ? root.widget.fluxHistory : []
            Layout.fillWidth: true
        }
        Text {
            text: root.compact ? "Kp, 3 h per bar (faded = forecast)" : "Kp, 3 hours per bar (faded = forecast)"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            Layout.topMargin: Theme.gap * 0.5
        }
        KpBars {
            bars: root.widget ? root.widget.kpBars : []
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 16 * Theme.scale
            Layout.maximumHeight: 90 * Theme.scale
        }
    }
}
