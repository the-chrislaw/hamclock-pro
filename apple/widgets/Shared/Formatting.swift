import Foundation

/// How much to trust what a widget shows. Never present old data as live.
enum Freshness: String, Codable {
    case live, stale, offline, noData

    /// Widgets refresh about every 30 minutes, so data up to an hour old is still "live".
    static let staleAfter: TimeInterval = 3600
    static let offlineAfter: TimeInterval = 6 * 3600

    /// `fetched` is the last successful fetch; `lastAttemptFailed` is whether the newest fetch failed.
    static func evaluate(fetched: Date?, lastAttemptFailed: Bool, now: Date) -> Freshness {
        guard let fetched else { return .noData }
        let age = now.timeIntervalSince(fetched)
        if age > offlineAfter { return .offline }
        if lastAttemptFailed { return age > staleAfter ? .offline : .stale }
        return age > staleAfter ? .stale : .live
    }

    /// Short badge text; nil while live.
    var badge: String? {
        switch self {
        case .live: return nil
        case .stale: return "Stale"
        case .offline: return "Offline"
        case .noData: return "No data"
        }
    }
}

/// Traffic-light severity. Every coloured value also carries a word, so meaning
/// survives the tinted/monochrome modes (Lock Screen, StandBy night, CarPlay).
enum Severity: Int, Codable { case calm, elevated, severe }

/// Two adjacent bands shown as one row, best of the pair: the widget has room for
/// four large rows, not eight (the app's Bands card has the full table).
struct BandGroup: Equatable {
    var label: String       // "20-17"
    var condition: BandCondition
}

enum Format {
    /// "as of 14:05" in the given zone, for stale/offline footers.
    static func asOf(_ date: Date, zone: TimeZone = .current) -> String {
        "as of " + clock(date, zone: zone)
    }

    static func clock(_ date: Date, zone: TimeZone = .current) -> String {
        let f = DateFormatter()
        f.locale = Locale(identifier: "en_US_POSIX")
        f.timeZone = zone
        f.dateFormat = "HH:mm"
        return f.string(from: date)
    }

    /// 16-point compass name for an azimuth in degrees.
    static func compass(_ degrees: Double) -> String {
        guard degrees.isFinite else { return "–" }
        let points = ["N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"]
        let d = degrees.truncatingRemainder(dividingBy: 360)
        let normalized = d < 0 ? d + 360 : d
        return points[Int((normalized / 22.5).rounded()) % 16]
    }

    static func kp(_ value: Double) -> String { String(format: "%.1f", value) }
    static func sfi(_ value: Double) -> String { String(Int(value.rounded())) }

    /// NOAA G-scale thresholds: Kp 5+ is a storm, 4 is active.
    static func kpSeverity(_ value: Double) -> Severity {
        value >= 5 ? .severe : value >= 4 ? .elevated : .calm
    }

    static func kpWord(_ value: Double) -> String {
        value >= 5 ? "Storm" : value >= 4 ? "Active" : value >= 3 ? "Unsettled" : "Quiet"
    }

    /// M flares cause short HF fadeouts on the day side, X flares strong ones (R2/R3+).
    static func xraySeverity(_ cls: String) -> Severity {
        switch cls.first {
        case "X": return .severe
        case "M": return .elevated
        default: return .calm
        }
    }

    /// SFI from roughly 70 (solar minimum) to 200+: higher opens the upper HF bands.
    static func sfiSeverity(_ value: Double) -> Severity {
        value >= 120 ? .calm : value >= 90 ? .elevated : .severe
    }

    static let bandPairs: [(label: String, bands: [String])] = [
        ("80-40", ["80m", "40m"]), ("30-20", ["30m", "20m"]),
        ("17-15", ["17m", "15m"]), ("12-10", ["12m", "10m"]),
    ]

    static func bandGroups(_ outlook: [BandOutlook], day: Bool) -> [BandGroup] {
        bandPairs.compactMap { pair in
            let members = outlook.filter { pair.bands.contains($0.band) }
            guard let best = members.map({ day ? $0.day : $0.night }).max() else { return nil }
            return BandGroup(label: pair.label, condition: best)
        }
    }

    /// Bands rated Good (or, failing that, Fair) for the period, as "20 17 15".
    static func openBands(_ outlook: [BandOutlook], day: Bool) -> (condition: BandCondition, list: String)? {
        for wanted in [BandCondition.good, .fair] {
            let names = outlook.filter { (day ? $0.day : $0.night) == wanted }
                .map { $0.band.replacingOccurrences(of: "m", with: "") }
            if !names.isEmpty { return (wanted, names.joined(separator: " ")) }
        }
        return nil
    }

    /// Daytime at the station when the Sun is above the horizon; UTC 06-18 without a station.
    static func isDay(at date: Date, station: Station?) -> Bool {
        if let s = station, let el = Core.sunElevation(at: date, latitude: s.latitude, longitude: s.longitude) {
            return el > 0
        }
        var cal = Calendar(identifier: .gregorian)
        cal.timeZone = TimeZone(identifier: "UTC")!
        let h = cal.component(.hour, from: date)
        return (6..<18).contains(h)
    }

    /// TLEs older than this drift noticeably; the app uses the same two-week rule.
    static let tleWarnAge: TimeInterval = 14 * 86400
}

/// What the satellite widget says about one pass, relative to `now`.
enum PassPhase: Equatable {
    case upNow       // AOS has passed, LOS has not
    case upcoming    // AOS in the future
    case over

    static func of(_ pass: SatellitePass, at now: Date) -> PassPhase {
        if now >= pass.los { return .over }
        return now >= pass.aos ? .upNow : .upcoming
    }
}
