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

    readonly property bool compact: width < 760

    Shortcut { sequences: ["F11"]; onActivated: window.visibility = window.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen }
    Shortcut { sequences: ["Escape", "Back"]; enabled: settingsLayer.visible; onActivated: settingsLayer.visible = false }

    header: Rectangle {
        color: Theme.canvas
        implicitHeight: Theme.touch + Theme.gap
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

    // Wide: station sidebar | (widget strip over map). Compact: one scrolling column.
    Flickable {
        id: scroller
        anchors.fill: parent
        anchors.margins: Theme.pad
        anchors.topMargin: 0
        contentHeight: Math.max(height, content.implicitHeight)
        interactive: contentHeight > height
        flickableDirection: Flickable.VerticalFlick
        maximumFlickVelocity: 6000 * Theme.scale
        flickDeceleration: 2500
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        GridLayout {
            id: content
            width: scroller.width
            height: Math.max(scroller.height, implicitHeight)
            columns: window.compact ? 1 : 2
            columnSpacing: Theme.gap
            rowSpacing: Theme.gap

            StationCard {
                id: stationCard
                Layout.preferredWidth: window.compact ? -1 : Math.max(220 * Theme.scale, scroller.width * 0.24)
                Layout.fillWidth: window.compact
                Layout.fillHeight: !window.compact
                Layout.alignment: Qt.AlignTop
                onSetupRequested: settingsLayer.visible = true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.gap

                // Widget cards registered in the "strip" area, in registry order.
                Flow {
                    id: strip
                    Layout.fillWidth: true
                    spacing: Theme.gap
                    visible: stripRepeater.count > 0
                    Repeater {
                        id: stripRepeater
                        model: widgetRegistry
                        delegate: Loader {
                            active: area === "strip"
                            visible: active
                            source: card
                            // As many columns as fit at ~300 px each, never more than there are cards.
                            readonly property int columns: Math.max(1, Math.min(widgetRegistry.count, 3,
                                                                               Math.floor(strip.width / (300 * Theme.scale))))
                            width: active ? (strip.width - (columns - 1) * Theme.gap) / columns : 0
                            onLoaded: item.widget = widgetModel
                        }
                    }
                }

                Card {
                    id: mapCard
                    title: "World map"
                    property bool showPath: true
                    Layout.fillWidth: true
                    Layout.fillHeight: !window.compact
                    // Always at least a full 2:1 map; the page scrolls when widgets push it down.
                    Layout.preferredHeight: width / 2 + Theme.touch + Theme.pad
                    WorldMap {
                        // Keep the 2:1 map shape and centre it, so it fills the card
                        // instead of drawing its own letterbox margins.
                        readonly property real available: Math.max(120, mapCard.height - Theme.touch - Theme.pad)
                        width: Math.min(parent.width, available * 2)
                        height: width / 2
                        x: (parent.width - width) / 2
                        dark: Theme.dark
                        showPath: mapCard.showPath
                    }
                }

                Repeater {
                    model: widgetRegistry
                    delegate: Loader {
                        active: area === "wide"
                        visible: active
                        source: card
                        Layout.fillWidth: true
                        onLoaded: item.widget = widgetModel
                    }
                }
            }
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
