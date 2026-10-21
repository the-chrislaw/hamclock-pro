import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Nearest sonde" : "Nearest radiosonde"
            available: widget && widget.nearest !== ""
            value: available ? widget.nearest : ""
            Layout.fillWidth: true
        }
        Text {
            // Metric hides its caption while unavailable, so say why here.
            visible: text.length > 0
            text: widget && widget.nearest === "" && widget.hasData
                ? (widget.hasStation ? "None heard in the last 30 min" : "Set your station in Settings") : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: 2; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.nearestDetail !== ""
            text: widget ? widget.nearestDetail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; maximumLineCount: 2; Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.hasData && !root.compact
            text: widget ? widget.count + " radiosondes heard worldwide" : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            elide: Text.ElideRight; Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.later
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text { text: modelData.dist; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       Layout.preferredWidth: Theme.fontBody * 7 }
                Text { text: modelData.text; color: Theme.ink; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
