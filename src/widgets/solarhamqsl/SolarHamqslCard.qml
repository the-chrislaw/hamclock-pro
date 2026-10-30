import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property bool ok: root.widget && root.widget.hasData
    function tone(rank) { return rank >= 3 ? Theme.good : rank === 2 ? Theme.warn : rank === 1 ? Theme.bad : Theme.secondary }
    // Full card or tall pane: the Day/Night table with a header, rows spread over the height.
    readonly property bool table: !root.compact || root.stacked
    // VHF openings, one line each (two when they wrap), only when the pane has height for them after the
    // table; otherwise one summary line, so the band rows never get squeezed.
    readonly property int vhfCount: root.ok ? root.widget.vhf.length : 0
    readonly property int vhfOpen: root.ok ? root.widget.vhf.filter(v => v.open).length : 0
    readonly property bool showVhf: !tableCol.parent || tableCol.parent.height >=
        Theme.fontCaption * 4 + (root.ok ? root.widget.bands.length * Math.round(Theme.fontBody * 1.3) : 0)
        + vhfCount * Theme.fontCaption * (root.stacked ? 2.8 : 1.4) + Theme.gap * 3
    ColumnLayout {
        id: tableCol
        visible: root.table
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.3
        Text {
            visible: !root.ok
            text: "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
        }
        Item {
            visible: root.ok
            Layout.fillWidth: true
            implicitHeight: Math.round(Theme.fontCaption * 1.4)
            Repeater {
                model: [{ text: "Bands", x: 0 }, { text: "Day", x: 0.42 }, { text: "Night", x: 0.72 }]
                Text {
                    required property var modelData
                    x: parent.width * modelData.x
                    text: modelData.text
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                }
            }
        }
        Repeater {
            model: root.ok && root.table ? root.widget.bands : []
            Item {
                required property var modelData
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: Math.round(Theme.fontBody * 1.3)
                Layout.minimumHeight: Math.round(Theme.fontBody * 1.2)
                Layout.maximumHeight: Theme.fontBody * 3
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.band
                    color: Theme.ink
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                    width: parent.width * 0.4
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: parent.width * 0.42
                    width: parent.width * 0.28
                    text: modelData.day
                    color: root.tone(modelData.dayRank)
                    font.pixelSize: Theme.fontBody
                    font.bold: true
                    elide: Text.ElideRight
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
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
        Repeater {
            model: root.ok && root.table && root.showVhf ? root.widget.vhf : []
            Text {
                required property var modelData
                Layout.fillWidth: true
                text: modelData.name + " " + modelData.location.replace(/_/g, " ") + ": " + modelData.status
                color: modelData.open ? Theme.good : Theme.secondary
                font.pixelSize: Theme.fontCaption
                wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
                elide: Text.ElideRight
            }
        }
        Text {
            visible: root.ok && !root.showVhf && root.vhfCount > 0
            Layout.fillWidth: true
            text: "VHF: " + root.vhfOpen + " of " + root.vhfCount + " open"
            color: root.vhfOpen > 0 ? Theme.good : Theme.secondary
            font.pixelSize: Theme.fontCaption
        }
        Text {
            visible: root.ok
            Layout.fillWidth: true
            Layout.topMargin: Theme.gap * 0.3
            text: "N0NBH HamQSL.com · " + (root.ok ? root.widget.updated : "")
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: root.stacked ? Text.WordWrap : Text.NoWrap
            elide: Text.ElideRight
        }
    }
    // Compact slot: plain Column/Row (no Layout) as in the other compact cards.
    Column {
        visible: !root.table
        width: parent.width
        spacing: Theme.gap * 0.3
        Repeater {
            model: root.ok && !root.table ? root.widget.bands : []
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
            text: "Day / night"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            elide: Text.ElideRight
        }
    }
}
