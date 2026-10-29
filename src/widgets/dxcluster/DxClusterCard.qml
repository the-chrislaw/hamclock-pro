import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool configured: widget && widget.state !== "unconfigured"
    // The spot list takes whatever height the pane has.
    ColumnLayout {
        width: parent.width
        height: parent.height
        spacing: Theme.gap
        RowLayout {
            visible: root.configured && widget.totalCount > 0
            Layout.fillWidth: true
            spacing: Theme.gap
            AppButton { text: "Band: " + (widget.bandFilter || "all"); onClicked: widget.cycleBandFilter() }
            AppButton { text: "Mode: " + (widget.modeFilter || "all"); onClicked: widget.cycleModeFilter() }
            Item { Layout.fillWidth: true }
            Text {
                visible: widget.watchedCount > 0
                text: "★ " + widget.watchedCount
                color: Theme.accent
                font.bold: true
                font.pixelSize: Theme.fontBody
            }
        }
        Text {
            readonly property var busiest: widget && widget.activity.length > 0
                ? widget.activity.reduce(function (a, b) { return b.total > a.total ? b : a }) : null
            visible: root.configured && busiest !== null
            Layout.fillWidth: true
            elide: Text.ElideRight
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            text: busiest ? "Busiest " + busiest.band + " · " + busiest.total + (root.compact ? "" :
                  "  (" + ["NA", "SA", "EU", "AF", "AS", "OC"].filter(function (c) { return busiest[c] > 0 })
                  .map(function (c) { return c + " " + busiest[c] }).join(" ") + ")") : ""
        }
        Text {
            visible: root.configured && widget.spotCount === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            text: widget && widget.totalCount > 0 ? "No spots match the filter" : "Waiting for spots…"
        }
        // Whole rows only: the list takes the leftover height, rounded down to full spot lines.
        Item {
            visible: root.configured && widget.spotCount > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: spotList.contentHeight
            ListView {
                id: spotList
                readonly property real rowHeight: Theme.fontBody * 1.45
                width: parent.width
                height: Math.min(contentHeight, Math.max(1, Math.floor(parent.height / rowHeight)) * rowHeight)
                clip: true
                interactive: contentHeight > height
                model: widget ? widget.spots : null
                delegate: RowLayout {
                    width: ListView.view.width
                    height: spotList.rowHeight
                    spacing: Theme.gap
                    Text { text: model.time; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                    Text { text: model.frequency; color: Theme.ink; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 5 }
                    Text { text: model.band; color: Theme.secondary; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 3 }
                    Text { text: (model.watched ? "★ " : "") + model.dx; color: model.watched ? Theme.accent : Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody }
                    Text {
                        visible: !root.compact && model.needed !== ""
                        text: model.needed; color: Theme.accent; font.pixelSize: Theme.fontBody; font.bold: true
                    }
                    Text {
                        visible: !root.compact && model.ota !== ""
                        text: model.ota + (model.otaName !== "" ? " " + model.otaName : ""); color: Theme.accent; font.pixelSize: Theme.fontBody; font.bold: true
                    }
                    // Compact: one trailing field that shortens (needed > reference > country) so the row never overflows.
                    Text {
                        readonly property string flag: model.needed !== "" ? model.needed : model.ota
                        objectName: "elideOk"  // the narrow pane shortens the trailing note on purpose
                        text: root.compact ? (flag || model.country || model.comment)
                                           : (model.country && model.comment ? model.country + " · " + model.comment : (model.country || model.comment))
                        color: root.compact && flag !== "" ? Theme.accent : Theme.secondary
                        font.bold: root.compact && flag !== ""
                        font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true
                        opacity: width >= Theme.fontBody * 1.5 ? 1 : 0  // no lone "…" when the row is full
                    }
                }
            }
        }
    }
}
