import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full labels, stacked values and the flux line need a tall pane with real height (not a barely-tall
    // 1024x768 pane); otherwise the compact summary, so the Kp bars always keep their room.
    readonly property bool trendRoomy: trendColumn.parent !== null && trendColumn.parent.height >= 260 * Theme.scale
    readonly property bool brief: root.compact && !(root.tall && trendRoomy)
    ColumnLayout {
        id: trendColumn
        width: parent.width
        height: parent.height  // bars take what the pane leaves instead of being clipped
        spacing: Theme.gap
        // Tall pane: the two values stacked with full labels and captions.
        GridLayout {
            Layout.fillWidth: true
            columns: root.tall && root.trendRoomy ? 1 : 2
            columnSpacing: Theme.gap * 1.5
            rowSpacing: Theme.gap * 0.6
            Metric {
                label: root.brief ? "Sunspots" : "Sunspot number"
                available: root.widget && root.widget.hasSunspots
                value: available ? Math.round(root.widget.sunspots) : ""
                caption: available ? root.brief ? root.widget.sunspotMonth.split(" ")[0] + " mean" : "Monthly mean, " + root.widget.sunspotMonth : ""
                Layout.fillWidth: true
            }
            Metric {
                label: root.brief ? "SFI 30d" : "Solar flux, 30 days"
                available: root.widget && root.widget.hasFlux
                value: available ? root.widget.fluxLatest : ""
                caption: available ? root.widget.fluxCaption : ""
                Layout.fillWidth: true
            }
        }
        // Compact slots keep the Kp bars (they are what a storm watcher needs)
        // and drop the 30-day flux line, which would push them out of the pane.
        TrendLine {
            // A short, wide pane (phone strip) keeps the Kp bars and drops the flux line too.
            visible: !root.brief && (root.tall || trendColumn.parent.height >= 200 * Theme.scale)
            values: root.widget ? root.widget.fluxHistory : []
            pixelScale: root.fit
            Layout.fillWidth: true
            // The Kp bars matter more to a storm watcher: they take the leftover height, the flux line a fixed band.
            Layout.preferredHeight: root.tall ? 44 * Theme.scale : implicitHeight
        }
        Text {
            text: root.brief ? "Kp, 3 h per bar (faded = forecast)" : "Kp, 3 hours per bar (faded = forecast)"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: root.tall ? Text.WordWrap : Text.NoWrap
            Layout.fillWidth: root.tall
            Layout.topMargin: Theme.gap * 0.5
        }
        KpBars {
            bars: root.widget ? root.widget.kpBars : []
            pixelScale: root.fit
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 16 * Theme.scale
            // The tuned 7-inch layout caps the bars; desktop panes let them take the leftover height.
            Layout.maximumHeight: Theme.dense && !root.tall ? 90 * Theme.scale : Number.POSITIVE_INFINITY
        }
    }
}
