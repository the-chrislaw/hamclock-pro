import SwiftUI

/// Settings layer (SettingsPage.qml): a scrolling column of cards. In setup mode (no station
/// yet) it opens as the first-run screen with the Station card first. Menu saves and closes.
struct SettingsView: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let setup: Bool
    let close: () -> Void
    @State private var zonePicker: ZonePickerTarget?

    var body: some View {
        ZStack {
            theme.canvas.ignoresSafeArea()
            ScrollView {
                VStack(alignment: .leading, spacing: Theme.gap) {
                    HStack {
                        Text(setup ? "Set up HamClock Pro" : "Settings")
                            .font(.system(size: Theme.fontTitle * 1.4, weight: .bold)).foregroundStyle(theme.ink)
                        Spacer()
                        Button(setup ? "Later" : "Done", action: close).buttonStyle(ChipStyle(primary: !setup))
                            .accessibilityIdentifier("doneButton")
                    }
                    if setup {
                        Text("Add your callsign and location to see local time, sunrise and sunset, paths and the weather at your station. Settings are kept on this Apple TV.")
                            .font(.system(size: Theme.fontBody)).foregroundStyle(theme.secondary)
                            .fixedSize(horizontal: false, vertical: true)
                    }
                    StationSection(setup: setup, pickZone: { zonePicker = $0 }, saved: { if setup { close() } })
                    if !setup { DisplaySections() }
                }
                .frame(width: 1300)
                .frame(maxWidth: .infinity)
                .padding(.vertical, 20)
            }
            .disabled(zonePicker != nil)
            if let target = zonePicker {
                ZonePicker(target: target) { zonePicker = nil }
            }
        }
        .onExitCommand { if zonePicker == nil { close() } }
    }
}

/// Section card used by every Settings group.
struct SettingsCard<Content: View>: View {
    @Environment(\.theme) private var theme
    let title: String
    @ViewBuilder var content: () -> Content
    var body: some View {
        VStack(alignment: .leading, spacing: Theme.gap * 0.8) {
            Text(title).font(.system(size: Theme.fontTitle, weight: .semibold)).foregroundStyle(theme.ink)
            content()
        }
        .padding(Theme.pad)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: Theme.radius).fill(theme.surface))
        .overlay(RoundedRectangle(cornerRadius: Theme.radius).strokeBorder(theme.divider, lineWidth: 1.5))
        .focusSection()
    }
}

/// Labelled text field (FormField.qml); the tvOS keyboard opens on Select.
struct FormField: View {
    @Environment(\.theme) private var theme
    let label: String
    let placeholder: String
    @Binding var text: String
    var id: String
    var submit: () -> Void = {}
    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(label).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            TextField(placeholder, text: $text)
                .font(.system(size: Theme.fontBody))
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .onSubmit(submit)
                .accessibilityIdentifier(id)
        }
    }
}

enum ZonePickerTarget: Equatable { case de, dx }

/// Full list of Region/City zones, narrowed by region (TimeZonePicker.qml).
struct ZonePicker: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let target: ZonePickerTarget
    let close: () -> Void
    @State private var region = "America"
    private static let regions = ["Africa", "America", "Antarctica", "Asia", "Atlantic", "Australia", "Europe", "Indian", "Pacific"]

    var body: some View {
        Overlay(title: target == .de ? "Time zone" : "DX time zone", width: 1200, close: close) {
            HStack(spacing: 10) {
                ForEach(Self.regions, id: \.self) { r in
                    Button(r) { region = r }.buttonStyle(ChipStyle(primary: region == r))
                }
            }
            .focusSection()
            ScrollView {
                LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 12), count: 3), spacing: 12) {
                    if target == .dx {
                        Button("Automatic") { ZoneChoice.shared.pick(target, ""); close() }.buttonStyle(ChipStyle(wide: true))
                    }
                    ForEach(TimeZones.all.filter { $0.hasPrefix(region + "/") }, id: \.self) { z in
                        Button(z.replacingOccurrences(of: region + "/", with: "").replacingOccurrences(of: "_", with: " ")) {
                            ZoneChoice.shared.pick(target, z); close()
                        }
                        .buttonStyle(ChipStyle(wide: true))
                    }
                }
                .padding(10)
            }
            .frame(height: 620)
        }
        .onAppear {
            let current = ZoneChoice.shared.value(target)
            if let r = current.split(separator: "/").first.map(String.init), Self.regions.contains(r) { region = r }
        }
    }
}

/// Hand-off between the zone picker and the station form (both live in the Settings layer).
@MainActor
final class ZoneChoice: ObservableObject {
    static let shared = ZoneChoice()
    @Published var de = ""
    @Published var dx = ""
    func pick(_ t: ZonePickerTarget, _ z: String) { if t == .de { de = z } else { dx = z } }
    func value(_ t: ZonePickerTarget) -> String { t == .de ? de : dx }
}
