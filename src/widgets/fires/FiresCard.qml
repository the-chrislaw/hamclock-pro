import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Every line wraps (never cut short); the card grows the text until it fills the pane.
    readonly property bool available: widget !== null && widget.hasData
    readonly property string value: !available ? "" : widget.count === 0 ? "None" : widget.summary
    readonly property string caption: available && widget.count > 0 ? widget.highCount + " high confidence · last 24 h"
                                    : (available ? "No detections in the last 24 h" : "")
    readonly property color valueColor: available && widget.highCount > 0 ? Theme.warn : Theme.ink
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact && !root.tall && !root.roomy ? "Fires" : "Active fires (US)"
            available: root.available
            value: root.value
            caption: root.caption
            valueColor: root.valueColor
            Layout.fillWidth: true
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
