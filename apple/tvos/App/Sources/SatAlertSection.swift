import SwiftUI

/// Settings > Satellite pass alerts (the Qt app's SatelliteAlertsCard.qml, without system
/// notifications: on TV the alert is the in-app banner). Each choice applies and is saved at once.
struct SatAlertSection: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme

    private static let leads = [0, 5, 10, 15, 30]
    private static let elevations = [0, 15, 30, 45]

    var body: some View {
        let p = settings.satAlerts
        SettingsCard(title: "Satellite pass alerts") {
            row([(false, "Off"), (true, "On")], p.enabled, id: "satEnabled") { settings.satAlerts.enabled = $0 }
            Text("A banner across the top of the dashboard before a selected satellite rises, and while it is up.")
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).fixedSize(horizontal: false, vertical: true)
            if p.enabled {
                row(Self.leads.map { ($0, $0 == 0 ? "At AOS" : "\($0) min before") }, p.leadMinutes, id: "satLead") {
                    settings.satAlerts.leadMinutes = $0
                }
                row(Self.elevations.map { ($0, $0 == 0 ? "Any pass" : "Peak ≥ \($0)°") }, p.minElevation, id: "satMinEl") {
                    settings.satAlerts.minElevation = $0
                }
                row([(true, "Banner while up"), (false, "Before AOS only")], p.announceStart, id: "satAnnounce") {
                    settings.satAlerts.announceStart = $0
                }
                row([(false, "Day and night"), (true, "Sun up only")], p.daylightOnly, id: "satDaylight") {
                    settings.satAlerts.daylightOnly = $0
                }
                row([(-1, "No quiet hours"), (22, "Quiet 22:00–07:00"), (23, "Quiet 23:00–06:00")], p.quietStart, id: "satQuiet") {
                    settings.satAlerts.quietStart = $0
                    settings.satAlerts.quietEnd = $0 == 22 ? 7 : $0 == 23 ? 6 : -1
                }
                row([(true, "All workable"), (false, "Only the ones I pick")], p.allWorkable, id: "satAll") { all in
                    settings.satAlerts.allWorkable = all
                    if !all { settings.satAlerts.muted = [] } // mutes only mean something against "all"
                }
                birds(p)
            }
        }
    }

    private func birds(_ p: SatAlertPrefs) -> some View {
        // Not lazy: every bird exists for the focus engine (and UI tests) even below the fold.
        let birds = SatCatalog.birds
        return Grid(horizontalSpacing: 12, verticalSpacing: 12) {
            ForEach(Array(stride(from: 0, to: birds.count, by: 3)), id: \.self) { row in
                GridRow {
                    ForEach(birds[row..<min(row + 3, birds.count)]) { b in bird(b, p) }
                }
            }
        }
        .focusSection()
    }

    private func bird(_ b: SatCatalog.Bird, _ p: SatAlertPrefs) -> some View {
        let on = SatCatalog.isSelected(p, b.norad)
        return Button((on ? "✓ " : "   ") + b.name + " · " + b.mode + (b.workable ? "" : " (quiet)")) {
            settings.satAlerts = SatCatalog.toggle(settings.satAlerts, b.norad)
        }
        .buttonStyle(ChipStyle(primary: on, wide: true))
        .accessibilityIdentifier("sat_\(b.norad)")
        .accessibilityAddTraits(on ? .isSelected : [])
    }

    private func row<V: Hashable>(_ options: [(V, String)], _ current: V, id: String, set: @escaping (V) -> Void) -> some View {
        HStack(spacing: 14) {
            ForEach(Array(options.enumerated()), id: \.offset) { i, o in
                Button(o.1) { set(o.0) }
                    .buttonStyle(ChipStyle(primary: current == o.0))
                    .accessibilityIdentifier("\(id)_\(i)")
                    .accessibilityAddTraits(current == o.0 ? .isSelected : [])
            }
        }
        .focusSection()
    }
}
