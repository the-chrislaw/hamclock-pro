import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Labels and values; the lower rows are dropped (not clipped) when the body is short.
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.25 + Theme.gap
    readonly property int rowsFit: Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    function v(s) { return s ? s : "—" }
    GridLayout {
        id: grid
        width: parent.width
        columns: 2
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Text { text: "Moon DE"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.v(root.widget ? root.widget.dePosition : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { text: "Moon DX"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.v(root.widget ? root.widget.dxPosition : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { text: "Mutual"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            text: root.v(root.widget ? root.widget.window : "")
            color: root.widget && root.widget.mutualNow ? Theme.ink : Theme.secondary
            font.pixelSize: Theme.fontCaption * 1.3; font.weight: root.widget && root.widget.mutualNow ? Font.DemiBold : Font.Normal
            Layout.fillWidth: true; elide: Text.ElideRight
        }
        Text { visible: root.rowsFit >= 4; text: "Distance"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 4; text: root.v(root.widget ? root.widget.distance : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 5; text: "Path loss"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 5; text: root.v(root.widget ? root.widget.degradation : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 6; text: "Declination"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 6; text: root.v(root.widget ? root.widget.declination : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 7; text: "Sun"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 7; text: root.v(root.widget ? root.widget.sun : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
        Text { visible: root.rowsFit >= 8; text: "Apsis"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.rowsFit >= 8; text: root.v(root.widget ? root.widget.apsis : ""); color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.fillWidth: true; elide: Text.ElideRight }
    }
}
