import Foundation

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

    private static let url = URL(string: "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle")!
    private let interval: TimeInterval = 12 * 3600
    private static let cacheURL = FileManager.default
        .urls(for: .cachesDirectory, in: .userDomainMask)[0].appendingPathComponent("amateur.tle")
    private let session: URLSession = {
        let cfg = URLSessionConfiguration.default
        cfg.timeoutIntervalForRequest = 30
        cfg.httpAdditionalHeaders = ["User-Agent": SpaceWeatherStore.userAgent]
        return URLSession(configuration: cfg)
    }()

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
            try? await Task.sleep(nanoseconds: 600 * 1_000_000_000)
        }
    }

    private func refresh() async {
        guard let (data, resp) = try? await session.data(from: Self.url),
              (resp as? HTTPURLResponse)?.statusCode == 200,
              data.withUnsafeBytes({ hcp_tle_count($0.bindMemory(to: CChar.self).baseAddress, data.count) }) > 0
        else { return }
        try? data.write(to: Self.cacheURL, options: .atomic)
        tle = data
        fetched = Date()
    }

    private var cache: (bucket: Int64, lat: Double, lon: Double, tle: Int, passes: [SatPass])?

    /// Cached per 5 s: each core call re-parses the whole TLE set, so don't redo it every clock tick.
    func visibleCached(at now: Date, lat: Double, lon: Double) -> [SatPass] {
        guard let tle else { return [] }
        let bucket = Int64(now.timeIntervalSince1970) / 5
        if let c = cache, c.bucket == bucket, c.lat == lat, c.lon == lon, c.tle == tle.count { return c.passes }
        let passes = Self.visible(tle: tle, at: Date(timeIntervalSince1970: TimeInterval(bucket * 5)), lat: lat, lon: lon)
        cache = (bucket, lat, lon, tle.count, passes)
        return passes
    }

    /// Satellites above the horizon for the observer, highest first.
    static func visible(tle: Data, at now: Date, lat: Double, lon: Double, limit: Int = 5) -> [SatPass] {
        let t = Int64(now.timeIntervalSince1970)
        return tle.withUnsafeBytes { raw -> [SatPass] in
            let p = raw.bindMemory(to: CChar.self).baseAddress
            let n = Int(hcp_tle_count(p, tle.count))
            var out: [SatPass] = []
            for i in 0..<n {
                var s = HcpSatState()
                guard hcp_sat_state(p, tle.count, Int32(i), t, lat, lon, &s) != 0, s.el_deg > 0 else { continue }
                var name = [CChar](repeating: 0, count: 32)
                guard hcp_tle_name(p, tle.count, Int32(i), &name, name.count) != 0 else { continue }
                out.append(SatPass(name: String(cString: name), azimuth: s.az_deg, elevation: s.el_deg,
                                   rangeKm: s.range_km, sunlit: s.sunlit != 0,
                                   epoch: Date(timeIntervalSince1970: TimeInterval(s.epoch_s))))
            }
            return Array(out.sorted { $0.elevation > $1.elevation }.prefix(limit))
        }
    }

    private var pointCache: (bucket: Int64, tle: Int, points: [CGPoint])?

    /// Sub-satellite points (lon, lat) of every satellite, cached per 5 s like visibleCached.
    func subPointsCached(at now: Date) -> [CGPoint] {
        guard let tle else { return [] }
        let bucket = Int64(now.timeIntervalSince1970) / 5
        if let c = pointCache, c.bucket == bucket, c.tle == tle.count { return c.points }
        let t = bucket * 5
        let pts = tle.withUnsafeBytes { raw -> [CGPoint] in
            let p = raw.bindMemory(to: CChar.self).baseAddress
            return (0..<Int(hcp_tle_count(p, tle.count))).compactMap { i in
                var s = HcpSatState()
                return hcp_sat_state(p, tle.count, Int32(i), t, 0, 0, &s) != 0 ? CGPoint(x: s.lon_deg, y: s.lat_deg) : nil
            }
        }
        pointCache = (bucket, tle.count, pts)
        return pts
    }

    /// Oldest-TLE age drives the label: elements older than 14 days drift noticeably.
    static func tleAgeDays(_ p: SatPass, now: Date) -> Double { now.timeIntervalSince(p.epoch) / 86400 }
}
