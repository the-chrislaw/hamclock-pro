import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property bool ok: root.widget && root.widget.hasData
    function tone(p) { return p >= 40 ? Theme.bad : p >= 15 ? Theme.warn : Theme.good }
    // Tall pane: both peaks with captions, then a 0-100 % bar per hemisphere filling the rest.
    ColumnLayout {
        visible: root.stacked
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.6
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.gap * 1.5
            Metric {
                label: "North"
                available: root.ok
                value: available ? root.widget.northPeak + "%" : ""
                caption: "peak chance"
                valueColor: available ? root.tone(root.widget.northPeak) : Theme.ink
                Layout.fillWidth: true
                Layout.preferredWidth: 1
            }
            Metric {
                label: "South"
                available: root.ok
                value: available ? root.widget.southPeak + "%" : ""
                caption: "peak chance"
                valueColor: available ? root.tone(root.widget.southPeak) : Theme.ink
                Layout.fillWidth: true
                Layout.preferredWidth: 1
            }
        }
        RowLayout {
            visible: root.ok
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 32 * Theme.scale
            spacing: Theme.gap * 1.5
            Repeater {
                model: root.ok ? [root.widget.northPeak, root.widget.southPeak] : []
                // One bar per hemisphere on a fixed 0-100 % scale; faint marks at 15 % and 40 %.
                Rectangle {
                    required property int modelData
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    radius: Theme.radius * 0.4
                    color: Theme.divider
                    clip: true
                    Rectangle {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
                        height: parent.height * Math.max(0, Math.min(100, parent.modelData)) / 100
                        color: root.tone(parent.modelData)
                        opacity: 0.85
                    }
                    Repeater {
                        model: [15, 40]
                        Rectangle {
                            required property int modelData
                            width: parent.width
                            height: 1
                            y: parent.height * (1 - modelData / 100)
                            color: Theme.secondary
                            opacity: 0.5
                        }
                    }
                }
            }
        }
        Text {
            text: root.ok ? "Peak aurora probability, 0–100 % · map style Aurora · " + root.widget.observed
                          : "Aurora nowcast unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    // Plain Column/Item (no Layout) as in the other compact cards.
    Column {
        visible: !root.stacked
        width: parent.width
        spacing: Theme.gap * 0.6
        Item {
            width: parent.width
            height: Math.round(Theme.fontBody * 1.3)
            Text {
                id: north
                text: root.ok ? "N " + root.widget.northPeak + "%" : "—"
                color: root.ok ? root.tone(root.widget.northPeak) : Theme.secondary
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
            Text {
                anchors { left: north.right; leftMargin: Theme.gap * 2 }
                text: root.ok ? "S " + root.widget.southPeak + "%" : ""
                color: root.ok ? root.tone(root.widget.southPeak) : Theme.secondary
                font.pixelSize: Theme.fontBody
                font.bold: true
            }
        }
        Text {
            width: parent.width
            text: root.ok ? (root.compact ? "Peak aurora · " : "Peak aurora probability · map style Aurora · ") + root.widget.observed : "Aurora nowcast unavailable"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
