import SwiftUI

/// Passes filter (the Qt card's filter row; saved as hcp.passes.filter like [passes] filter=).
/// Amateur and Starred are not offered: the TV's elements are CelesTrak's amateur group already
/// and there is nothing to tap a star on.
enum PassesFilter: String, CaseIterable, Sendable {
    case all, visible, alerts
    var title: String { self == .all ? "All" : self == .visible ? "Visible" : "Alerts" }
    var core: Int32 { self == .all ? 0 : self == .visible ? 1 : 2 }
}

/// The next 48 h of passes at DE (PassesModel), recomputed off the main actor every minute, and
/// at once when the elements, the station, the alert choices or the filter change.
@MainActor
final class PassesStore: ObservableObject {
    struct Snapshot: Equatable {
        var rows: [HcpPassRow] = []
        var state: Int32 = 1 // 0 live, 1 loading, 2 stale (hcp_passes_schedule)
        var reason = ""
        static func == (a: Snapshot, b: Snapshot) -> Bool {
            a.state == b.state && a.reason == b.reason && a.rows.count == b.rows.count
                && zip(a.rows, b.rows).allSatisfy { $0.aos_s == $1.aos_s && $0.live == $1.live && cText($0.name) == cText($1.name) }
        }
    }
    @Published private(set) var snapshot = Snapshot()
    private static let refresh: TimeInterval = 60
    nonisolated static let maxRows = 24

    func track(tle: Data?, position: (lat: Double, lon: Double)?, prefs: SatAlertPrefs, filter: PassesFilter) async {
        snapshot = Snapshot()
        guard let tle, let position else { return }
        while !Task.isCancelled {
            let now = Int64(Date().timeIntervalSince1970)
            let next = await Task.detached(priority: .utility) {
                Parse.timed("passes") { Self.compute(tle: tle, at: position, prefs: prefs, filter: filter, now: now) }
            }.value
            if !Task.isCancelled, next != snapshot { snapshot = next }
            try? await Task.sleep(nanoseconds: UInt64(Self.refresh * 1e9))
        }
    }

    nonisolated static func compute(tle: Data, at pos: (lat: Double, lon: Double), prefs: SatAlertPrefs,
                                    filter: PassesFilter, now: Int64) -> Snapshot {
        var out = [HcpPassRow](repeating: HcpPassRow(), count: maxRows)
        var state: Int32 = 1
        var reason = [CChar](repeating: 0, count: 96)
        let n = prefs.withC { p in
            out.withUnsafeMutableBufferPointer { o in
                reason.withUnsafeMutableBufferPointer { r in
                    tle.withC { t, tl in
                        SatCatalog.json.withC { c, cl in
                            hcp_passes_schedule(t, tl, c, cl, p, pos.lat, pos.lon, now, filter.core, o.baseAddress, o.count,
                                                &state, r.baseAddress, r.count)
                        }
                    }
                }
            }
        }
        return Snapshot(rows: Array(out.prefix(max(0, Int(n)))), state: state, reason: String(cString: reason))
    }
}

/// "Passes over you" (PassesCard.qml, compact layout): satellite, when, peak elevation and the
/// rise direction, soonest first; a pass in progress says "now" in the accent colour. A pane is
/// too narrow at 29 pt for the Qt card's rise → set pair, so the set direction is only spoken.
struct PassesWidget: View {
    @EnvironmentObject var sats: SatelliteStore
    @EnvironmentObject var settings: AppSettings
    @EnvironmentObject var passes: PassesStore
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    /// Rows that fit a 390 pt dashboard pane at 29 pt, so no row is cut in half.
    static let shownRows = 5

    var body: some View {
        let snap = passes.snapshot
        let fetchAge = sats.fetched.map { now.timeIntervalSince($0) } ?? .infinity
        let state: WidgetState = !settings.configured ? .unconfigured
            : sats.tle == nil ? (sats.failedOnce ? .offline : .loading)
            : fetchAge > SatelliteStore.offlineAfter ? .offline
            : snap.state == 2 ? .stale : snap.state == 1 ? .loading : .live
        let filter = settings.passesFilter
        Card(title: "Passes", chooser: true, state: state,
             status: (filter == .all ? sats.source : filter.title) + " · SGP4",
             age: Age.text(sats.fetched, now: now), focused: focused) {
            if !settings.configured {
                Note(text: "Set your station in Settings.")
            } else if state == .offline {
                Note(text: "No satellite elements for a week.")
            } else if snap.rows.isEmpty {
                Note(text: snap.reason.isEmpty ? "Computing passes…" : snap.reason)
            } else {
                rows(Array(snap.rows.prefix(Self.shownRows)))
            }
        }
    }

    private func rows(_ list: [HcpPassRow]) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            ForEach(Array(list.enumerated()), id: \.offset) { _, r in
                let tint = r.live != 0 ? theme.accent : theme.secondary
                HStack(spacing: 8) {
                    // Long names ("FUNCUBE-1") give way a little; time and direction never do.
                    Text(cText(r.name)).fontWeight(.semibold).foregroundStyle(r.live != 0 ? theme.accent : theme.ink)
                        .minimumScaleFactor(0.85).frame(maxWidth: .infinity, alignment: .leading)
                    if Self.isSoon(r) {
                        Text(Self.when(r)).foregroundStyle(tint).fixedSize().frame(width: Self.whenWidth, alignment: .leading)
                        Text("\(r.max_el_deg)° \(cText(r.from))").foregroundStyle(tint).fixedSize()
                            .frame(width: Self.directionWidth, alignment: .leading)
                            .accessibilityLabel("\(r.max_el_deg)° \(cText(r.from)) to \(cText(r.to))")
                    } else {
                        // A later day needs the width of both columns; the peak still fits, the direction does not.
                        Text("\(Self.when(r))  \(r.max_el_deg)°").foregroundStyle(tint).fixedSize()
                    }
                }
                .font(.system(size: Theme.fontCaption))
                .lineLimit(1)
            }
        }
    }

    /// "19:06Z" and "64° WNW" at 29 pt.
    private static let whenWidth: CGFloat = 94
    private static let directionWidth: CGFloat = 130

    private static func isSoon(_ r: HcpPassRow) -> Bool { r.live != 0 || cText(r.day) == "Today" }

    /// PassesCard.when for a compact slot: "now", "14:05Z" today, else "Tmrw 14:05Z" or "Mon 14:05Z".
    static func when(_ r: HcpPassRow) -> String {
        if r.live != 0 { return "now" }
        let day = cText(r.day), utc = cText(r.utc)
        if day == "Today" { return utc }
        return (day == "Tomorrow" ? "Tmrw" : day) + " " + utc
    }
}
