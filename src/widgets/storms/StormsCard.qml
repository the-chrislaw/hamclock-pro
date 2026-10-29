import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Tall pane: the full layout (every other storm with its detail), every line wrapped and laid
    // out narrow so the card grows it until it fills the height instead of leaving it empty.
    designWidth: (tall ? 180 : compact ? 260 : 420) * Theme.scale
    readonly property bool full: !compact || tall
    readonly property bool available: widget !== null && widget.hasData
    readonly property string value: !available ? "" : widget.count === 0 ? "None active" : widget.headline
    readonly property string caption: available && widget.count > 0 ? widget.headlineIntensity : ""
    readonly property color valueColor: available && widget.headlineCategory >= 3 ? Theme.bad
                                      : available && widget.headlineCategory >= 1 ? Theme.warn : Theme.ink
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            visible: !root.tall
            label: root.full ? "Tropical storms" : "Storms" + (root.available ? " " + widget.count : "")
            available: root.available
            value: root.value
            valueColor: root.valueColor
            Layout.fillWidth: true
        }
        ColumnLayout {
            visible: root.tall
            spacing: 2 * Theme.scale
            Layout.fillWidth: true
            Text { text: "Tropical storms"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                   font.capitalization: Font.AllUppercase; font.letterSpacing: 0.6; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: root.available ? root.value : "—"; color: root.available ? root.valueColor : Theme.secondary
                   font.pixelSize: Theme.fontValue; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        // Intensity (or why there is none) wraps instead of being cut short.
        Text {
            visible: text.length > 0
            text: root.caption !== "" ? root.caption
                : root.available && widget.count === 0 ? "No active Atlantic or Pacific storms" : ""
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
            Layout.topMargin: -Theme.gap + 2 * Theme.scale
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineDetail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Repeater {
            model: !root.full || !widget ? [] : widget.others
            delegate: ColumnLayout {
                required property var modelData
                Layout.fillWidth: true
                Layout.topMargin: Theme.gap * 0.4
                spacing: 0
                Text { text: modelData.text; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       wrapMode: Text.WordWrap; Layout.fillWidth: true }
                Text { visible: root.tall && text.length > 0; text: modelData.detail || ""; color: Theme.secondary; font.pixelSize: Theme.fontCaption
                       wrapMode: Text.WordWrap; Layout.fillWidth: true }
            }
        }
    }
}
