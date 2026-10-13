import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool configured: widget && widget.state !== "unconfigured"
    Text {
        visible: root.configured && widget.spotCount === 0
        width: parent.width
        wrapMode: Text.WordWrap
        color: Theme.secondary
        font.pixelSize: Theme.fontBody
        text: "Waiting for spots…"
    }
    ListView {
        visible: root.configured && widget.spotCount > 0
        width: parent.width
        height: Math.min(contentHeight, Theme.touch * 6)
        clip: true
        model: widget ? widget.spots : null
        delegate: RowLayout {
            width: ListView.view.width
            spacing: Theme.gap
            Text { text: model.time; color: Theme.secondary; font.pixelSize: Theme.fontBody }
            Text { text: model.frequency; color: Theme.ink; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 5 }
            Text { text: model.band; color: Theme.secondary; font.pixelSize: Theme.fontBody; Layout.preferredWidth: Theme.fontBody * 3 }
            Text { text: model.dx; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody }
            Text { text: model.comment; color: Theme.secondary; font.pixelSize: Theme.fontBody; elide: Text.ElideRight; Layout.fillWidth: true }
        }
    }
}
