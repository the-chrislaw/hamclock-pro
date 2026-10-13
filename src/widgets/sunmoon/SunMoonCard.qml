import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    GridLayout {
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
        Text { text: "Phase"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text {
            text: root.widget && root.widget.moonPhase ? root.widget.moonPhase + " " + root.widget.moonIllumination : "—"
            color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2
        }
        Text { text: "Moonrise"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.moonRise ? root.widget.moonRise : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2 }
        Text { text: "Moonset"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.moonSet ? root.widget.moonSet : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2 }
        Text { text: "Now"; color: Theme.secondary; font.pixelSize: Theme.fontCaption }
        Text { text: root.widget && root.widget.moonPosition ? root.widget.moonPosition : "—"; color: Theme.ink; font.pixelSize: Theme.fontCaption * 1.3; Layout.columnSpan: 2 }

        Text {
            visible: text.length > 0
            text: root.widget ? [root.widget.deNote ? "DE: " + root.widget.deNote : "",
                                 root.widget.dxNote ? "DX: " + root.widget.dxNote : ""].filter(Boolean).join("   ") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            Layout.columnSpan: 3
        }
    }
}
