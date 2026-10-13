import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool configured: widget && widget.state !== "unconfigured"
    ColumnLayout {
        width: parent.width
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
            visible: root.configured && widget.spotCount === 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            text: widget && widget.totalCount > 0 ? "No spots match the filter" : "Waiting for spots…"
        }
        ListView {
            visible: root.configured && widget.spotCount > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, Theme.touch * 6)
            clip: true
            model: widget ? widget.spots : null
            delegate: RowLayout {
                width: ListView.view.width
                spacing: Theme.gap
                Text { text: model.time; color: Theme.secondary; font.pixelSize: Theme.fontBody }
                Text { text: model.frequency; color: Theme.ink; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 5 }
                Text { text: model.band; color: Theme.secondary; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 3 }
                Text { text: (model.watched ? "★ " : "") + model.dx; color: model.watched ? Theme.accent : Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody }
                Text { text: model.comment; color: Theme.secondary; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
