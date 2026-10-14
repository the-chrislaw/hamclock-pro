import Foundation

/// One NOAA SWPC product reading as the C++ core parsed it, with the time we fetched it.
struct Sample: Equatable {
    var value: Double
    var observed: Date
    var fetched: Date
}

enum Freshness { case live, stale, offline }

/// Swift owns networking; the Qt-free C++ core owns interpretation (hcpcore.h).
@MainActor
final class SpaceWeatherStore: ObservableObject {
    @Published var kp: Sample?
    @Published var flux: Sample?
    @Published var xray: Sample?

    static let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "0"
    static let userAgent = "HamClockPro/\(version) (+https://github.com/the-chrislaw/hamclock-pro)"

    private let session: URLSession = {
        let cfg = URLSessionConfiguration.default
        cfg.timeoutIntervalForRequest = 20
        cfg.httpAdditionalHeaders = ["User-Agent": SpaceWeatherStore.userAgent]
        return cfg
    }()

    private static let base = "https://services.swpc.noaa.gov"
    /// Refresh interval matches the Qt app's SWPC cadence.
    private let interval: TimeInterval = 120

    func run() async {
        while !Task.isCancelled {
            async let k = fetch("/products/noaa-planetary-k-index.json", hcp_swpc_latest_kp)
            async let f = fetch("/products/summary/10cm-flux.json", hcp_swpc_latest_solar_flux)
            async let x = fetch("/json/goes/primary/xrays-6-hour.json", hcp_swpc_latest_xray)
            // A failed fetch keeps the previous sample; its age then drives Stale/Offline.
            if let v = await k { kp = v }
            if let v = await f { flux = v }
            if let v = await x { xray = v }
            try? await Task.sleep(nanoseconds: UInt64(interval * 1e9))
        }
    }

    private func fetch(_ path: String, _ parse: (UnsafePointer<CChar>?, Int) -> HcpReading) async -> Sample? {
        guard let url = URL(string: Self.base + path),
              let (data, resp) = try? await session.data(from: url),
              (resp as? HTTPURLResponse)?.statusCode == 200 else { return nil }
        let r = data.withUnsafeBytes { raw -> HcpReading in
            let p = raw.bindMemory(to: CChar.self).baseAddress
            return parse(p, data.count)
        }
        guard r.valid != 0 else { return nil }
        return Sample(value: r.value,
                      observed: Date(timeIntervalSince1970: Double(r.time_ms) / 1000),
                      fetched: Date())
    }

    static func freshness(_ s: Sample?, now: Date, staleAfter: TimeInterval = 900,
                          offlineAfter: TimeInterval = 3600) -> Freshness {
        guard let s else { return .offline }
        let age = now.timeIntervalSince(s.fetched)
        return age > offlineAfter ? .offline : age > staleAfter ? .stale : .live
    }

    static func xrayClass(_ flux: Double) -> String {
        var buf = [CChar](repeating: 0, count: 16)
        return hcp_swpc_xray_class(flux, &buf, buf.count) != 0 ? String(cString: buf) : "–"
    }

    static func kpDescription(_ kp: Double) -> String {
        var buf = [CChar](repeating: 0, count: 64)
        return hcp_swpc_kp_description(kp, &buf, buf.count) != 0 ? String(cString: buf) : ""
    }
}
