import QtQuick
import QtQuick.Layouts
import HamClock

// Settings > Screen (kiosk, Mac and TV; not iPhone/iPad): blanking, TV edge inset and Exit,
// in one card (it replaces three). Every choice applies at once.
Card {
    id: section
    title: "Screen"
    visible: Qt.platform.os !== "ios"
    property bool exitArmed: false

    ColumnLayout {
        width: parent.width
        spacing: Theme.gap

        ChoiceRow { label: "Turn the screen black when idle"
                    note: "A black overlay for kiosks (the backlight stays on). Tap to wake."
                    choices: [[0, "Off"], [5, "5 min"], [15, "15 min"], [30, "30 min"], [60, "60 min"]]
                    current: appSettings.blankIdleMinutes
                    onChosen: (v) => appSettings.saveBlanking(v, appSettings.blankOffHour, appSettings.blankOnHour) }
        ChoiceRow { label: "Black overnight"
                    note: "Inside the night window the screen goes black again 1 minute after a tap."
                    choices: [["-1,-1", "Off"], ["22,6", "22:00–06:00"], ["23,7", "23:00–07:00"], ["0,6", "00:00–06:00"]]
                    current: appSettings.blankOffHour + "," + appSettings.blankOnHour
                    onChosen: (v) => { const [off, on] = v.split(",").map(Number)
                                       appSettings.saveBlanking(appSettings.blankIdleMinutes, off, on) } }
        ColumnLayout {
            visible: appSettings.blankOffHour >= 0
            spacing: Theme.gap * 0.4
            Layout.fillWidth: true
            Text { text: "Nights it applies"; color: Theme.secondary; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
            Flow {
                Layout.fillWidth: true
                spacing: Theme.gap * 0.6
                Repeater {
                    model: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
                    ToggleChip {
                        label: modelData
                        on: (appSettings.blankDays >> index) & 1
                        onFlipped: appSettings.saveBlankDays(appSettings.blankDays ^ (1 << index))
                    }
                }
            }
        }
        ChoiceRow { label: "TV edge inset"; note: "Space kept free on every edge, for TVs that crop the picture."
                    choices: [[0, "Off"], [2, "2 %"], [4, "4 %"], [6, "6 %"], [8, "8 %"], [10, "10 %"]]
                    current: appSettings.overscanPercent; onChosen: (v) => appSettings.saveOverscan(v) }

        Timer { id: exitDisarm; interval: 5000; onTriggered: section.exitArmed = false }
        Flow {
            Layout.fillWidth: true
            spacing: Theme.gap * 0.6
            AppButton {
                text: section.exitArmed ? "Tap again to exit" : "Exit HamClock Pro"
                primary: section.exitArmed
                onClicked: {
                    if (section.exitArmed) { Qt.quit(); return }
                    section.exitArmed = true
                    exitDisarm.restart()
                }
            }
            Text {
                text: "Closes the app only; it does not reboot or power off the device."
                color: Theme.secondary
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }
    }
}
