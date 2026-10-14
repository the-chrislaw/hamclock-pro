import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Rows that fit in the body: header + 3 sun rows + Moon title are fixed;
    // the later Moon rows are dropped (Now first, then rise/set, then phase)
    // instead of being clipped. Estimate is non-circular (no layout feedback).
    readonly property real rowH: Theme.fontCaption * 1.3 * 1.25 + Theme.gap
    readonly property int rowsFit: Math.floor((grid.parent ? grid.parent.height : 0) / rowH)
    readonly property bool showPhase: rowsFit >= 6
    readonly property bool showRiseSet: rowsFit >= 7
    readonly property bool showNow: rowsFit >= 8
    GridLayout {
        id: grid
        width: parent.width
        columns: 3
        columnSpacing: Theme.gap * 1.5
        rowSpacing: Theme.gap
        Text { text: ""; Layout.fillWidth: true }
        Text { text: "DE"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
        Text { text: "DX"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }

        Text { text: "Sunrise"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deRise ? root.widget.deRise : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxRise ? root.widget.dxRise : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text { text: "Sunset"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deSet ? root.widget.deSet : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxSet ? root.widget.dxSet : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text { text: "Day length"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.deDay ? root.widget.deDay : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }
        Text { text: root.widget && root.widget.dxDay ? root.widget.dxDay : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3 }

        Text {
            text: "Moon (DE)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
            Layout.columnSpan: 3; Layout.topMargin: Theme.gap
        }
        Text { visible: root.showPhase; text: "Phase"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showPhase
            text: root.widget && root.widget.moonPhase ? root.widget.moonPhase + " " + root.widget.moonIllumination : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
        }
        Text { visible: root.showRiseSet; text: "Moon rise/set"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            visible: root.showRiseSet
            text: (root.widget && root.widget.moonRise ? root.widget.moonRise : "—") + " / " + (root.widget && root.widget.moonSet ? root.widget.moonSet : "—")
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
        }
        Text { visible: root.showNow; text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { visible: root.showNow; text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2 }

        Text {
            visible: text.length > 0
            text: root.widget ? [root.widget.deNote ? "DE: " + root.widget.deNote : "",
                                 root.widget.dxNote ? "DX: " + root.widget.dxNote : ""].filter(Boolean).join("   ") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            Layout.columnSpan: 3
        }
    }
}
