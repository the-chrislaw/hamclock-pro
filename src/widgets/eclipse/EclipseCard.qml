import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Lower rows are dropped (not clipped) when the body is short.
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.25 + Theme.gap
    readonly property int rowsFit: Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    function v(s) { return s ? s : "—" }
    GridLayout {
        id: grid
        width: parent.width
        columns: 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Text { text: "Next"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.v(root.widget ? root.widget.kind : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { text: "Greatest"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.v(root.widget ? root.widget.when : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 3; text: "In"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 3; text: root.v(root.widget ? root.widget.countdown : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 4; text: "Path"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 4; text: root.v(root.widget ? root.widget.detail : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
    }
}
