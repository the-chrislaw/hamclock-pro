import Foundation

/// The App Group shared by the Qt app and the widget extension. The Qt app writes the
/// station with src/platform/apple_widgetshare_ios.mm; the key names are a contract with
/// src/platform/apple_widgetshare.hpp (tests/widget_share_regression.cpp). Change both together.
enum AppGroup {
    static let identifier = "group.com.chrislaw.hamclockpro"

    enum Key {
        static let version = "share.version"
        static let updated = "share.updated"
        static let configured = "station.configured"
        static let callsign = "station.callsign"
        static let latitude = "station.lat"
        static let longitude = "station.lon"
        static let tracked = "satellites.tracked"
    }

    /// Shared defaults, or nil when the App Group is not available (no entitlement).
    static var defaults: UserDefaults? { UserDefaults(suiteName: identifier) }

    /// Shared container for cached feed data. Falls back to the extension's own Caches
    /// directory when the App Group is unavailable (unsigned Simulator builds).
    static var cacheDirectory: URL {
        if let group = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: identifier) {
            let dir = group.appendingPathComponent("Library/Caches/Widgets", isDirectory: true)
            try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
            return dir
        }
        return FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
    }
}

/// The operator's station as the app shared it.
struct Station: Equatable, Codable {
    var callsign: String
    var latitude: Double
    var longitude: Double
    /// Satellite short names the user tracks in the app (at most two), first is preferred.
    var tracked: [String]

    /// Reads the shared station; nil while the app has not shared a configured, valid station.
    static func read(from defaults: UserDefaults?) -> Station? {
        guard let d = defaults, d.bool(forKey: AppGroup.Key.configured),
              let lat = d.object(forKey: AppGroup.Key.latitude) as? Double,
              let lon = d.object(forKey: AppGroup.Key.longitude) as? Double,
              lat.isFinite, lon.isFinite, (-90...90).contains(lat), (-180...180).contains(lon)
        else { return nil }
        let call = (d.string(forKey: AppGroup.Key.callsign) ?? "").trimmingCharacters(in: .whitespaces)
        return Station(callsign: call, latitude: lat, longitude: lon, tracked: trackedNames(from: d))
    }

    /// Tracked names are shared even without a station.
    static func trackedNames(from defaults: UserDefaults?) -> [String] {
        let raw = defaults?.stringArray(forKey: AppGroup.Key.tracked) ?? []
        return raw.map { $0.trimmingCharacters(in: .whitespaces) }.filter { !$0.isEmpty }
    }
}
