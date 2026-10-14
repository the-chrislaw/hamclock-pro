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
    Binding { target: Theme; property: "scale"; value: Math.max(0.85, Math.min(window.width / 1024, window.height / 600, 3)) }

    Binding { target: Theme; property: "dense"; value: window.width <= 1280 && window.height <= 720 }

    Shortcut { sequences: ["F11"]; onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
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
                text: mapCard.showPath ? "Hide path" : "Show path"
                visible: station.configured
                onClicked: mapCard.showPath = !mapCard.showPath
            }
            AppButton { id: settingsButton; text: "Settings"; onClicked: settingsLayer.visible = true }
        }
    }

    // One page, no scrolling: station column | (pane row over map). Every size
    // from 800x480 to 4K shares this grid; slots clip their content.
    RowLayout {
        id: page
        anchors.fill: parent
        anchors.margins: Theme.pad
        anchors.topMargin: Theme.dense ? Theme.pad : 0
        spacing: Theme.gap

        StationCard {
            id: stationCard
            Layout.preferredWidth: Math.max(Theme.dense ? 165 : 190, 0) * Theme.scale + (Theme.dense ? 0 : Math.max(0, page.width * 0.22 - 190 * Theme.scale))
            Layout.fillHeight: true
            onSetupRequested: settingsLayer.visible = true
            onSettingsRequested: settingsLayer.visible = true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.gap

            RowLayout {
                id: paneRow
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(150 * Theme.scale, page.height * 0.4)
                Layout.maximumHeight: Layout.preferredHeight
                Layout.minimumHeight: Layout.preferredHeight
                spacing: Theme.gap
                function slotAt(i) { return paneRepeater.itemAt(i) }
                Repeater {
                    id: paneRepeater
                    model: 3
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
                fillBody: true
                property bool showPath: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                WorldMap {
                    // The renderer fills any size (crops polar rows, keeps +/-70
                    // degrees), so the map takes the whole card body.
                    anchors.fill: parent
                    dark: Theme.dark
                    showPath: mapCard.showPath
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
        id: settingsLayer
        anchors.fill: parent
        visible: false
        color: Theme.canvas
        z: 10
        onVisibleChanged: if (visible) settingsPage.forceActiveFocus()
        SettingsPage { id: settingsPage; anchors.fill: parent; onCloseRequested: settingsLayer.visible = false }
    }
}
