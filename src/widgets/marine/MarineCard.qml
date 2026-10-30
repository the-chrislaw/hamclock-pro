import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Full layout (wide, tall or roomy pane): one line per kind of warning in a tall pane, every
    // line wrapped. A small slot keeps the counts on (at most) two wrapped lines and the end time only.
    readonly property bool full: !compact || tall || roomy
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
            label: root.full ? "Marine warnings" : "Marine" + (widget && widget.hasData ? " " + widget.count : "")
            available: widget && widget.hasData
            value: root.value
            valueColor: root.valueColor
            Layout.fillWidth: true
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
        // Nothing in force: a full pane says what is being watched, so the quiet state reads as
        // a checked "all clear" rather than an empty card.
        Text {
            visible: root.full && widget !== null && widget.hasData && widget.count === 0
            text: "Watching NWS marine warnings and advisories for US waters (api.weather.gov)"
            color: Theme.secondary; font.pixelSize: Theme.fontCaption
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
