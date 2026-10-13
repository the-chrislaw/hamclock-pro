import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        Metric {
            label: root.compact ? "Storms" + (widget && widget.hasData ? " " + widget.count : "") : "Tropical storms"
            available: widget && widget.hasData
            value: !available ? "" : widget.count === 0 ? "None active" : widget.headline
            caption: available && widget.count > 0 ? widget.headlineIntensity : (available ? "No active Atlantic or Pacific storms" : "")
            valueColor: available && widget.headlineCategory >= 3 ? Theme.bad
                      : available && widget.headlineCategory >= 1 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Text {
            visible: widget && widget.count > 0
            text: widget ? widget.headlineDetail : ""
            color: Theme.secondary; font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap; Layout.fillWidth: true
        }
        Repeater {
            model: root.compact || !widget ? [] : widget.others
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Text { text: modelData.text; color: Theme.ink; font.bold: true; font.pixelSize: Theme.fontBody
                       elide: Text.ElideRight; Layout.fillWidth: true }
            }
        }
    }
}
