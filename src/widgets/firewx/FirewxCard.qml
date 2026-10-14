import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Fire wx" + (widget && widget.hasData ? " " + widget.count : "") : "Fire weather"
            available: widget && widget.hasData
            value: !available ? "" : widget.count === 0 ? "None active" : (root.compact ? widget.shortHeadline : widget.headline)
            caption: !available ? "" : widget.count === 0 ? "No US fire-weather warnings or watches"
                   : widget.count > 1 ? widget.summary : widget.headlineStart
            valueColor: available && widget.topRank >= 4 ? Theme.bad
                      : available && widget.topRank >= 3 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.atDe !== ""
            text: widget ? widget.atDe : ""
            color: widget && widget.atDe.indexOf("none") < 0 ? Theme.bad : Theme.secondary
            font.pixelSize: Theme.fontBody; font.bold: true
            elide: Text.ElideRight; maximumLineCount: 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineArea : ""
            color: Theme.ink; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: 1
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineTimes : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; maximumLineCount: 2
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
    }
}
