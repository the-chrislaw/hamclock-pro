import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Map View menu: which overlay layers, night shading, the DE/DX path and the
// label mode are shown on the map, independent of the panes. Choices are saved
// by the map item in options.ini.
Popup {
    id: menu
    property var map
    padding: Theme.pad
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 8 * Theme.scale
        color: Theme.surfaceRaised
        border.color: Theme.divider
    }

    component Toggle: AppButton {
        id: t
        property bool on: false
        property string label: ""
        implicitHeight: Math.max(36 * Theme.scale, Theme.touch * 0.8)
        Layout.fillWidth: true
        text: (on ? "✓ " : "  ") + label
        primary: on
    }

    contentItem: ColumnLayout {
        spacing: Theme.gap
        Text { text: "Map view"; color: Theme.ink; font.pixelSize: Theme.fontBody; font.weight: Font.Bold }
        GridLayout {
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.gap / 2
            Toggle { label: "Night shade"; on: menu.map ? menu.map.nightShade : false
                     onClicked: menu.map.nightShade = !menu.map.nightShade }
            Toggle { label: "DE/DX path"; on: menu.map ? menu.map.showPath : false
                     onClicked: menu.map.showPath = !menu.map.showPath }
            Repeater {
                model: menu.map ? menu.map.layerIds : []
                Toggle {
                    label: modelData
                    on: menu.map.layerVisible(modelData)
                    // layerVisible is a function, so re-evaluate when the layer list changes.
                    Connections { target: menu.map; function onLayersChanged() { on = menu.map.layerVisible(modelData) } }
                    onClicked: { menu.map.setLayerVisible(modelData, !on); on = menu.map.layerVisible(modelData) }
                }
            }
        }
        Text { text: "Base map"; color: Theme.ink; font.pixelSize: Theme.fontBody
               visible: menu.map && menu.map.styleIds.length > 0 }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.gap / 2
            visible: menu.map && menu.map.styleIds.length > 0
            Toggle { label: "Plain"; on: menu.map && menu.map.style === "" && menu.map.rotation === 0
                     onClicked: { menu.map.rotation = 0; menu.map.style = "" } }
            Repeater {
                model: menu.map ? menu.map.styleIds : []
                Toggle { label: modelData; on: menu.map.style === modelData && menu.map.rotation === 0
                         onClicked: { menu.map.rotation = 0; menu.map.style = modelData } }
            }
        }
        Text { text: "Rotate styles"; color: Theme.ink; font.pixelSize: Theme.fontBody
               visible: menu.map && menu.map.styleIds.length > 1 }
        RowLayout {
            spacing: Theme.gap / 2
            visible: menu.map && menu.map.styleIds.length > 1
            Repeater {
                model: menu.map ? menu.map.rotationChoices : []
                Toggle { label: modelData === 0 ? "Off" : modelData + " s"; on: menu.map.rotation === modelData
                         onClicked: menu.map.rotation = modelData }
            }
        }
        Text { text: "Grid"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        RowLayout {
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "None", g: 0 }, { t: "Tropics", g: 1 }, { t: "Lat/Lng", g: 2 }, { t: "Maidenhead", g: 3 }, { t: "Rings", g: 4 }]
                Toggle { label: modelData.t; on: menu.map && menu.map.grid === modelData.g
                         onClicked: menu.map.grid = modelData.g }
            }
        }
        Text { text: "Centre"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        RowLayout {
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "DE", c: 999 }, { t: "Greenwich", c: 0 }, { t: "Americas", c: -90 }, { t: "Asia", c: 90 }, { t: "Pacific", c: 180 }]
                Toggle { label: modelData.t; on: menu.map && menu.map.centerLon === modelData.c
                         onClicked: menu.map.centerLon = modelData.c }
            }
        }
        Text { text: "Labels"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        RowLayout {
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "Layer", m: -1 }, { t: "None", m: 0 }, { t: "Prefix", m: 1 }, { t: "Call", m: 2 }, { t: "Dot", m: 3 }]
                Toggle {
                    label: modelData.t
                    on: menu.map && menu.map.labelMode === modelData.m
                    onClicked: menu.map.labelMode = modelData.m
                }
            }
        }
        AppButton { text: "Close"; Layout.alignment: Qt.AlignRight; onClicked: menu.close() }
    }
}
