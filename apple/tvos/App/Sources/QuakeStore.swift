import Foundation

struct QuakeItem: Identifiable, Equatable {
    var id: Int64 { time }
    var time: Int64 // origin time, UTC ms
    var magnitude: Double
    var place: String
    var lat = 0.0
    var lon = 0.0
}

/// USGS 2.5+ day feed. Swift fetches, the Qt-free C++ core parses (hcpcore.h).
@MainActor
final class QuakeStore: ObservableObject {
    @Published var recent: [QuakeItem] = []
    @Published var all: [QuakeItem] = [] // every parsed quake, for the map
    @Published var strongest: QuakeItem?
    @Published var fetched: Date?

    private static let url = URL(string: "https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson")!
    private let interval: TimeInterval = 300
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
        var all = [HcpQuake](repeating: HcpQuake(), count: 500)
        let (n, top) = data.withUnsafeBytes { raw -> (Int32, Int32) in
            let p = raw.bindMemory(to: CChar.self).baseAddress
            return (hcp_quakes_parse(p, data.count, &all, all.count), hcp_quakes_strongest_index(p, data.count))
        }
        guard n >= 0 else { return }
        func item(_ q: HcpQuake) -> QuakeItem {
            var place = q.place
            let text = withUnsafePointer(to: &place) {
                $0.withMemoryRebound(to: CChar.self, capacity: 96) { String(cString: $0) }
            }
            return QuakeItem(time: q.time_ms, magnitude: q.magnitude, place: text, lat: q.latitude, lon: q.longitude)
        }
        self.all = all.prefix(Int(n)).map(item)
        recent = all.prefix(Int(n)).prefix(5).map(item)
        strongest = top >= 0 && Int(top) < all.count ? item(all[Int(top)]) : nil
        fetched = Date()
    }

    static func magnitudeClass(_ m: Double) -> String {
        var buf = [CChar](repeating: 0, count: 16)
        return hcp_quake_magnitude_class(m, &buf, buf.count) != 0 ? String(cString: buf) : ""
    }
}
