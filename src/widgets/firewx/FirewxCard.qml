import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Tall pane: the full layout (whole headline, area and times) with every line wrapped, laid
    // out narrow so the card grows it until it fills the height instead of leaving it empty.
    designWidth: (tall ? 180 : compact ? 260 : 420) * Theme.scale
    readonly property bool full: !compact || tall
    readonly property bool available: widget !== null && widget.hasData
    readonly property string value: !available ? "" : widget.count === 0 ? "None active" : (!full ? widget.shortHeadline : widget.headline)
    readonly property color valueColor: available && widget.topRank >= 4 ? Theme.bad
                                      : available && widget.topRank >= 3 ? Theme.warn : Theme.ink
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            visible: !root.tall
            label: root.full ? "Fire weather" : "Fire wx" + (root.available ? " " + widget.count : "")
            available: root.available
            value: root.value
            valueColor: root.valueColor
            Layout.fillWidth: true
        }
        ColumnLayout {
            visible: root.tall
            spacing: 2 * Theme.scale
            Layout.fillWidth: true
            Text { text: "Fire weather"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                   font.capitalization: Font.AllUppercase; font.letterSpacing: 0.6; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: root.available ? root.value : "—"; color: root.available ? root.valueColor : Theme.secondary
                   font.pixelSize: Theme.fontValue; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        // The caption wraps (counts per kind can be long) instead of being cut short.
        Text {
            visible: text.length > 0
            text: !root.available ? "" : widget.count === 0 ? "No US fire-weather warnings or watches"
                : widget.count > 1 ? widget.summary : widget.headlineStart
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; maximumLineCount: root.full ? 100 : 2; Layout.fillWidth: true
            Layout.topMargin: -Theme.gap + 2 * Theme.scale
        }
        Text {
            visible: widget && widget.atDe !== ""
            text: widget ? widget.atDe : ""
            color: widget && widget.atDe.indexOf("none") < 0 ? Theme.bad : Theme.secondary
            font.pixelSize: Theme.fontBody; font.bold: true
            elide: Text.ElideRight; maximumLineCount: root.full ? 100 : 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineArea : ""
            color: Theme.ink; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: root.full ? 100 : 1
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineTimes : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: root.full ? 100 : 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }
}
