import QtQuick
import QtQuick.Layouts
import HamClock

// DE -> DX HF reliability: bands down, the next 24 UTC hours across. Approximate model.
Card {
    id: root
    // Stacked full layout: a tall pane, or a compact pane with height to spare (Card.roomy: the
    // 7-inch panels' panes, the two-row grids at 1280x800 and 2560x1440).
    readonly property bool stacked: root.tall || root.roomy
    readonly property bool ok: widget && widget.hasMatrix
    readonly property var rows: widget ? widget.rows : []
    // Compact slot that is not tall: best band and one chip per band. Otherwise the heat grid.
    readonly property bool brief: root.compact && !root.stacked
    function shade(v) {
        return v >= 0.6 ? Theme.good : v >= 0.25 ? Theme.warn : Theme.bad
    }
    // Compact: one coloured chip per band for the current hour.
    component NowChips: Flow {
        spacing: Theme.gap
        Repeater {
            model: root.ok ? root.rows : []
            delegate: Text {
                text: modelData.band + " " + Math.round(modelData.now * 100) + "%"
                color: root.shade(modelData.now)
                font.pixelSize: Theme.fontCaption
                font.weight: Font.DemiBold
            }
        }
    }
    Column {
        visible: root.brief
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
            text: root.ok ? root.widget.bestText : ""
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
        NowChips {
            visible: root.ok && root.brief
            width: parent.width
        }
    }
    // Full card or tall pane: the heat grid takes the leftover height, one row per band.
    ColumnLayout {
        visible: !root.brief
        width: parent.width
        height: parent.height
        spacing: Theme.gap * 0.4
        Text {
            visible: !root.ok
            Layout.fillWidth: true
            text: root.widget && !root.widget.hasPath ? "Set a DX target" : "—"
            color: Theme.secondary
            font.pixelSize: Theme.fontValue
            wrapMode: Text.WordWrap
        }
        Text {
            visible: root.ok
            Layout.fillWidth: true
            text: root.ok ? root.widget.bestText : ""
            color: Theme.ink
            font.pixelSize: Theme.fontValue
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
        }
        Text {
            visible: root.ok
            Layout.fillWidth: true
            text: root.ok ? root.widget.pathText : ""
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
        }
        Item {
            id: gridArea
            visible: root.ok
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: root.rows.length * Theme.fontCaption * 1.3
            readonly property real gap: 2 * Theme.scale
            readonly property real labelWidth: 40 * Theme.scale
            readonly property real rowHeight: Math.min(Theme.fontCaption * 2.5,
                                                       root.rows.length ? height / root.rows.length : 0)
            // Too short for a readable row per band (phone strip): the chips for the current hour instead.
            readonly property bool fits: rowHeight >= Theme.fontCaption * 1.1
            Column {
                visible: gridArea.fits
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                Repeater {
                    model: root.ok && !root.brief ? root.rows : []
                    delegate: Row {
                        height: gridArea.rowHeight
                        spacing: gridArea.gap
                        Text { width: gridArea.labelWidth; text: modelData.band; color: Theme.ink
                               anchors.verticalCenter: parent.verticalCenter
                               font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                        Repeater {
                            model: modelData.cells
                            delegate: Rectangle {
                                width: Math.max(2, (gridArea.width - gridArea.labelWidth - 24 * gridArea.gap) / 24)
                                height: gridArea.rowHeight - gridArea.gap
                                anchors.verticalCenter: parent.verticalCenter
                                color: root.shade(modelData)
                                opacity: 0.25 + 0.75 * modelData
                            }
                        }
                    }
                }
            }
            NowChips {
                visible: !gridArea.fits
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Text {
            visible: root.ok
            Layout.fillWidth: true
            text: gridArea.fits ? "UTC from " + (root.widget ? root.widget.startText : "") + " · approximate model"
                                : "Now · approximate model"
            color: Theme.secondary
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
        }
    }
}
