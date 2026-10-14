import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Marine" + (widget && widget.hasData ? " " + widget.count : "") : "Marine warnings"
            available: widget && widget.hasData
            value: !available ? "" : widget.count === 0 ? "None active" : widget.headline
            caption: available && widget.count > 0 ? widget.summary : (available ? "No US marine warnings or advisories" : "")
            valueColor: available && widget.topRank >= 4 ? Theme.bad
                      : available && widget.topRank >= 3 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearDe !== ""
            text: widget ? widget.nearDe : ""
            color: widget && widget.nearDe.indexOf("none") < 0 ? Theme.bad : Theme.secondary
            font.pixelSize: Theme.fontBody; font.bold: true
            elide: Text.ElideRight; maximumLineCount: 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineArea : ""
            color: Theme.ink; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: root.compact ? 1 : 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineTimes : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; Layout.fillWidth: true
        }
    }
}
