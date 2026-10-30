import Foundation
import os

/// Last known space weather, cached between timeline reloads so an offline refresh still
/// shows the previous values (labelled by age, never as live).
struct SpaceWeatherSnapshot: Codable, Equatable {
    var kp: Reading?
    var sfi: Reading?
    var xray: Reading?
    /// Last fetch in which every product parsed.
    var fetched: Date?
    var lastAttemptFailed = false

    func freshness(now: Date) -> Freshness {
        Freshness.evaluate(fetched: fetched, lastAttemptFailed: lastAttemptFailed, now: now)
    }

    /// New values replace cached ones product by product; a partial result is not "live".
    func merged(kp newKp: Reading?, sfi newSfi: Reading?, xray newXray: Reading?, now: Date) -> SpaceWeatherSnapshot {
        let complete = newKp != nil && newSfi != nil && newXray != nil
        let any = newKp != nil || newSfi != nil || newXray != nil
        return SpaceWeatherSnapshot(kp: newKp ?? kp, sfi: newSfi ?? sfi, xray: newXray ?? xray,
                                    fetched: complete ? now : (fetched ?? (any ? now : nil)),
                                    lastAttemptFailed: !complete)
    }
}

/// Satellite elements, cached because CelesTrak asks clients not to poll more than every two hours.
struct TleSnapshot: Equatable {
    var data: Data
    var fetched: Date
    var lastAttemptFailed: Bool
}

/// Data sources: the same public endpoints the Qt app uses (src/widgets/spaceweather,
/// src/widgets/bands, src/widgets/satellites). One fetch function is injected so tests run offline.
struct Feeds {
    typealias Fetch = (URL) async -> Data?

    static let kpURL = URL(string: "https://services.swpc.noaa.gov/products/noaa-planetary-k-index.json")!
    static let fluxURL = URL(string: "https://services.swpc.noaa.gov/products/summary/10cm-flux.json")!
    static let xrayURL = URL(string: "https://services.swpc.noaa.gov/json/goes/primary/xrays-6-hour.json")!
    static let tleURL = URL(string: "https://celestrak.org/NORAD/elements/gp.php?GROUP=amateur&FORMAT=tle")!
    /// CelesTrak requests at most one download per two hours; the app uses 12 h.
    static let tleRefresh: TimeInterval = 12 * 3600

    static var userAgent: String {
        let v = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "0"
        return "HamClockPro/\(v) (+https://github.com/the-chrislaw/hamclock-pro)"
    }

    private static let log = Logger(subsystem: "com.chrislaw.hamclockpro.widgets", category: "feeds")

    /// URLSession fetch: HTTP 200 only, 20 s timeout, honest User-Agent.
    static let network: Fetch = { url in
        let cfg = URLSessionConfiguration.ephemeral
        cfg.timeoutIntervalForRequest = 20
        cfg.httpAdditionalHeaders = ["User-Agent": userAgent]
        do {
            let (data, resp) = try await URLSession(configuration: cfg).data(from: url)
            guard (resp as? HTTPURLResponse)?.statusCode == 200 else {
                log.error("HTTP \((resp as? HTTPURLResponse)?.statusCode ?? -1, privacy: .public) for \(url.host ?? "", privacy: .public)")
                return nil
            }
            return data
        } catch {
            log.error("fetch failed for \(url.host ?? "", privacy: .public): \(error.localizedDescription, privacy: .public)")
            return nil
        }
    }

    var fetch: Fetch = Feeds.network
    var cacheDirectory: URL = AppGroup.cacheDirectory

    private var weatherCacheURL: URL { cacheDirectory.appendingPathComponent("spaceweather.json") }
    private var tleCacheURL: URL { cacheDirectory.appendingPathComponent("amateur.tle") }

    func cachedSpaceWeather() -> SpaceWeatherSnapshot? {
        guard let d = try? Data(contentsOf: weatherCacheURL) else { return nil }
        return try? JSONDecoder().decode(SpaceWeatherSnapshot.self, from: d)
    }

    func spaceWeather(now: Date = Date()) async -> SpaceWeatherSnapshot {
        async let k = fetch(Self.kpURL)
        async let f = fetch(Self.fluxURL)
        async let x = fetch(Self.xrayURL)
        let (kd, fd, xd) = await (k, f, x)
        let cached = cachedSpaceWeather() ?? SpaceWeatherSnapshot()
        let next = cached.merged(kp: kd.flatMap(Core.kp), sfi: fd.flatMap(Core.solarFlux),
                                 xray: xd.flatMap(Core.xray), now: now)
        write(try? JSONEncoder().encode(next), to: weatherCacheURL)
        return next
    }

    func cachedTle() -> TleSnapshot? {
        guard let d = try? Data(contentsOf: tleCacheURL), Core.tleCount(d) > 0,
              let attrs = try? FileManager.default.attributesOfItem(atPath: tleCacheURL.path),
              let mod = attrs[.modificationDate] as? Date else { return nil }
        return TleSnapshot(data: d, fetched: mod, lastAttemptFailed: false)
    }

    /// Cached elements while younger than tleRefresh; otherwise downloads, falling back to the cache.
    func tle(now: Date = Date()) async -> TleSnapshot? {
        let cached = cachedTle()
        if let c = cached, now.timeIntervalSince(c.fetched) < Self.tleRefresh { return c }
        if let d = await fetch(Self.tleURL), Core.tleCount(d) > 0 {
            write(d, to: tleCacheURL)
            return TleSnapshot(data: d, fetched: now, lastAttemptFailed: false)
        }
        return cached.map { TleSnapshot(data: $0.data, fetched: $0.fetched, lastAttemptFailed: true) }
    }

    /// Class C protection: readable while the phone is locked after first unlock (CarPlay, StandBy).
    private func write(_ data: Data?, to url: URL) {
        guard let data else { return }
        do {
            try data.write(to: url, options: [.atomic, .completeFileProtectionUntilFirstUserAuthentication])
        } catch {
            Self.log.error("cache write failed: \(error.localizedDescription, privacy: .public)")
        }
    }
}
