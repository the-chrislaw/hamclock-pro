import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import HamClock

ApplicationWindow {
    id: window
    width: 1024
    height: 600
    minimumWidth: 640
    minimumHeight: 400
    visible: true
    visibility: startFullScreen ? Window.FullScreen : Window.AutomaticVisibility
    title: "HamClock Pro"
    color: Theme.canvas

    SystemPalette { id: systemPalette }
    Binding {
        target: Theme; property: "dark"
        value: forcedTheme === "dark" || (forcedTheme !== "light" &&
               (appSettings.appearance === "dark" ||
                (appSettings.appearance === "system" && systemPalette.window.hslLightness < 0.5)))
    }
    // 1.0 on a 1024x600 panel; grows on TVs so text stays readable from the sofa.
    Binding { target: Theme; property: "scale"; value: Math.max(0.85, Math.min(window.width / 1024, window.height / 600, 3)) * appSettings.textScalePercent / 100 }

    Binding { target: Theme; property: "dense"; value: (window.width <= 1280 && window.height <= 720) || window.phone }

    // Phone portrait (A0d): one column, station strip over one pane over the map.
    readonly property bool phone: width < 600 && height > width
    // Phone landscape (A0d): map left, one pane right of it; 800x480 Pi panels keep the 3-pane grid.
    readonly property bool phoneWide: height < 440 && width > height
    Binding { target: Theme; property: "phone"; value: window.phone }

    readonly property real overscanX: width * appSettings.overscanPercent / 100
    readonly property real overscanY: height * appSettings.overscanPercent / 100

    Shortcut { sequences: ["F11"]; onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { sequences: ["Escape", "Back"]; enabled: bigClockLayer.visible; onActivated: bigClockLayer.visible = false }
    Shortcut { sequences: ["Escape", "Back"]; enabled: timersLayer.visible && !bigClockLayer.visible; onActivated: timersLayer.visible = false }
    Shortcut { sequences: ["Escape", "Back"]; enabled: settingsLayer.visible; onActivated: settingsLayer.visible = false }

    header: Rectangle {
        visible: !Theme.dense
        color: Theme.canvas
        implicitHeight: Theme.dense ? 0 : Theme.touch + Theme.gap
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.pad
            anchors.rightMargin: Theme.pad
            spacing: Theme.gap
            Text {
                text: "HamClock Pro"
                color: Theme.ink
                font.pixelSize: Theme.fontTitle * 1.15
                font.weight: Font.Bold
            }
            Item { Layout.fillWidth: true }
            AppButton {
                text: worldMap.showPath ? "Hide path" : "Show path"
                visible: station.configured
                onClicked: worldMap.showPath = !worldMap.showPath
            }
            AppButton { id: settingsButton; text: "Settings"; onClicked: settingsLayer.visible = true }
        }
    }

    // One page, no scrolling: station column | (pane row over map). Every size
    // from 800x480 to 4K shares this grid; slots clip their content.
    GridLayout {
        id: page
        columns: window.phone ? 1 : 2
        rowSpacing: Theme.gap
        columnSpacing: Theme.gap
        anchors.fill: parent
        anchors.margins: Theme.pad
        anchors.topMargin: (Theme.dense ? Theme.pad : 0) + safeArea.top + window.overscanY
        anchors.leftMargin: Theme.pad + safeArea.left + window.overscanX
        anchors.rightMargin: Theme.pad + safeArea.right + window.overscanX
        anchors.bottomMargin: Theme.pad + safeArea.bottom + window.overscanY

        StationCard {
            id: stationCard
            wide: window.phone
            Layout.preferredWidth: window.phone ? page.width : Math.max(Theme.dense ? 165 : 190, 0) * Theme.scale + (Theme.dense ? 0 : Math.max(0, page.width * 0.22 - 190 * Theme.scale))
            Layout.fillWidth: window.phone
            Layout.fillHeight: !window.phone
            Layout.preferredHeight: window.phone ? page.height * 0.36 : -1
            onSetupRequested: settingsLayer.visible = true
            onSettingsRequested: settingsLayer.visible = true
            onTimersRequested: timersLayer.visible = true
        }

        GridLayout {
            columns: window.phoneWide ? 2 : 1
            layoutDirection: window.phoneWide ? Qt.RightToLeft : Qt.LeftToRight
            rowSpacing: Theme.gap
            columnSpacing: Theme.gap
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                id: paneRow
                Layout.fillWidth: !window.phoneWide
                Layout.fillHeight: window.phoneWide
                Layout.preferredWidth: window.phoneWide ? page.width * 0.3 : -1
                Layout.preferredHeight: window.phoneWide ? -1 : (window.phone ? page.height * 0.22 : Math.max(150 * Theme.scale, page.height * 0.4))
                Layout.maximumHeight: window.phoneWide ? 100000 : Layout.preferredHeight
                Layout.minimumHeight: window.phoneWide ? 0 : Layout.preferredHeight
                spacing: Theme.gap
                function slotAt(i) { return paneRepeater.itemAt(i) }
                Repeater {
                    id: paneRepeater
                    model: (window.phone || window.phoneWide) ? 1 : 3
                    PaneSlot {
                        index: modelData
                        paused: paneChooser.visible || paneOptions.visible
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 1
                        onChooseRequested: function (slotIndex) {
                            paneChooser.slot = slotIndex
                            paneChooser.open()
                        }
                        onOptionsRequested: function (slotIndex) {
                            paneOptions.slot = slotIndex
                            paneOptions.open()
                        }
                    }
                }
            }

            Card {
                id: mapCard
                title: "World map"
                showTitle: !Theme.dense
                fillBody: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                WorldMap {
                    id: worldMap
                    // The renderer fills any size (crops polar rows, keeps +/-70
                    // degrees), so the map takes the whole card body.
                    anchors.fill: parent
                    dark: Theme.dark
                    uiScale: Theme.scale
                    onTapped: function (lat, lon) { mapTapMenu.openAt(lat, lon) }
                    onMarkerTapped: function (layer, call, detail, lat, lon) { mapTapMenu.openMarker(layer, call, detail, lat, lon); mapTapMenu.path = worldMap.pathText(lat, lon) }
                }
                MapTapMenu {
                    id: mapTapMenu
                    parent: mapCard
                    x: Math.max(4 * Theme.scale, (mapCard.width - width) / 2)
                    y: Math.max(4 * Theme.scale, mapCard.height - height - 8 * Theme.scale)
                }
                Rectangle {
                    id: mapMenuButton
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: 4 * Theme.scale
                    width: Math.max(52 * Theme.scale, mapLabel.implicitWidth + 2 * Theme.pad)
                    height: 34 * Theme.scale
                    radius: 4 * Theme.scale
                    color: mapMenu.opened ? Theme.accent : (Theme.dark ? "#cc10181b" : "#ccffffff")
                    activeFocusOnTab: true
                    Keys.onReturnPressed: mapMenuArea.clicked(null)
                    Keys.onSpacePressed: mapMenuArea.clicked(null)
                    border.color: activeFocus ? Theme.focus : "transparent"
                    border.width: 2
                    Text {
                        id: mapLabel
                        anchors.centerIn: parent
                        text: "Map \u25be"
                        color: mapMenu.opened ? Theme.accentInk : (Theme.dark ? "#e6eef0" : "#1b2a30")
                        font.pixelSize: Theme.fontBody
                        font.weight: Font.DemiBold
                    }
                    MouseArea {
                        id: mapMenuArea
                        anchors.fill: parent
                        onClicked: mapMenu.opened ? mapMenu.close() : mapMenu.open()
                    }
                }
                MapMenu {
                    id: mapMenu
                    map: worldMap
                    parent: mapCard
                    x: 4 * Theme.scale
                    y: mapMenuButton.y + mapMenuButton.height + 4 * Theme.scale
                    width: Math.min(implicitWidth, mapCard.width - 8 * Theme.scale)
                    height: Math.min(implicitHeight, mapCard.height - y - 4 * Theme.scale)
                }
                // Pinch / trackpad pinch zoom about the gesture point (flat map; wheel and double tap are in WorldMap).
                PinchHandler {
                    target: null
                    property real lastScale: 1
                    onActiveChanged: lastScale = 1
                    onScaleChanged: if (active) {
                        worldMap.zoomAt(centroid.position.x, centroid.position.y, scale / lastScale)
                        lastScale = scale
                    }
                }
                // Zoom 1-4x and reset (drag pans when zoomed; a tap opens Set DE / Set DX).
                Row {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 4 * Theme.scale
                    spacing: 4 * Theme.scale
                    Repeater {
                        model: [{ t: "+", d: 1 }, { t: "\u2212", d: -1 }, { t: "\u21ba", d: 0 }]
                        Rectangle {
                            width: 28 * Theme.scale; height: width; radius: 4 * Theme.scale
                            color: Theme.dark ? "#cc10181b" : "#ccffffff"
                            Text {
                                anchors.centerIn: parent
                                text: modelData.t
                                color: Theme.dark ? "#e6eef0" : "#1b2a30"
                                font.pixelSize: 16 * Theme.scale
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: modelData.d === 0 ? worldMap.resetView()
                                                             : worldMap.zoom = Math.max(1, Math.min(4, worldMap.zoom + modelData.d))
                            }
                        }
                    }
                }
            }
        }
    }

    PaneChooser { id: paneChooser; parent: window.contentItem }
    PaneOptions {
        id: paneOptions
        parent: window.contentItem
        onChangeRequested: function (slotIndex) { paneChooser.slot = slotIndex; paneChooser.open() }
        onStepRequested: function (slotIndex, direction) { paneRow.slotAt(slotIndex).step(direction) }
    }

    Rectangle {
        id: timersLayer
        anchors.fill: parent
        visible: false
        color: Theme.canvas
        z: 9
        TimersPage { anchors.fill: parent; onCloseRequested: timersLayer.visible = false; onBigClockRequested: bigClockLayer.visible = true }
    }

    Rectangle {
        id: bigClockLayer
        anchors.fill: parent
        visible: false
        color: Theme.canvas
        z: 10
        BigClockPage { anchors.fill: parent; onCloseRequested: bigClockLayer.visible = false }
    }

    // Any touch counts as activity (passive: children still get their taps).
    TapHandler { gesturePolicy: TapHandler.DragThreshold; onPressedChanged: blanking.touch() }

    // Software screen blanking (kiosk): black overlay, any tap wakes it.
    Rectangle {
        id: blankLayer
        visible: blanking.blanked
        anchors.fill: parent
        color: "black"
        z: 100
        focus: visible
        onVisibleChanged: if (visible) forceActiveFocus()
        Keys.onPressed: function (event) { blanking.touch(); event.accepted = true }
        MouseArea { anchors.fill: parent; onPressed: blanking.touch() }
    }

    Rectangle {
        id: alarmBanner
        visible: alarms.ringing
        z: 20
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: Math.min(parent.width, 420 * Theme.scale)
        height: Theme.touch + Theme.pad
        radius: Theme.radius
        color: Theme.accent
        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.pad / 2
            Text { Layout.fillWidth: true; text: alarms.ringingText; color: Theme.canvas; font.pixelSize: Theme.fontValue; font.bold: true; elide: Text.ElideRight }
            AppButton { text: "Dismiss"; onClicked: alarms.dismiss() }
        }
    }

    Rectangle {
        id: settingsLayer
        anchors.fill: parent
        visible: false
        color: Theme.canvas
        z: 10
        onVisibleChanged: if (visible) settingsPage.forceActiveFocus()
        SettingsPage { id: settingsPage; anchors.fill: parent; onCloseRequested: settingsLayer.visible = false }
    }
}
