import SwiftUI

/// Settings cards after Station: the main app's Appearance, Distance units, Clock format,
/// Callsign banner, Accent colour and Map sections, plus the TV-only screen and data cards.
/// Each choice applies and is saved at once, as in SettingsPage.qml.
struct DisplaySections: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    @State private var bannerTitle = ""

    var body: some View {
        SettingsCard(title: "Appearance") {
            choices([("dark", "Dark"), ("light", "Light")], $settings.appearance, id: "appearance")
        }
        SettingsCard(title: "Distance units") {
            choices([(false, "Metric (km)"), (true, "Imperial (miles)")], $settings.imperialUnits, id: "units")
        }
        SettingsCard(title: "Clock format") {
            choices([(0, "Thu 15 Oct"), (1, "Thu Oct 15"), (2, "2026-10-15")], $settings.dateOrder, id: "date")
            choices([(false, "24 h"), (true, "12 h AM/PM")], $settings.clock12h, id: "clock")
            choices([(false, "Big clock: UTC"), (true, "Big clock: local time")], $settings.localClockPrimary, id: "primary")
        }
        SettingsCard(title: "Callsign banner") {
            choices(Array(["Theme", "Amber", "Green", "Red", "White", "Rainbow"].enumerated()).map { ($0.offset, $0.element) },
                    $settings.bannerStyle, id: "banner")
            choices(Array(["No fill", "Black", "Navy", "Maroon", "White"].enumerated()).map { ($0.offset, "Back: " + $0.element) },
                    $settings.bannerBackground, id: "bannerBack")
            HStack(alignment: .bottom, spacing: Theme.gap) {
                FormField(label: "Custom title (Play/Pause on the station card cycles to it; empty = off)",
                          placeholder: "e.g. Field Day", text: $bannerTitle, id: "bannerTitleField") { saveTitle() }
                Button("Save") { saveTitle() }.buttonStyle(ChipStyle())
            }
        }
        SettingsCard(title: "Accent colour") {
            choices(Array(["Blue", "Green", "Amber", "Red", "Purple"].enumerated()).map { ($0.offset, $0.element) },
                    $settings.accentStyle, id: "accent")
        }
        SettingsCard(title: "Map") {
            choices([("large", "Map with one row of panes"), ("hidden", "No map, two rows of panes")], $settings.mapSize, id: "mapSize")
            Text("Show on map").font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            LayerGrid()
        }
        SettingsCard(title: "Panes") {
            Text("Select a pane on the dashboard to change its widget; hold Select or press Play/Pause for its options.")
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).fixedSize(horizontal: false, vertical: true)
            Button("Reset all panes") { settings.resetPanes() }.buttonStyle(ChipStyle())
        }
        SettingsCard(title: "Screen") {
            choices([(0, "Never dim"), (2, "Dim after 2 h"), (4, "Dim after 4 h"), (8, "Dim after 8 h")], $settings.dimAfterHours, id: "dim")
            Text("Dims to 25 % after that long without the remote; any press restores it. The layout also drifts a few points each minute against burn-in.")
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).fixedSize(horizontal: false, vertical: true)
        }
        SettingsCard(title: "Data sources") {
            Text("NOAA Space Weather Prediction Center (incl. Kyoto WDC Dst) · NOAA National Hurricane Center · USGS Earthquake Hazards Program · NASA FIRMS · NASA SDO · Open-Meteo · OpenStreetMap Nominatim · CelesTrak · AMSAT · Launch Library 2 (The Space Devs) · POTA · SOTA · WWFF · Natural Earth. Every pane shows its source, the age of its data and Live, Stale or Offline.")
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).fixedSize(horizontal: false, vertical: true)
            Text("HamClock Pro \(Net.version)").font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
        }
        .onAppear { bannerTitle = settings.bannerTitle }
    }

    private func saveTitle() { settings.bannerTitle = String(bannerTitle.trimmingCharacters(in: .whitespaces).prefix(24)) }

    private func choices<V: Hashable>(_ options: [(V, String)], _ binding: Binding<V>, id: String) -> some View {
        HStack(spacing: 14) {
            ForEach(Array(options.enumerated()), id: \.offset) { i, o in
                Button(o.1) { binding.wrappedValue = o.0 }
                    .buttonStyle(ChipStyle(primary: binding.wrappedValue == o.0))
                    .accessibilityIdentifier("\(id)_\(i)")
                    .accessibilityAddTraits(binding.wrappedValue == o.0 ? .isSelected : [])
            }
        }
        .focusSection()
    }
}
