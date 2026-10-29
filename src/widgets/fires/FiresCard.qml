import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Tall pane: the full layout with every line wrapped, laid out narrow so the card grows it
    // (up to its 2.2x) until it fills the height instead of leaving the bottom empty.
    designWidth: (tall ? 180 : compact ? 260 : 420) * Theme.scale
    readonly property bool available: widget !== null && widget.hasData
    readonly property string value: !available ? "" : widget.count === 0 ? "None" : widget.summary
    readonly property string caption: available && widget.count > 0 ? widget.highCount + " high confidence · last 24 h"
                                    : (available ? "No detections in the last 24 h" : "")
    readonly property color valueColor: available && widget.highCount > 0 ? Theme.warn : Theme.ink
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            visible: !root.tall
            label: root.compact ? "Fires" : "Active fires (US)"
            available: root.available
            value: root.value
            caption: root.caption
            valueColor: root.valueColor
            Layout.fillWidth: true
        }
        // Tall: the same value, wrapping, so the narrow stage never clips it.
        ColumnLayout {
            visible: root.tall
            spacing: 2 * Theme.scale
            Layout.fillWidth: true
            Text { text: "Active fires (US)"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                   font.capitalization: Font.AllUppercase; font.letterSpacing: 0.6; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: root.available ? root.value : "—"; color: root.available ? root.valueColor : Theme.secondary
                   font.pixelSize: Theme.fontValue; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { visible: text.length > 0; text: root.caption; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                   wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.detail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearestText !== ""
            text: widget ? widget.nearestText : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: root.tall ? 100 : 2; Layout.fillWidth: true
        }
    }
}
