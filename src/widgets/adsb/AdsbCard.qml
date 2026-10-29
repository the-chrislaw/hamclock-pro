import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || (compact && height >= 260 * Theme.scale)
    // Stacked: the two counts stack; the design width is the stack's own, so the card grows it
    // until it fills the pane (never past the compact width).
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
            label: "Aircraft"
            available: root.widget && root.widget.tracked !== ""
            value: available ? root.widget.tracked : ""
            caption: available ? "heard in the last minute" : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
        }
        Metric {
            label: "With position"
            available: root.widget && root.widget.withPosition !== ""
            value: available ? root.widget.withPosition : ""
            caption: available ? "reporting a position" : ""
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
            label: "Aircraft"
            available: root.widget && root.widget.tracked !== ""
            value: available ? root.widget.tracked : ""
        }
        Metric {
            label: "With position"
            visible: root.widget && root.widget.withPosition !== ""
            value: visible ? root.widget.withPosition : ""
        }
    }
}
