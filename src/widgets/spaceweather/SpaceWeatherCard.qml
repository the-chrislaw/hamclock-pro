import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    readonly property bool hasKp: widget !== null && widget.hasKp
    readonly property color kpColor: hasKp && widget.kp >= 5 ? Theme.bad : hasKp && widget.kp >= 4 ? Theme.warn : Theme.ink
    readonly property color xrayColor: widget && widget.xrayClass[0] === "X" ? Theme.bad
                                     : widget && widget.xrayClass[0] === "M" ? Theme.warn : Theme.ink
    // Tall pane: the three values stacked with their captions, the X-ray plot takes the rest.
    // The stacked values need real height: a barely-tall pane (1024x768) keeps the one-line rows.
    readonly property bool stackTall: root.tall && compactColumn.parent !== null
                                      && compactColumn.parent.height >= 250 * Theme.scale
    ColumnLayout {
        visible: root.stackTall
        width: parent.width
        height: parent.height
        spacing: Theme.gap
        Metric {
            label: "Kp index"
            available: root.hasKp
            value: available ? root.widget.kp.toFixed(1) : ""
            caption: available ? root.widget.kpText : ""
            valueColor: root.kpColor
            Layout.fillWidth: true
        }
        Metric {
            label: "Solar flux"
            available: root.widget && root.widget.hasSolarFlux
            value: available ? root.widget.solarFlux : ""
            caption: "SFI, 10.7 cm"
            Layout.fillWidth: true
        }
        Metric {
            label: "X-ray"
            available: root.widget && root.widget.xrayClass.length > 0
            value: available ? root.widget.xrayClass : ""
            caption: "GOES flare class"
            valueColor: root.xrayColor
            Layout.fillWidth: true
        }
        XrayPlot {
            visible: root.widget && root.widget.hasXrayHistory
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 40 * Theme.scale
            Layout.preferredHeight: 60 * Theme.scale
            values: root.widget ? root.widget.xrayHistory : []
            pixelScale: root.fit
        }
    }
    // Compact slot: one line per value (label left, value right) so nothing is clipped.
    Column {
        id: compactColumn
        visible: root.compact && !root.stackTall
        width: parent.width
        spacing: Theme.gap * 0.4
        Repeater {
            model: [
                { label: "Kp", ok: root.widget && root.widget.hasKp,
                  value: root.widget && root.widget.hasKp ? root.widget.kp.toFixed(1) + " " + root.widget.kpText : "",
                  color: root.widget && root.widget.hasKp && root.widget.kp >= 5 ? Theme.bad
                       : root.widget && root.widget.hasKp && root.widget.kp >= 4 ? Theme.warn : Theme.ink },
                { label: "SFI", ok: root.widget && root.widget.hasSolarFlux,
                  value: root.widget && root.widget.hasSolarFlux ? String(root.widget.solarFlux) : "", color: Theme.ink },
                { label: "X-ray", ok: root.widget && root.widget.xrayClass.length > 0,
                  value: root.widget ? root.widget.xrayClass : "",
                  color: root.widget && root.widget.xrayClass[0] === "X" ? Theme.bad
                       : root.widget && root.widget.xrayClass[0] === "M" ? Theme.warn : Theme.ink }
            ]
            Item {
                required property var modelData
                width: parent.width
                height: Math.round(Theme.fontValue * 0.75) + 4 * Theme.scale
                Text {
                    text: modelData.label
                    color: Theme.secondary
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    font.capitalization: Font.AllUppercase
                    anchors.baseline: valueText.baseline
                }
                Text {
                    id: valueText
                    x: 52 * Theme.scale
                    width: parent.width - x
                    text: modelData.ok ? modelData.value : "—"
                    color: modelData.ok ? modelData.color : Theme.secondary
                    font.pixelSize: Math.round(Theme.fontValue * 0.75)
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }
        XrayPlot {
            visible: root.widget && root.widget.hasXrayHistory
            width: parent.width
            // Desktop panes: the plot takes the height left under the values (the 7-inch layout keeps 34).
            height: Theme.dense ? 34 * Theme.scale : Math.max(34 * Theme.scale, compactColumn.parent.height - y)
            values: root.widget ? root.widget.xrayHistory : []
            pixelScale: root.fit
        }
    }
    Column {
        visible: !root.compact
        width: parent.width
        spacing: Theme.gap
    RowLayout {
        width: parent.width
        spacing: Theme.gap * 1.5
        Metric {
            label: "Kp index"
            available: root.widget && root.widget.hasKp
            value: available ? root.widget.kp.toFixed(1) : ""
            caption: available ? root.widget.kpText : ""
            valueColor: available && root.widget.kp >= 5 ? Theme.bad
                      : available && root.widget.kp >= 4 ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
        Metric {
            label: "Solar flux"
            available: root.widget && root.widget.hasSolarFlux
            value: available ? root.widget.solarFlux : ""
            caption: "SFI, 10.7 cm"
            Layout.fillWidth: true
        }
        Metric {
            label: "X-ray"
            available: root.widget && root.widget.xrayClass.length > 0
            value: available ? root.widget.xrayClass : ""
            caption: "GOES flare class"
            valueColor: available && root.widget.xrayClass[0] === "X" ? Theme.bad
                      : available && root.widget.xrayClass[0] === "M" ? Theme.warn : Theme.ink
            Layout.fillWidth: true
        }
    }
    XrayPlot {
        visible: root.widget && root.widget.hasXrayHistory
        width: parent.width
        height: Math.max(56 * Theme.scale, Math.min(parent.height * 0.3, 160 * Theme.scale))
        values: root.widget ? root.widget.xrayHistory : []
        pixelScale: root.fit
    }
    }
}
