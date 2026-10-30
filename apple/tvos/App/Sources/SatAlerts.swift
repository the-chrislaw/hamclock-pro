import SwiftUI

/// Satellite pass alert choices, the same fields and meaning as the Qt app's [satalerts]
/// (src/widgets/satellites/sat_alert_prefs.hpp), saved as hcp.satalerts.* keys.
struct SatAlertPrefs: Equatable, Sendable {
    var enabled = false
    var allWorkable = true
    var muted: [Int] = []
    var picked: [Int] = []
    var minElevation = 15
    var leadMinutes = 5
    var quietStart = -1
    var quietEnd = -1
    var daylightOnly = false
    var announceStart = true

    /// Runs `body` with the C view of these prefs (the id arrays stay alive for the call).
    func withC<T>(_ body: (UnsafePointer<HcpSatAlertPrefs>) -> T) -> T {
        let muted32 = muted.map(Int32.init), picked32 = picked.map(Int32.init)
        return muted32.withUnsafeBufferPointer { m in
            picked32.withUnsafeBufferPointer { p in
                var c = HcpSatAlertPrefs(enabled: enabled ? 1 : 0, all_workable: allWorkable ? 1 : 0,
                                         min_elevation_deg: Int32(minElevation), lead_minutes: Int32(leadMinutes),
                                         quiet_start_hour: Int32(quietStart), quiet_end_hour: Int32(quietEnd),
                                         daylight_only: daylightOnly ? 1 : 0, announce_start: announceStart ? 1 : 0,
                                         muted: m.baseAddress, muted_count: m.count, picked: p.baseAddress, picked_count: p.count)
                return body(&c)
            }
        }
    }
}

/// The curated amateur-satellite catalog shipped with every HamClock Pro app
/// (src/widgets/satellites/amateur-satellites.json), copied into the TV bundle.
enum SatCatalog {
    static let json: Data = Bundle.main.url(forResource: "amateur-satellites", withExtension: "json")
        .flatMap { try? Data(contentsOf: $0) } ?? Data()

    struct Bird: Identifiable, Equatable {
        var id: Int { norad }
        let norad: Int
        let name: String
        let mode: String
        let workable: Bool
    }

    static let birds: [Bird] = {
        var out = [HcpSatCatalogEntry](repeating: HcpSatCatalogEntry(), count: 200)
        let n = out.withUnsafeMutableBufferPointer { o in json.withC { hcp_satalert_catalog($0, $1, o.baseAddress, o.count) } }
        return out.prefix(Int(n)).map {
            Bird(norad: Int($0.norad), name: cText($0.name), mode: cText($0.mode), workable: $0.workable != 0)
        }
    }()

    static func isSelected(_ prefs: SatAlertPrefs, _ norad: Int) -> Bool {
        prefs.withC { p in json.withC { hcp_satalert_selected(p, $0, $1, Int32(norad)) } } != 0
    }

    /// Flips one bird in or out, keeping "All workable" as it is (toggle_alert_satellite).
    static func toggle(_ prefs: SatAlertPrefs, _ norad: Int) -> SatAlertPrefs {
        guard norad > 0 else { return prefs }
        var next = prefs
        if isSelected(prefs, norad) {
            next.picked.removeAll { $0 == norad }
            if isSelected(next, norad) { next.muted = Array(Set(prefs.muted + [norad])).sorted() }
        } else {
            next.muted.removeAll { $0 == norad }
            if !isSelected(next, norad) { next.picked = Array(Set(prefs.picked + [norad])).sorted() }
        }
        return next
    }
}

/// Predicts the selected birds' passes off the main actor (every 10 min, or when the
/// elements, the station or the choices change); the banner is picked from them each second.
@MainActor
final class SatAlertStore: ObservableObject {
    @Published private(set) var passes: [HcpAlertPass] = []
    private static let refresh: TimeInterval = 600

    func track(tle: Data?, position: (lat: Double, lon: Double)?, prefs: SatAlertPrefs) async {
        passes = []
        guard prefs.enabled, let tle, let position else { return }
        while !Task.isCancelled {
            let now = Int64(Date().timeIntervalSince1970)
            let next = await Task.detached(priority: .utility) {
                Parse.timed("sat passes") { Self.predict(tle: tle, at: position, prefs: prefs, now: now) }
            }.value
            if !Task.isCancelled { passes = next }
            try? await Task.sleep(nanoseconds: UInt64(Self.refresh * 1e9))
        }
    }

    nonisolated static func predict(tle: Data, at pos: (lat: Double, lon: Double), prefs: SatAlertPrefs, now: Int64) -> [HcpAlertPass] {
        var out = [HcpAlertPass](repeating: HcpAlertPass(), count: 600)
        let n = prefs.withC { p in
            out.withUnsafeMutableBufferPointer { o in
                tle.withC { t, tl in
                    SatCatalog.json.withC { c, cl in
                        hcp_satalert_predict(t, tl, c, cl, p, pos.lat, pos.lon, now, o.baseAddress, o.count)
                    }
                }
            }
        }
        return Array(out.prefix(Int(n)))
    }

    /// The banner due at `now` (text, live), or nil.
    func banner(at now: Date, prefs: SatAlertPrefs, zone: TimeZone) -> (text: String, live: Bool)? {
        guard prefs.enabled, !passes.isEmpty else { return nil }
        var buf = [CChar](repeating: 0, count: 256)
        var live: Int32 = 0
        let offset = Int32(zone.secondsFromGMT(for: now))
        let shown = prefs.withC { p in
            passes.withUnsafeBufferPointer { list in
                buf.withUnsafeMutableBufferPointer { b in
                    SatCatalog.json.withC { c, cl in
                        hcp_satalert_banner(list.baseAddress, list.count, p, c, cl, offset, Int64(now.timeIntervalSince1970),
                                            b.baseAddress, b.count, &live)
                    }
                }
            }
        }
        return shown == 1 ? (String(cString: buf), live != 0) : nil
    }
}

/// In-app pass alert (SatAlertBanner.qml): accent while the bird is coming, green while it is up.
/// Never takes the focus; it goes away by itself at LOS.
struct SatAlertBanner: View {
    @Environment(\.theme) private var theme
    let text: String
    let live: Bool
    var body: some View {
        HStack(spacing: 14) {
            Image(systemName: "dot.radiowaves.left.and.right")
            Text(text).lineLimit(2).minimumScaleFactor(0.8)
        }
        .font(.system(size: Theme.fontBody, weight: .semibold))
        .foregroundStyle(theme.canvas)
        .padding(.horizontal, 28).padding(.vertical, 14)
        .background(RoundedRectangle(cornerRadius: Theme.radius).fill(live ? theme.good : theme.accent))
        .shadow(color: .black.opacity(0.4), radius: 12, y: 6)
        .frame(maxWidth: 1300)
        .accessibilityElement(children: .combine)
        .accessibilityIdentifier("satAlertBanner")
    }
}
