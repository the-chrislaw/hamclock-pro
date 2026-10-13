import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool ready: widget && widget.configured
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
            caption: root.widget ? [root.widget.location, root.widget.grid].filter(function (s) { return s.length > 0 }).join(" · ") : ""
            Layout.fillWidth: true
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
