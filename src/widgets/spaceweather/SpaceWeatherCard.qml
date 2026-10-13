import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
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
}
