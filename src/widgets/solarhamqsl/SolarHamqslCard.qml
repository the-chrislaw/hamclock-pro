import QtQuick
import HamClock

Card {
    id: root
    readonly property bool ok: root.widget && root.widget.hasData
    function tone(rank) { return rank >= 3 ? Theme.good : rank === 2 ? Theme.warn : rank === 1 ? Theme.bad : Theme.secondary }
    // Plain Column/Row (no Layout) as in the other compact cards.
    Column {
        width: parent.width
        spacing: Theme.gap * 0.3
        Repeater {
            model: root.ok ? root.widget.bands : []
            Item {
                width: parent.width
                height: Math.round(Theme.fontBody * 1.2)
                Text {
                    text: modelData.band
                    color: Theme.ink
                    font.pixelSize: Theme.fontBody
                    elide: Text.ElideRight
                    width: parent.width * 0.4
                }
                Text {
                    x: parent.width * 0.42
                    width: parent.width * 0.28
                    text: modelData.day
                    color: root.tone(modelData.dayRank)
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    x: parent.width * 0.72
                    width: parent.width * 0.28
                    text: modelData.night
                    color: root.tone(modelData.nightRank)
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    elide: Text.ElideRight
                }
            }
        }
        Text {
            visible: !root.ok
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
        }
        Text {
            visible: root.ok
            width: parent.width
            text: root.compact ? "Day / night" : "Day / night · N0NBH HamQSL.com · " + (root.ok ? root.widget.updated : "")
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
        Repeater {
            model: root.ok && !root.compact ? root.widget.vhf : []
            Text {
                width: parent.width
                text: modelData.name + " " + modelData.location.replace(/_/g, " ") + ": " + modelData.status
                color: modelData.open ? Theme.good : Theme.secondary
                font.pixelSize: Theme.fontCaption
                elide: Text.ElideRight
            }
        }
    }
}
