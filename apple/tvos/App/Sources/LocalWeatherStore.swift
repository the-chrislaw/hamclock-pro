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

    /// Same query as the Qt app's weather_parse::request_url (coordinates to 2 decimals).
    static func url(lat: Double, lon: Double) -> URL? {
        URL(string: String(format: "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f"
            + "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,wind_direction_10m,weather_code,is_day,pressure_msl,cloud_cover"
            + "&hourly=pressure_msl&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&past_hours=3&forecast_hours=1&timezone=auto",
            lat, lon))
    }

    @Published var failed = false
    /// Station position the loop fetches for; set by the app when settings change.
    func run(position: (lat: Double, lon: Double)?) async {
        let key = position.map { String(format: "%.2f,%.2f", $0.lat, $0.lon) } ?? ""
        while !Task.isCancelled {
            await refresh(position, locator: key)
            try? await Task.sleep(nanoseconds: UInt64(interval * 1e9))
        }
    }

    private func refresh(_ pos: (lat: Double, lon: Double)?, locator: String) async {
        guard let pos, let url = Self.url(lat: pos.lat, lon: pos.lon) else { return }
        if fetchedFor != locator { current = nil; fetched = nil; failed = false } // never show another place's weather
        // A failed fetch keeps the previous reading; its age drives Stale/Offline.
        guard let data = await Net.get(url) else { if !Task.isCancelled { failed = true }; return }
        var w = HcpWeather()
        let ok = data.withUnsafeBytes { hcp_weather_parse($0.bindMemory(to: CChar.self).baseAddress, data.count, &w) }
        guard ok != 0 else { failed = true; return }
        current = LocalWeather(
            temperatureC: w.temperature_c, humidity: Int(w.humidity_pct), windKmh: w.wind_kmh,
            windFrom: cText(w.wind_from), description: cText(w.description),
            dewPointC: w.has_dew_point != 0 ? w.dew_point_c : nil,
            pressureHpa: w.has_pressure != 0 ? w.pressure_hpa : nil,
            trend: w.has_pressure_change != 0 ? cText(w.trend) : nil)
        fetched = Date()
        fetchedFor = locator
        failed = false
    }
}
