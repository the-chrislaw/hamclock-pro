import QtQuick
import QtQuick.Layouts
import HamClock

// Settings > Display: how the clock looks, in one card (it replaces eleven small cards).
// Every choice applies at once; the banner title saves when editing finishes.
Card {
    id: section
    title: "Display"
    property alias bannerTitle: bannerTitle

    ColumnLayout {
        width: parent.width
        spacing: Theme.gap

        ChoiceRow { label: "Appearance"; choices: [["system", "System"], ["light", "Light"], ["dark", "Dark"]]
                    current: appSettings.appearance; onChosen: (v) => appSettings.appearance = v }
        ChoiceRow { label: "Distances"; choices: [[false, "Kilometres"], [true, "Miles"]]
                    current: appSettings.imperialUnits; onChosen: (v) => appSettings.saveImperialUnits(v) }
        ChoiceRow { label: "Clock"; choices: [[false, "24 h"], [true, "12 h AM/PM"]]
                    current: appSettings.clock12h; onChosen: (v) => appSettings.saveClockFormat(appSettings.dateOrder, v) }
        ChoiceRow { label: "Date"; choices: [[0, "Thu 15 Oct"], [1, "Thu Oct 15"], [2, "2026-10-15"]]
                    current: appSettings.dateOrder; onChosen: (v) => appSettings.saveClockFormat(v, appSettings.clock12h) }
        ChoiceRow { label: "Week starts"; choices: [[false, "Monday"], [true, "Sunday"]]
                    current: appSettings.weekStartsSunday; onChosen: (v) => appSettings.saveWeekStart(v) }
        ChoiceRow { label: "Text size"; note: "Larger type for TVs viewed from a distance. Panes show less at once."
                    choices: [[100, "Normal"], [115, "115 %"], [130, "130 %"], [150, "150 %"]]
                    current: appSettings.textScalePercent; onChosen: (v) => appSettings.saveTextScale(v) }
        ChoiceRow { label: "Accent colour"; choices: [[0, "Blue"], [1, "Green"], [2, "Amber"], [3, "Red"], [4, "Purple"]]
                    current: appSettings.accentStyle; onChosen: (v) => appSettings.saveAccentStyle(v) }
        // Phones always show two stacked panes.
        ChoiceRow { visible: !Theme.phone; label: "Map size"
                    note: "Panes fill the width of the screen. A smaller map leaves room for a second row of panes."
                    choices: [["large", "Large"], ["medium", "Medium"], ["small", "Small"]]
                    current: appSettings.mapSize; onChosen: (v) => appSettings.saveMapSize(v) }
        ChoiceRow { label: "Big Clock face"; choices: [[0, "Digital"], [1, "Analog"]]
                    current: appSettings.bigClockFace; onChosen: (v) => appSettings.saveBigClockFace(v) }
        // The Pi, Mac and TV don't rotate.
        ChoiceRow { visible: Qt.platform.os === "ios"; label: "Screen rotation"
                    note: "Keep HamClock Pro upright or sideways however you hold the device. The Control Center rotation lock still applies."
                    choices: [["auto", "Auto"], ["portrait", "Portrait"], ["landscape", "Landscape"]]
                    current: appSettings.orientationLock; onChosen: (v) => appSettings.saveOrientationLock(v) }
        ChoiceRow { label: "Callsign banner colour"
                    choices: [[0, "Theme"], [1, "Amber"], [2, "Green"], [3, "Red"], [4, "White"], [5, "Rainbow"]]
                    current: appSettings.bannerStyle; onChosen: (v) => appSettings.saveBannerStyle(v) }
        ChoiceRow { label: "Callsign banner background"
                    choices: [[0, "No fill"], [1, "Black"], [2, "Navy"], [3, "Maroon"], [4, "White"]]
                    current: appSettings.bannerBackground; onChosen: (v) => appSettings.saveBannerBackground(v) }
        FormField { id: bannerTitle; label: "Custom banner title (tap the banner to show it; empty = off)"
                    text: appSettings.bannerTitle; field.maximumLength: 24
                    onEditingFinished: appSettings.saveBannerTitle(text) }
    }
}
