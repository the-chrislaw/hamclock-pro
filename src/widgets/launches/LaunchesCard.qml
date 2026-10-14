import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Next launch" : "Next launch (NET)"
            available: widget && widget.hasNext
            value: available ? widget.nextCountdown : ""
            caption: available ? widget.nextName : (widget && widget.hasData ? "No upcoming launches" : "")
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nextDetail !== ""
            text: widget ? widget.nextDetail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: 2; Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.later
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.when; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 7 }
                Text { text: modelData.name; color: Theme.ink; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
