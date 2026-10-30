import Foundation

/// What the satellite card and the map draw, computed off the main actor.
struct SatSky: Equatable {
    var above: [SatPass] = []
    var subPoints: [CGPoint] = []
}

struct SatPass: Identifiable, Equatable {
    var id: String { name }
    var name: String
    var azimuth: Double
    var elevation: Double
    var rangeKm: Double
    var sunlit: Bool
    var epoch: Date
}

/// CelesTrak amateur-radio TLEs. Swift fetches (at most every 12 h, CelesTrak asks for >= 2 h),
/// the shared SGP4 core propagates on device. The last good set is cached so a relaunch offline
/// still shows positions, labelled by TLE age.
@MainActor
final class SatelliteStore: ObservableObject {
    @Published var tle: Data?
    @Published var fetched: Date?

    /// CelesTrak first; AMSAT's bare two-line set when CelesTrak fails or rate-limits.
    private static let urls = [URL(string: "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle")!,
                               URL(string: "https://www.amsat.org/tle/current/nasabare.txt")!]
    @Published var source = "CelesTrak"
    private let interval: TimeInterval = 12 * 3600
    private static let cacheURL = FileManager.default
        .urls(for: .cachesDirectory, in: .userDomainMask)[0].appendingPathComponent("amateur.tle")

    init() {
        // The cache may be purged by tvOS at any time; absence just means "No data".
        if let d = try? Data(contentsOf: Self.cacheURL),
           let attrs = try? FileManager.default.attributesOfItem(atPath: Self.cacheURL.path) {
            tle = d
            fetched = attrs[.modificationDate] as? Date
        }
    }

    func run() async {
        while !Task.isCancelled {
            if fetched.map({ Date().timeIntervalSince($0) > interval }) ?? true { await refresh() }
            try? await Task.sleep(nanoseconds: (failedOnce && tle == nil ? 120 : 600) * 1_000_000_000)
        }
    }

    private func refresh() async {
        for (i, url) in Self.urls.enumerated() {
            guard let data = await Net.get(url), data.withC(hcp_tle_count) > 0 else { continue }
            try? data.write(to: Self.cacheURL, options: .atomic)
            tle = data
            fetched = Date()
            source = i == 0 ? "CelesTrak" : "AMSAT"
            return
        }
        failedOnce = true
    }
    @Published var failedOnce = false
    /// Without a successful fetch for this long the elements are not shown at all.
    static let offlineAfter: TimeInterval = 7 * 86400

    /// Satellites above the observer (highest first) and every sub-satellite point (lon, lat),
    /// recomputed every `tick` seconds OFF the main actor by `track`: SGP4 over the whole set
    /// took ~25 ms per pass on the main thread (Simulator), once every 5 s.
    @Published private(set) var sky = SatSky()
    static let tick: TimeInterval = 5

    /// Keeps `sky` current for the station position; restarted by the app when the position changes.
    func track(position: (lat: Double, lon: Double)?) async {
        sky = SatSky()
        while !Task.isCancelled {
            if let tle {
                let t = Int64(Date().timeIntervalSince1970)
                let next = await Task.detached(priority: .utility) { Self.compute(tle: tle, at: t, observer: position) }.value
                if !Task.isCancelled { sky = next }
            }
            try? await Task.sleep(nanoseconds: UInt64(Self.tick * 1e9))
        }
    }

    /// One batched core call (the TLE text is parsed once, not once per satellite).
    nonisolated static func compute(tle: Data, at t: Int64, observer: (lat: Double, lon: Double)?) -> SatSky {
        let o = observer ?? (0, 0)
        var out = [HcpSatInfo](repeating: HcpSatInfo(), count: maxSatellites)
        let n = out.withUnsafeMutableBufferPointer { buf in
            tle.withC { hcp_sat_states($0, $1, t, o.lat, o.lon, buf.baseAddress, buf.count) }
        }
        let valid = out.prefix(min(Int(n), maxSatellites)).filter { $0.valid != 0 }
        let points = valid.map { CGPoint(x: $0.state.lon_deg, y: $0.state.lat_deg) }
        guard observer != nil else { return SatSky(above: [], subPoints: points) }
        let above = valid.filter { $0.state.el_deg > 0 && $0.name.0 != 0 }.map { info -> SatPass in
            let s = info.state
            return SatPass(name: cText(info.name), azimuth: s.az_deg, elevation: s.el_deg, rangeKm: s.range_km,
                           sunlit: s.sunlit != 0, epoch: Date(timeIntervalSince1970: TimeInterval(s.epoch_s)))
        }
        return SatSky(above: above.sorted { $0.elevation > $1.elevation }, subPoints: points)
    }

    /// The CelesTrak amateur group is about 100 sets; anything past this is ignored.
    nonisolated static let maxSatellites = 400

    /// Oldest-TLE age drives the label: elements older than 14 days drift noticeably.
    static func tleAgeDays(_ p: SatPass, now: Date) -> Double { now.timeIntervalSince(p.epoch) / 86400 }
}
