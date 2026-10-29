import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Tall pane: the full layout with one line per kind of warning, every line wrapped and laid
    // out narrow so the card grows it until it fills the height. A small slot keeps the counts on (at most) two
    // wrapped lines and the end time only.
    designWidth: (tall ? 180 : compact ? 260 : 420) * Theme.scale
    readonly property bool full: !compact || tall
    readonly property bool active: widget && widget.hasData && widget.count > 0
    readonly property string value: !(widget && widget.hasData) ? "" : widget.count === 0 ? "None active" : widget.headline
    readonly property color valueColor: widget && widget.hasData && widget.topRank >= 4 ? Theme.bad
                                      : widget && widget.hasData && widget.topRank >= 3 ? Theme.warn : Theme.ink
    readonly property string times: {
        if (!widget) return ""
        const t = widget.headlineTimes
        const i = t.indexOf("until ")
        return full || i < 0 ? t : t.slice(i)
    }
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap * (root.full ? 1 : 0.5)
        Metric {
            visible: !root.tall
            label: root.full ? "Marine warnings" : "Marine" + (widget && widget.hasData ? " " + widget.count : "")
            available: widget && widget.hasData
            value: root.value
            valueColor: root.valueColor
            Layout.fillWidth: true
        }
        // Tall: the same value, wrapping, so the narrow stage never clips it.
        ColumnLayout {
            visible: root.tall
            spacing: 2 * Theme.scale
            Layout.fillWidth: true
            Text { text: "Marine warnings"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold
                   font.capitalization: Font.AllUppercase; font.letterSpacing: 0.6; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Text { text: widget && widget.hasData ? root.value : "—"; color: widget && widget.hasData ? root.valueColor : Theme.secondary
                   font.pixelSize: Theme.fontValue; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
        // Counts per kind: wrapped (never cut short); one per line in a tall pane.
        Text {
            visible: text.length > 0
            text: !(widget && widget.hasData) ? "" : widget.count === 0 ? "No US marine warnings or advisories"
                : root.tall ? widget.summary.split(" · ").join("\n") : widget.summary
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap; Layout.fillWidth: true
            Layout.topMargin: -parent.spacing + 2 * Theme.scale
        }
        Text {
            visible: widget && widget.nearDe !== ""
            text: widget ? widget.nearDe : ""
            color: widget && widget.nearDe.indexOf("none") < 0 ? Theme.bad : Theme.secondary
            font.pixelSize: Theme.fontBody; font.bold: true
            elide: Text.ElideRight; maximumLineCount: root.full ? 100 : 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: root.active
            text: widget ? widget.headlineArea : ""
            color: Theme.ink; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: root.full ? 100 : 1
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: root.active
            text: root.times
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }
}
