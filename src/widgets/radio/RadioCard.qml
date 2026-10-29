import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || (compact && height >= 260 * Theme.scale)
    // Stacked: the values stack with their captions; the design width is the stack's own, so the
    // card grows it until it fills the pane (never past the compact width).
    designWidth: stacked ? Math.min(260 * Theme.scale, Math.max(150 * Theme.scale, stack.implicitWidth))
                      : (compact ? 260 : 420) * Theme.scale
    onDesignWidthChanged: Qt.callLater(refit)
    ColumnLayout {
        id: stack
        visible: root.stacked
        width: parent.width
        height: parent.height
        spacing: Theme.gap
        Metric {
            label: "Frequency"
            available: root.widget && root.widget.frequency !== ""
            value: available ? root.widget.frequency : ""
            caption: available ? root.widget.band : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
        }
        Metric {
            label: "Mode"
            available: root.widget && root.widget.mode !== ""
            value: available ? root.widget.mode : ""
            caption: available ? root.widget.passband : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
        }
    }
    Flow {
        visible: !root.stacked
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Frequency"
            available: root.widget && root.widget.frequency !== ""
            value: available ? root.widget.frequency : ""
            caption: available && !root.compact ? root.widget.band : ""
        }
        Metric {
            label: "Mode"
            visible: root.widget && root.widget.mode !== ""
            value: visible ? root.widget.mode : ""
            caption: visible && !root.compact ? root.widget.passband : ""
        }
    }
}
