import QtQuick
import HamClock

// DE -> DX HF reliability: bands down, the next 24 UTC hours across. Approximate model.
Card {
    id: root
    readonly property bool ok: widget && widget.hasMatrix
    readonly property var rows: widget ? widget.rows : []
    function shade(v) {
        return v >= 0.6 ? Theme.good : v >= 0.25 ? Theme.warn : Theme.bad
    }
    Column {
        width: parent.width
        spacing: Theme.gap * 0.4
        Text {
            visible: !root.ok
            width: parent.width
            text: root.widget && !root.widget.hasPath ? "Set a DX target" : "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
            elide: Text.ElideRight
        }
        Text {
            visible: root.ok
            width: parent.width
            text: root.ok ? (root.compact ? root.widget.bestText : root.widget.bestText + " · " + root.widget.pathText) : ""
            color: Theme.ink
            font.pixelSize: root.compact ? Theme.fontBody : Theme.fontValue
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        // Compact: one coloured chip per band for the current hour.
        Flow {
            visible: root.ok && root.compact
            width: parent.width
            spacing: Theme.gap
            Repeater {
                model: root.compact && root.ok ? root.rows : []
                delegate: Text {
                    text: modelData.band + " " + Math.round(modelData.now * 100) + "%"
                    color: root.shade(modelData.now)
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                }
            }
        }
        // Full: heat grid, one cell per band and hour.
        Column {
            visible: root.ok && !root.compact
            width: parent.width
            spacing: 2 * Theme.scale
            Repeater {
                model: root.compact || !root.ok ? [] : root.rows
                delegate: Row {
                    spacing: 2 * Theme.scale
                    Text { width: 40 * Theme.scale; text: modelData.band; color: Theme.ink
                           font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                    Repeater {
                        model: modelData.cells
                        delegate: Rectangle {
                            width: Math.max(2, ((root.width / root.fit) - 40 * Theme.scale - 25 * 2 * Theme.scale - 2 * Theme.gap) / 24)
                            height: Theme.fontCaption * 1.2
                            color: root.shade(modelData)
                            opacity: 0.25 + 0.75 * modelData
                        }
                    }
                }
            }
            Text {
                text: "UTC from " + (root.widget ? root.widget.startText : "") + " · approximate model"
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                width: parent.width
                elide: Text.ElideRight
            }
        }
    }
}
