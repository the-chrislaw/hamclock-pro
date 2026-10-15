import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import HamClock

// Shown when the map is tapped: offers Set DE / Set DX for the tapped point.
// Both save through AppSettings::saveStation, keeping callsign and zone.
Popup {
    id: menu
    property double latitude: 0
    property double longitude: 0
    property string error: ""
    property string title: ""   // marker call, empty for a bare map tap
    property string detail: ""
    property string path: ""     // distance and bearing from DE
    property string layerId: ""
    padding: Theme.pad
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle {
        radius: 8 * Theme.scale
        color: Theme.surfaceRaised
        border.color: Theme.divider
    }

    function openMarker(layer, call, info, lat, lon) {
        layerId = layer
        title = call !== "" ? call : layer
        detail = info
        openAt(lat, lon, true)
    }
    function openAt(lat, lon, keep) {
        if (keep !== true) { title = ""; detail = ""; path = ""; layerId = "" }
        latitude = lat
        longitude = lon
        error = ""
        open()
    }
    // Satellite markers: show that satellite's detail box in the Satellites pane (if it is on screen).
    function openSatellite() {
        const m = widgetRegistry.model("satellites")
        if (m && m.selectByName(title)) close()
        else error = "Satellite not in the pass list"
    }
    function setDe() {
        error = appSettings.saveStation(appSettings.callsign, latitude, longitude,
                                        appSettings.dxLatitude, appSettings.dxLongitude, appSettings.deZone)
        if (error === "") close()
    }
    function setDx() {
        error = appSettings.saveStation(appSettings.callsign, appSettings.deLatitude, appSettings.deLongitude,
                                        latitude, longitude, appSettings.deZone)
        if (error === "") close()
    }

    contentItem: ColumnLayout {
        spacing: Theme.gap
        Text {
            visible: menu.title !== ""
            text: menu.title
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.weight: Font.Bold
        }
        Text {
            visible: menu.detail !== ""
            text: menu.detail
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
            wrapMode: Text.Wrap
            Layout.maximumWidth: 260 * Theme.scale
        }
        Text {
            text: Math.abs(menu.latitude).toFixed(2) + (menu.latitude >= 0 ? "°N " : "°S ")
                  + Math.abs(menu.longitude).toFixed(2) + (menu.longitude >= 0 ? "°E" : "°W")
            color: Theme.ink
            font.pixelSize: Theme.fontBody
            font.weight: Font.Bold
        }
        Text {
            visible: menu.title !== "" && menu.path !== ""
            text: "From DE: " + menu.path
            color: Theme.secondary
            font.pixelSize: Theme.fontBody
        }
        RowLayout {
            spacing: Theme.gap
            AppButton { visible: menu.layerId === "satellites" && menu.title !== ""; text: "Details"; onClicked: menu.openSatellite() }
            AppButton { text: "Set DE"; onClicked: menu.setDe() }
            AppButton { text: "Set DX"; primary: true; onClicked: menu.setDx() }
            AppButton { text: "Cancel"; onClicked: menu.close() }
        }
        Text {
            visible: menu.error !== ""
            text: menu.error
            color: Theme.bad
            font.pixelSize: Theme.fontBody
            wrapMode: Text.Wrap
            Layout.maximumWidth: 260 * Theme.scale
        }
    }
}
