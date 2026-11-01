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
        // A fixed three-row model: the rows read their values through bindings. An array literal
        // rebuilt on every reading destroys and recreates the rows, possibly while they are being
        // placed (the solar wind card crashed Qt's layout engine that way on Linux).
        function row(i) {
            const w = root.widget
            if (i === 0) return { label: "Kp", ok: w && w.hasKp,
                value: w && w.hasKp ? w.kp.toFixed(1) + " " + w.kpText : "",
                color: w && w.hasKp && w.kp >= 5 ? Theme.bad : w && w.hasKp && w.kp >= 4 ? Theme.warn : Theme.ink }
            if (i === 1) return { label: "SFI", ok: w && w.hasSolarFlux,
                value: w && w.hasSolarFlux ? String(w.solarFlux) : "", color: Theme.ink }
            return { label: "X-ray", ok: w && w.xrayClass.length > 0,
                value: w ? w.xrayClass : "",
                color: w && w.xrayClass[0] === "X" ? Theme.bad : w && w.xrayClass[0] === "M" ? Theme.warn : Theme.ink }
        }
        Repeater {
            model: 3
            Item {
                required property int index
                readonly property var modelData: compactColumn.row(index)
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
            // The plot takes the height left under the values.
            height: Math.max(34 * Theme.scale, compactColumn.parent.height - y)
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
