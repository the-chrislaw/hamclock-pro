import Foundation

struct StormItem: Identifiable, Equatable {
    var id: String { name + String(issued) }
    var name: String
    var classification: String
    var heading: String
    var windKt: Int
    var pressureMb: Int
    var category: Int
    var speedKt: Int
    var lat: Double
    var lon: Double
    var issued: Int64 // advisory issuance, UTC ms (0 unknown)
}

/// NHC active-storm index (Atlantic and East/Central Pacific). Swift fetches, the Qt-free C++ core parses.
/// An empty list is a valid answer ("no active storms") and is only shown after a successful fetch.
@MainActor
final class StormStore: ObservableObject {
    @Published var storms: [StormItem] = []
    @Published var fetched: Date?

    private static let url = URL(string: "https://www.nhc.noaa.gov/CurrentStorms.json")!
    private let interval: TimeInterval = 1800 // the index changes only with advisories
    private let session: URLSession = {
        let cfg = URLSessionConfiguration.default
        cfg.timeoutIntervalForRequest = 20
        cfg.httpAdditionalHeaders = ["User-Agent": SpaceWeatherStore.userAgent]
        return URLSession(configuration: cfg)
    }()

    func run() async {
        while !Task.isCancelled {
            await refresh()
            try? await Task.sleep(nanoseconds: UInt64(interval * 1e9))
        }
    }

    private func refresh() async {
        // A failed fetch keeps the previous list; its age drives Stale/Offline.
        guard let (data, resp) = try? await session.data(from: Self.url),
              (resp as? HTTPURLResponse)?.statusCode == 200 else { return }
        var raw = [HcpStorm](repeating: HcpStorm(), count: 20)
        let n = data.withUnsafeBytes { hcp_storms_parse($0.bindMemory(to: CChar.self).baseAddress, data.count, &raw, raw.count) }
        guard n >= 0 else { return }
        func text<T>(_ v: T) -> String {
            var v = v
            return withUnsafePointer(to: &v) { $0.withMemoryRebound(to: CChar.self, capacity: MemoryLayout<T>.size) { String(cString: $0) } }
        }
        storms = raw.prefix(Int(n)).map {
            StormItem(name: text($0.name), classification: text($0.classification), heading: text($0.heading),
                      windKt: Int($0.wind_kt), pressureMb: Int($0.pressure_mb), category: Int($0.category),
                      speedKt: Int($0.speed_kt), lat: $0.latitude, lon: $0.longitude, issued: $0.issued_ms)
        }
        fetched = Date()
    }
}
