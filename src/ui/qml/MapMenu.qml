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
    // Width the menu asks for, from its column alone (not the Popup implicit size, which loops).
    readonly property real naturalWidth: menuColumn.implicitWidth + leftPadding + rightPadding
    // Natural width of a chip row (its chips side by side, at least the toggle grid's width). A
    // Flow's own implicit width follows its width, so feeding that back into the popup width
    // loops; this sums the chips instead. Narrow screens (phones) wrap the row.
    function rowWidth(row) {
        let w = 0
        for (let i = 0; i < row.children.length; ++i)
            if (row.children[i].visible && row.children[i].implicitWidth > 0) w += row.children[i].implicitWidth + row.spacing
        return Math.max(layerGrid.implicitWidth, w - row.spacing)
    }
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

    // Scrolls when the map card is shorter than the menu (Pi panel, phone, MacBook
    // window), so every control stays reachable.
    contentItem: Flickable {
        id: menuFlick
        implicitWidth: menuColumn.implicitWidth
        implicitHeight: menuColumn.implicitHeight
        // Content is exactly as wide as the popup: never derived from the column (width loop).
        contentWidth: width
        contentHeight: menuColumn.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollIndicator.vertical: ScrollIndicator { }
    ColumnLayout {
        id: menuColumn
        width: menuFlick.width
        spacing: Theme.gap
        Text { text: "Map view"; color: Theme.ink; font.pixelSize: Theme.fontBody; font.weight: Font.Bold }
        GridLayout {
            id: layerGrid
            columns: 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.gap / 2
            Toggle { label: "Night shade"; on: menu.map ? menu.map.nightShade : false
                     onClicked: menu.map.nightShade = !menu.map.nightShade }
            Toggle { label: "Moon / antipode"; on: menu.map ? menu.map.skyPoints : false
                     onClicked: menu.map.skyPoints = !menu.map.skyPoints }
            Toggle { label: "Country borders"; on: menu.map ? menu.map.borders : false
                     onClicked: menu.map.borders = !menu.map.borders }
            Toggle { label: "US state lines"; on: menu.map ? menu.map.states : false
                     onClicked: menu.map.states = !menu.map.states }
            Toggle { label: "Cities"; on: menu.map ? menu.map.cities : false
                     onClicked: menu.map.cities = !menu.map.cities }
            Toggle { label: "Gray map"; on: menu.map ? menu.map.gray : false
                     onClicked: menu.map.gray = !menu.map.gray }
            Toggle { label: "DE/DX path"; on: menu.map ? menu.map.showPath : false
                     onClicked: menu.map.showPath = !menu.map.showPath }
            Toggle { label: "Long path"; on: menu.map ? menu.map.longPath : false
                     onClicked: menu.map.longPath = !menu.map.longPath }
            Repeater {
                model: menu.map ? menu.map.layerIds : []
                Toggle {
                    label: menu.map.layerName(modelData)
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
            // A Flow's implicit width follows its own width; a fixed preference stops it feeding
            // back into the popup width (binding loop). It still fills and wraps.
            Layout.preferredWidth: menu.rowWidth(this)
            spacing: Theme.gap / 2
            visible: menu.map && menu.map.styleIds.length > 0
            Toggle { label: "Plain"; on: menu.map && menu.map.style === "" && menu.map.rotation === 0
                     onClicked: { menu.map.rotation = 0; menu.map.style = "" } }
            Repeater {
                model: menu.map ? menu.map.styleIds : []
                Toggle { label: menu.map.styleName(modelData); on: menu.map.style === modelData && menu.map.rotation === 0
                         onClicked: { menu.map.rotation = 0; menu.map.style = modelData } }
            }
        }
        Text { text: "Rotate styles"; color: Theme.ink; font.pixelSize: Theme.fontBody
               visible: menu.map && menu.map.styleIds.length > 1 }
        Flow {
            Layout.fillWidth: true
            Layout.preferredWidth: menu.rowWidth(this)
            spacing: Theme.gap / 2
            visible: menu.map && menu.map.styleIds.length > 1
            Repeater {
                model: menu.map ? menu.map.rotationChoices : []
                Toggle { label: modelData === 0 ? "Off" : modelData + " s"; on: menu.map.rotation === modelData
                         onClicked: menu.map.rotation = modelData }
            }
        }
        Text { text: "Grid"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        Flow {
            Layout.fillWidth: true
            Layout.preferredWidth: menu.rowWidth(this)
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "None", g: 0 }, { t: "Tropics", g: 1 }, { t: "Lat/Lng", g: 2 }, { t: "Maidenhead", g: 3 }, { t: "Rings", g: 4 }]
                Toggle { label: modelData.t; on: menu.map && menu.map.grid === modelData.g
                         onClicked: menu.map.grid = modelData.g }
            }
        }
        Text { text: "Projection"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        Flow {
            Layout.fillWidth: true
            Layout.preferredWidth: menu.rowWidth(this)
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "Flat", p: 0 }, { t: "Azimuthal", p: 1 }, { t: "Robinson", p: 2 }, { t: "Azim one", p: 3 }]
                Toggle { label: modelData.t; on: menu.map && menu.map.projection === modelData.p
                         onClicked: menu.map.projection = modelData.p }
            }
        }
        Text { text: "Centre"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        Flow {
            Layout.fillWidth: true
            Layout.preferredWidth: menu.rowWidth(this)
            spacing: Theme.gap / 2
            Repeater {
                model: [{ t: "DE", c: 999 }, { t: "Greenwich", c: 0 }, { t: "Americas", c: -90 }, { t: "Asia", c: 90 }, { t: "Pacific", c: 180 }]
                Toggle { label: modelData.t; on: menu.map && menu.map.centerLon === modelData.c
                         onClicked: menu.map.centerLon = modelData.c }
            }
        }
        Text { text: "Labels"; color: Theme.ink; font.pixelSize: Theme.fontBody }
        Flow {
            Layout.fillWidth: true
            Layout.preferredWidth: menu.rowWidth(this)
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
}
