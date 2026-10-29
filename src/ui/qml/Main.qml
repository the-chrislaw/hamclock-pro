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

    // Kiosk first boot (--first-run): no station yet, so start in Settings.
    Component.onCompleted: if (firstRun && !station.configured) settingsLayer.visible = true

    SystemPalette { id: systemPalette }
    Binding {
        target: Theme; property: "dark"
        value: forcedTheme === "dark" || (forcedTheme !== "light" &&
               (appSettings.appearance === "dark" ||
                (appSettings.appearance === "system" && systemPalette.window.hslLightness < 0.5)))
    }
    // TV-sized screens (over ~36 inches wide, or Apple TV) are read from the sofa.
    readonly property bool tvScreen: Qt.platform.os === "tvos" || Screen.width / Math.max(Screen.pixelDensity, 0.1) > 800
    // 1.0 on a 1024x600 panel and on desktops, tablets and monitors, which use the room
    // for more panes instead; grows on TVs so text stays readable from the sofa.
    Binding { target: Theme; property: "scale"; value: Math.max(0.85, Math.min(window.width / 1024, window.height / 600, window.tvScreen ? 3 : 1)) * appSettings.textScalePercent / 100 }

    Binding { target: Theme; property: "accentStyle"; value: appSettings.accentStyle }

    Binding { target: Theme; property: "dense"; value: (window.width <= 1280 && window.height <= 720) || window.phone }

    // Phone portrait (A0d): one column, compact station strip over two panes over the map.
    readonly property bool phone: width < 600 && height > width
    // Phone landscape (A0d): station, map, then two stacked panes; 800x480 Pi panels keep the 3-pane grid.
    readonly property bool phoneWide: height < 440 && width > height
    Binding { target: Theme; property: "phone"; value: window.phone }

    // Settings, timers and the big clock cover the page. On iOS a tap arrives as a
    // touch and then again as a synthesized mouse click, so the page stays disabled
    // while one is open and for a moment after it closes; otherwise the second copy
    // of the tap that closed it lands on whatever is underneath (e.g. the gear).
    readonly property bool overlayOpen: settingsLayer.visible || timersLayer.visible || bigClockLayer.visible
    property bool pageInputBlocked: false
    onOverlayOpenChanged: if (overlayOpen) { pageInputBlocked = true; unblockTimer.stop() } else unblockTimer.restart()
    Timer { id: unblockTimer; interval: 350; onTriggered: window.pageInputBlocked = false }

    readonly property real overscanX: width * appSettings.overscanPercent / 100
    readonly property real overscanY: height * appSettings.overscanPercent / 100

    Shortcut { sequences: ["F11"]; onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { sequences: ["Escape", "Back"]; enabled: bigClockLayer.visible; onActivated: bigClockLayer.visible = false }
    Shortcut { sequences: ["Escape", "Back"]; enabled: timersLayer.visible && !bigClockLayer.visible; onActivated: timersLayer.visible = false }
    Shortcut { sequences: ["Escape", "Back"]; enabled: settingsLayer.visible; onActivated: settingsLayer.visible = false }

    header: Rectangle {
        visible: !Theme.dense
        enabled: !window.pageInputBlocked
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

    // One page, no scrolling: station column | (pane grid over map). Every size
    // from 800x480 to 4K shares this grid; slots clip their content. The pane grid
    // holds as many panes as fit (at least three; a second row when Map size allows).
    GridLayout {
        id: page
        enabled: !window.pageInputBlocked
        columns: window.phone ? 1 : 2
        rowSpacing: Theme.gap
        columnSpacing: Theme.gap
        anchors.fill: parent
        anchors.margins: Theme.pad
        anchors.topMargin: (Theme.dense ? Theme.pad : 0) + safeArea.top + window.overscanY
        anchors.leftMargin: Theme.pad + safeArea.left + window.overscanX
        anchors.rightMargin: Theme.pad + safeArea.right + window.overscanX
        anchors.bottomMargin: Theme.pad + safeArea.bottom + window.overscanY

        // Station column. When the station card leaves room below it (tall desktop
        // windows), the room becomes one more pane instead of empty space: the next
        // saved slot after the grid's, so it never repeats a widget on screen.
        // Plain Item with explicit geometry (like the pane row): a layout that shows
        // and hides the spill slot while it settles can leave the slot at 0x0 (#638).
        Item {
            id: stationColumn
            Layout.preferredWidth: window.phone ? page.width : Math.max(Theme.dense ? 165 : 190, 0) * Theme.scale + (Theme.dense ? 0 : Math.min(150 * Theme.scale, Math.max(0, page.width * 0.22 - 190 * Theme.scale)))
            Layout.fillWidth: window.phone
            Layout.fillHeight: !window.phone
            Layout.preferredHeight: window.phone ? Math.min(stationCard.implicitHeight, page.height * 0.3) : -1
            implicitHeight: stationCard.implicitHeight
            readonly property int spillIndex: paneRow.grid.columns * paneRow.grid.rows
            readonly property bool spill: !window.phone && !window.phoneWide && spillIndex < appSettings.panes.length
                && width >= paneRow.minPaneWidth && height - stationCard.implicitHeight - Theme.gap >= paneRow.minPaneHeight

            StationCard {
                id: stationCard
                wide: window.phone
                compact: window.phone || window.phoneWide
                width: parent.width
                height: stationColumn.spill ? implicitHeight : parent.height
                onSetupRequested: settingsLayer.visible = true
                onSettingsRequested: settingsLayer.visible = true
                onTimersRequested: timersLayer.visible = true
            }
            PaneSlot {
                id: spillSlot
                visible: stationColumn.spill
                index: stationColumn.spillIndex
                paused: paneChooser.visible || paneOptions.visible
                y: stationCard.height + Theme.gap
                width: parent.width
                height: Math.max(0, parent.height - y)
                onChooseRequested: function (slotIndex) {
                    paneChooser.slot = slotIndex
                    paneChooser.open()
                }
                onSetupRequested: settingsLayer.visible = true
                onOptionsRequested: function (slotIndex) {
                    paneOptions.slot = slotIndex
                    paneOptions.open()
                }
            }
        }

        GridLayout {
            id: paneAndMap
            columns: window.phoneWide ? 2 : 1
            layoutDirection: window.phoneWide ? Qt.RightToLeft : Qt.LeftToRight
            rowSpacing: Theme.gap
            columnSpacing: Theme.gap
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Plain Item with explicit slot geometry: a GridLayout whose column count
            // changes while the layout settles can leave every slot at 0x0 (#638).
            Item {
                id: paneRow
                readonly property real minPaneWidth: (Theme.dense ? 250 : 260) * Theme.scale
                readonly property real minPaneHeight: (Theme.dense ? 190 : 220) * Theme.scale
                // Phones show two stacked swipeable panes; mapSize is read so a Map size change re-fits.
                readonly property var grid: (window.phone || window.phoneWide) ? ({ columns: 1, rows: 2 })
                    : (appSettings.mapSize, appSettings.paneGrid(paneAndMap.width, page.height, Theme.gap, minPaneWidth, minPaneHeight))
                readonly property int cols: Math.max(1, grid.columns)
                readonly property int rowCount: Math.max(1, grid.rows)
                readonly property real slotWidth: Math.max(0, (width - (cols - 1) * Theme.gap) / cols)
                readonly property real slotHeight: Math.max(0, (height - (rowCount - 1) * Theme.gap) / rowCount)
                Layout.fillWidth: !window.phoneWide
                Layout.fillHeight: window.phoneWide
                Layout.preferredWidth: window.phoneWide ? page.width * 0.32 : -1
                Layout.preferredHeight: window.phoneWide ? -1 : (window.phone ? page.height * 0.42
                    : grid.height > 0 ? grid.height : Math.max(150 * Theme.scale, page.height * 0.4))
                Layout.maximumHeight: window.phoneWide ? 100000 : Layout.preferredHeight
                Layout.minimumHeight: window.phoneWide ? 0 : Layout.preferredHeight
                function slotAt(i) { return i === stationColumn.spillIndex ? spillSlot : paneRepeater.itemAt(i) }
                Repeater {
                    id: paneRepeater
                    model: paneRow.grid.columns * paneRow.grid.rows
                    PaneSlot {
                        index: modelData
                        paused: paneChooser.visible || paneOptions.visible
                        x: (index % paneRow.cols) * (paneRow.slotWidth + Theme.gap)
                        y: Math.floor(index / paneRow.cols) * (paneRow.slotHeight + Theme.gap)
                        width: paneRow.slotWidth
                        height: paneRow.slotHeight
                        onChooseRequested: function (slotIndex) {
                            paneChooser.slot = slotIndex
                            paneChooser.open()
                        }
                        onSetupRequested: settingsLayer.visible = true
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
                    // A tap on the button while the menu is open closes it on press
                    // (outside the popup); the click that follows must not re-open it.
                    readonly property int reopenGuardMs: 400
                    property double closedAt: 0
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
                        onClicked: {
                            if (mapMenu.opened) mapMenu.close()
                            else if (Date.now() - mapMenuButton.closedAt >= mapMenuButton.reopenGuardMs) mapMenu.open()
                        }
                    }
                }
                // Same parent as the Map button (the card body), so it opens just below
                // the button even when the card title row is shown (iPad, desktop).
                MapMenu {
                    id: mapMenu
                    map: worldMap
                    x: 4 * Theme.scale
                    y: mapMenuButton.y + mapMenuButton.height + 4 * Theme.scale
                    width: Math.min(implicitWidth, worldMap.width - 8 * Theme.scale)
                    height: Math.min(implicitHeight, worldMap.height - y - 4 * Theme.scale)
                    onClosed: mapMenuButton.closedAt = Date.now()
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
        // Swallow taps and wheel events so they never reach the page underneath.
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: function (wheel) { wheel.accepted = true } }
        TimersPage { anchors.fill: parent; onCloseRequested: timersLayer.visible = false; onBigClockRequested: bigClockLayer.visible = true }
    }

    Rectangle {
        id: bigClockLayer
        anchors.fill: parent
        visible: false
        color: Theme.canvas
        z: 10
        // Swallow taps and wheel events so they never reach the page underneath.
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: function (wheel) { wheel.accepted = true } }
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
        // Swallow taps and wheel events so they never reach the page underneath.
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: function (wheel) { wheel.accepted = true } }
        onVisibleChanged: if (visible) { settingsPage.reload(); settingsPage.forceActiveFocus() }
        SettingsPage { id: settingsPage; anchors.fill: parent; onCloseRequested: settingsLayer.visible = false }
    }
}
