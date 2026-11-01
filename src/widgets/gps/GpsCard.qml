import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    // Stacked layout: a tall pane, or a narrow one with room below (e.g. 1440p pane rows).
    readonly property bool stacked: tall || roomy
    // Stacked: fix, grid and clock stack with their captions; the design width is the stack's own,
    // so the card grows it until it fills the pane (never past the compact width).
    designWidth: stacked ? Math.min(260 * Theme.scale, Math.max(150 * Theme.scale, stack.implicitWidth))
                      : (compact ? 260 : 420) * Theme.scale
    ColumnLayout {
        id: stack
        visible: root.stacked
        width: parent.width
        height: parent.height
        spacing: Theme.gap
        Metric {
            label: "Fix"
            available: root.widget && root.widget.fixText !== ""
            value: available ? root.widget.fixText : ""
            caption: available && root.widget.satellites !== "" ? root.widget.satellites + " satellites" : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
        }
        Metric {
            label: "Grid"
            visible: root.widget && root.widget.grid !== ""
            value: visible ? root.widget.grid : ""
            caption: visible ? root.widget.position : ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
        }
        // The clock check is a sentence: a caption-style label over wrapped body text.
        ColumnLayout {
            visible: root.widget && root.widget.clockCheck !== ""
            spacing: 2 * Theme.scale
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.maximumHeight: implicitHeight * 1.6
            Text {
                text: "Clock"
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.6
            }
            Text {
                text: root.widget ? root.widget.clockCheck : ""
                color: Theme.ink
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Item { Layout.fillHeight: true }
        }
    }
    Flow {
        visible: !root.stacked
        width: parent.width
        spacing: 12 * Theme.scale
        Metric {
            label: "Fix"
            available: root.widget && root.widget.fixText !== ""
            value: available ? root.widget.fixText : ""
            caption: available && !root.compact && root.widget.satellites !== "" ? root.widget.satellites + " satellites" : ""
        }
        Metric {
            label: "Grid"
            visible: root.widget && root.widget.grid !== ""
            value: visible ? root.widget.grid : ""
            caption: visible && !root.compact ? root.widget.position : ""
        }
        Metric {
            label: "Clock"
            visible: !root.compact && root.widget && root.widget.clockCheck !== ""
            value: visible ? root.widget.clockCheck : ""
        }
    }
}
