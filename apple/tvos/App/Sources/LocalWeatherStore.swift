import Foundation

struct LocalWeather: Equatable {
    var temperatureC: Double
    var humidity: Int
    var windKmh: Double
    var windFrom: String
    var description: String
    var dewPointC: Double?
    var pressureHpa: Double?
    var trend: String?
}

/// Open-Meteo current conditions at the operator's locator. Swift fetches, the Qt-free C++ core
/// parses (hcpcore.h). No locator means no request and no card content.
@MainActor
final class LocalWeatherStore: ObservableObject {
    @Published var current: LocalWeather?
    @Published var fetched: Date?
    private var fetchedFor: String?

    private let interval: TimeInterval = 900
    private let session: URLSession = {
        let cfg = URLSessionConfiguration.default
        cfg.timeoutIntervalForRequest = 20
        cfg.httpAdditionalHeaders = ["User-Agent": SpaceWeatherStore.userAgent]
        return URLSession(configuration: cfg)
    }()

    /// Same query as the Qt app's weather_parse::request_url (coordinates to 2 decimals).
    static func url(lat: Double, lon: Double) -> URL? {
        URL(string: String(format: "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
            + "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m,weather_code,is_day,pressure_msl,cloud_cover"
            + "&hourly=pressure_msl&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&past_hours=3&forecast_hours=1&timezone=auto",
            lat, lon))
    }

    func run(station: StationStore) async {
        while !Task.isCancelled {
            await refresh(station.position, locator: station.locator)
            try? await Task.sleep(nanoseconds: UInt64(interval * 1e9))
        }
    }

    private func refresh(_ pos: (lat: Double, lon: Double)?, locator: String) async {
        guard let pos, let url = Self.url(lat: pos.lat, lon: pos.lon) else { return }
        if fetchedFor != locator { current = nil; fetched = nil } // never show another place's weather
        // A failed fetch keeps the previous reading; its age drives Stale/Offline.
        guard let (data, resp) = try? await session.data(from: url),
              (resp as? HTTPURLResponse)?.statusCode == 200 else { return }
        var w = HcpWeather()
        let ok = data.withUnsafeBytes { hcp_weather_parse($0.bindMemory(to: CChar.self).baseAddress, data.count, &w) }
        guard ok != 0 else { return }
        func text<T>(_ v: T) -> String {
            var v = v
            return withUnsafePointer(to: &v) { $0.withMemoryRebound(to: CChar.self, capacity: MemoryLayout<T>.size) { String(cString: $0) } }
        }
        current = LocalWeather(
            temperatureC: w.temperature_c, humidity: Int(w.humidity_pct), windKmh: w.wind_kmh,
            windFrom: text(w.wind_from), description: text(w.description),
            dewPointC: w.has_dew_point != 0 ? w.dew_point_c : nil,
            pressureHpa: w.has_pressure != 0 ? w.pressure_hpa : nil,
            trend: w.has_pressure_change != 0 ? text(w.trend) : nil)
        fetched = Date()
        fetchedFor = locator
    }
}
