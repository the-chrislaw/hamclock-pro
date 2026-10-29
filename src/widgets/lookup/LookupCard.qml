import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool ready: widget && widget.configured
    readonly property var details: widget ? [widget.location, widget.grid, widget.path].filter(function (s) { return s.length > 0 }) : []
    ColumnLayout {
        width: parent.width
        spacing: Theme.gap
        RowLayout {
            visible: root.ready
            Layout.fillWidth: true
            spacing: Theme.gap
            TextField {
                id: input
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.touch
                placeholderText: "Callsign"
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText
                font.pixelSize: Theme.fontBody
                onAccepted: root.widget.lookup(text)
            }
            AppButton { text: "Look up"; enabled: !root.widget.busy; onClicked: root.widget.lookup(input.text) }
        }
        Metric {
            visible: root.ready && root.widget.call.length > 0
            label: root.widget ? root.widget.call : ""
            available: visible
            value: root.widget ? root.widget.name : ""
            // Tall pane: the details get a line each below instead of one shortened caption.
            caption: root.tall ? "" : root.details.join(" · ")
            Layout.fillWidth: true
        }
        Text {
            visible: root.tall && root.ready && root.widget.call.length > 0 && root.details.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            lineHeight: 1.3
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            text: root.details.join("\n")
        }
        Text {
            visible: root.ready && root.widget.message.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            text: root.widget ? root.widget.message : ""
        }
    }
}
