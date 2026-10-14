import QtQuick
import QtQuick.Layouts
import HamClock

Card {
    id: root
    RowLayout {
        width: parent.width
        spacing: Theme.gap * 1.5
        Metric {
            label: "DE weather"
            available: root.widget && root.widget.deTemp.length > 0
            value: available ? root.widget.deTemp : ""
            caption: available ? root.compact ? root.widget.deSummary : root.widget.deSummary + " · " + root.widget.deWind + (root.widget.deLocal.length > 0 ? " · " + root.widget.deLocal : "") + "\n" + root.widget.deAir : ""
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.maximumWidth: root.width / 2
        }
        Metric {
            label: "DX weather"
            available: root.widget && root.widget.dxTemp.length > 0
            value: available ? root.widget.dxTemp : ""
            caption: available ? root.compact ? root.widget.dxSummary : root.widget.dxSummary + " · " + root.widget.dxWind + (root.widget.dxLocal.length > 0 ? " · " + root.widget.dxLocal : "") + "\n" + root.widget.dxAir : ""
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.maximumWidth: root.width / 2
        }
    }
}
