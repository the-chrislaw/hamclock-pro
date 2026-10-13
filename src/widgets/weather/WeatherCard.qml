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
            caption: available ? root.widget.deSummary + " · " + root.widget.deWind : ""
            Layout.fillWidth: true
        }
        Metric {
            label: "DX weather"
            available: root.widget && root.widget.dxTemp.length > 0
            value: available ? root.widget.dxTemp : ""
            caption: available ? root.widget.dxSummary + " · " + root.widget.dxWind : ""
            Layout.fillWidth: true
        }
    }
}
