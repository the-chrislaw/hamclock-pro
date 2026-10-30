import QtQuick
import QtQuick.Layouts
import HamClock

// Settings > Satellite alerts: which satellites, how high, how early, quiet hours.
// Every choice applies at once (like Screen blanking); `satAlerts` is the C++ service.
Card {
    id: card
    title: "Satellite alerts"
    readonly property bool apple: satAlerts.systemNotifications
    readonly property string osName: Qt.platform.os === "osx" ? "macOS" : "iOS"
    property bool showFrequencies: false
    property bool showOthers: false

    component Caption: Text {
        color: Theme.secondary
        font.pixelSize: Theme.fontCaption
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    component Heading: Text {
        color: Theme.ink
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
        Layout.fillWidth: true
        Layout.topMargin: Theme.gap * 0.4
    }
    // One row of mutually exclusive presets: [[value, label], ...].
    component Presets: Flow {
        id: presets
        property var options: []
        property var current
        signal picked(var value)
        Layout.fillWidth: true
        spacing: Theme.gap * 0.6
        Repeater {
            model: presets.options
            AppButton {
                required property var modelData
                text: modelData[1]
                primary: presets.current === modelData[0]
                onClicked: presets.picked(modelData[0])
            }
        }
    }

    ColumnLayout {
        width: parent.width
        spacing: Theme.gap * 0.6

        Caption {
            text: card.apple
                ? "Notifies you before passes of the satellites you choose, even when HamClock Pro is closed. Up to 60 alerts are kept scheduled for the next 48 h; opening the app tops them up."
                : "Shows a banner before and during passes of the satellites you choose" +
                  (Qt.platform.os === "linux" ? ", with a chime." : ".") + " Alerts appear while HamClock Pro is running."
        }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            AppButton {
                objectName: "satAlertsToggle"
                text: satAlerts.enabled ? "Pass alerts: on" : "Pass alerts: off"
                primary: satAlerts.enabled
                onClicked: satAlerts.setEnabled(!satAlerts.enabled)
            }
            AppButton {
                visible: satAlerts.enabled
                text: "Send test alert"
                onClicked: satAlerts.sendTestAlert()
            }
        }
        Caption {
            visible: card.apple && satAlerts.permission === "denied"
            color: Theme.bad
            text: "Notifications are turned off for HamClock Pro, so no alerts can appear. Turn them on in the " +
                  (card.osName === "iOS" ? "Settings app > Notifications > HamClock Pro." : "System Settings > Notifications > HamClock Pro.")
        }
        Caption {
            visible: satAlerts.enabled && card.apple && satAlerts.permission === "granted" && satAlerts.scheduledCount >= 0
            text: satAlerts.scheduledCount + " alert" + (satAlerts.scheduledCount === 1 ? "" : "s") + " scheduled with " + card.osName
        }
        Caption { visible: satAlerts.enabled; text: satAlerts.statusText }

        ColumnLayout {
            visible: satAlerts.enabled
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6

            Heading { text: "Satellites" }
            Presets {
                options: [[true, "All workable"], [false, "Only the ones I pick"]]
                current: satAlerts.allWorkable
                onPicked: function (value) { satAlerts.setAllWorkable(value) }
            }
            Caption {
                text: satAlerts.selectedCount + " selected. Tap a satellite to turn its alerts on or off. Status from AMSAT, checked " +
                      satAlerts.catalogUpdated + "; silent birds are marked."
            }
            Repeater {
                model: satAlerts.groups
                ColumnLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.gap * 0.4
                    Caption { text: modelData.mode; font.weight: Font.DemiBold }
                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.gap * 0.6
                        Repeater {
                            model: modelData.sats
                            AppButton {
                                required property var modelData
                                text: modelData.name + (modelData.status === "inactive" ? " (silent)" : "")
                                primary: modelData.selected
                                onClicked: satAlerts.toggleSatellite(modelData.norad)
                            }
                        }
                    }
                    Repeater {
                        model: card.showFrequencies ? modelData.sats : []
                        Caption {
                            required property var modelData
                            text: modelData.name + ": " + modelData.freq + (modelData.also ? " (also " + modelData.also + ")" : "") +
                                  (modelData.note ? ". " + modelData.note : "")
                        }
                    }
                }
            }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.gap * 0.6
                AppButton {
                    text: card.showFrequencies ? "Hide frequencies" : "Show frequencies"
                    onClicked: card.showFrequencies = !card.showFrequencies
                }
                AppButton {
                    visible: satAlerts.others.length > 0
                    text: (card.showOthers ? "Hide" : "Other satellites") + " (" + satAlerts.others.length + ")"
                    onClicked: card.showOthers = !card.showOthers
                }
            }
            Flow {
                visible: card.showOthers
                Layout.fillWidth: true
                spacing: Theme.gap * 0.6
                Repeater {
                    model: card.showOthers ? satAlerts.others : []
                    AppButton {
                        required property var modelData
                        text: modelData.name
                        primary: modelData.selected
                        onClicked: satAlerts.toggleSatellite(modelData.norad)
                    }
                }
            }

            Heading { text: "Highest point of the pass at least" }
            Presets {
                options: [[0, "Any"], [10, "10°"], [15, "15°"], [20, "20°"], [30, "30°"], [45, "45°"]]
                current: satAlerts.minElevation
                onPicked: function (value) { satAlerts.setMinElevation(value) }
            }
            Heading { text: "Alert me" }
            Presets {
                options: [[2, "2 min before"], [5, "5 min"], [10, "10 min"], [15, "15 min"], [0, "At rise"]]
                current: satAlerts.leadMinutes
                onPicked: function (value) { satAlerts.setLeadMinutes(value) }
            }
            Heading { text: "Quiet hours (no alerts for passes rising then)" }
            Presets {
                options: [[-1, "Off"], [22, "22:00-07:00"], [23, "23:00-06:00"], [0, "00:00-06:00"]]
                current: satAlerts.quietStart
                onPicked: function (value) {
                    satAlerts.setQuietHours(value, ({ "-1": -1, "22": 7, "23": 6, "0": 6 })[String(value)])
                }
            }
            Flow {
                Layout.fillWidth: true
                Layout.topMargin: Theme.gap * 0.4
                spacing: Theme.gap * 0.6
                AppButton {
                    text: satAlerts.daylightOnly ? "Daylight passes only: on" : "Daylight passes only: off"
                    primary: satAlerts.daylightOnly
                    onClicked: satAlerts.setDaylightOnly(!satAlerts.daylightOnly)
                }
                AppButton {
                    text: satAlerts.sound ? "Sound: on" : "Sound: off"
                    primary: satAlerts.sound
                    onClicked: satAlerts.setSound(!satAlerts.sound)
                }
                AppButton {
                    visible: !card.apple
                    text: satAlerts.announceStart ? "Banner at rise: on" : "Banner at rise: off"
                    primary: satAlerts.announceStart
                    onClicked: satAlerts.setAnnounceStart(!satAlerts.announceStart)
                }
            }

            Heading { text: "Next alerts"; visible: satAlerts.upcoming.length > 0 }
            Repeater {
                model: satAlerts.upcoming
                Caption {
                    required property var modelData
                    color: Theme.ink
                    text: modelData.when + "  " + modelData.name + "  " + modelData.maxEl + "°  " +
                          modelData.from + " → " + modelData.to + (modelData.mode ? "  " + modelData.mode : "")
                }
            }
        }
    }
}
