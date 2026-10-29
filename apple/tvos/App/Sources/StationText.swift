import SwiftUI

/// Station card text, ported from src/ui/clock_text.hpp, sun_text.hpp, path_text.hpp,
/// banner_text.hpp and aux_time so the TV station card says exactly what the main app says.
enum StationText {
    private static func formatter(_ format: String, _ zone: TimeZone) -> DateFormatter {
        let f = DateFormatter()
        f.locale = Locale(identifier: "en_US_POSIX")
        f.timeZone = zone
        f.dateFormat = format
        return f
    }
    static let utc = TimeZone(identifier: "UTC")!

    /// "HH:mm:ss" or "h:mm:ss AM".
    static func clock(_ now: Date, _ zone: TimeZone, h12: Bool) -> String {
        formatter(h12 ? "h:mm:ss a" : "HH:mm:ss", zone).string(from: now)
    }

    static func shortClock(_ now: Date, _ zone: TimeZone, h12: Bool) -> String {
        formatter(h12 ? "h:mm a" : "HH:mm", zone).string(from: now)
    }

    /// Date order 0 "Thu 15 Oct 2026", 1 "Thu Oct 15 2026", 2 "2026-10-15 Thu".
    static func date(_ now: Date, _ zone: TimeZone, order: Int) -> String {
        let f = order == 1 ? "EEE MMM d yyyy" : order == 2 ? "yyyy-MM-dd EEE" : "EEE d MMM yyyy"
        return formatter(f, zone).string(from: now)
    }

    /// "Day 294 of 365 · week 43" (ISO week), as the Qt aux time's default mode.
    static func dayOfYear(_ now: Date) -> String {
        var cal = Calendar(identifier: .iso8601)
        cal.timeZone = utc
        let day = cal.ordinality(of: .day, in: .year, for: now) ?? 0
        let days = cal.range(of: .day, in: .year, for: now)?.count ?? 365
        return "Day \(day) of \(days) · week \(cal.component(.weekOfYear, from: now))"
    }

    static func zoneLabel(_ id: String, now: Date) -> String {
        guard !id.isEmpty else { return "Set a time zone in Settings" }
        guard let tz = TimeZone(identifier: id) else { return "Unknown time zone \(id)" }
        return id + " · " + (tz.abbreviation(for: now) ?? "")
    }

    /// "Local 14:05 · Tokyo (auto)" (zone_local_text).
    static func zoneLocal(_ id: String, now: Date, h12: Bool, suggested: Bool) -> String {
        guard !id.isEmpty, let tz = TimeZone(identifier: id) else { return "" }
        let city = (id.split(separator: "/").last.map(String.init) ?? id).replacingOccurrences(of: "_", with: " ")
        return "Local " + shortClock(now, tz, h12: h12) + " · " + city + (suggested ? " (auto)" : "")
    }

    static func countdown(_ seconds: Int64) -> String {
        let m = seconds < 0 ? 0 : (seconds + 30) / 60
        if m < 60 { return "in \(m)m" }
        if m < 24 * 60 { return "in \(m / 60)h \(m % 60)m" }
        return "in \(m / 1440)d \(m % 1440 / 60)h"
    }

    /// "Rise in 3h 12m · Set in 14h 5m", soonest first (sun_line).
    static func sunLine(_ now: Date, lat: Double, lon: Double) -> String {
        let t = Int64(now.timeIntervalSince1970)
        var rise: Int64 = 0, set: Int64 = 0
        let hasRise = hcp_next_sun_event(t, 0, lat, lon, &rise) != 0
        let hasSet = hcp_next_sun_event(t, 1, lat, lon, &set) != 0
        if !hasRise && !hasSet {
            var el = 0.0
            guard hcp_sun_elevation(t, lat, lon, &el) != 0 else { return "" }
            return el > 0 ? "Sun up all day" : "Sun down all day"
        }
        let r = hasRise ? "Rise " + countdown(rise - t) : ""
        let s = hasSet ? "Set " + countdown(set - t) : ""
        if !hasRise { return s }
        if !hasSet { return r }
        return rise <= set ? r + " · " + s : s + " · " + r
    }

    static func coordinates(_ lat: Double, _ lon: Double) -> String {
        String(format: "%.2f° %@, %.2f° %@", abs(lat), lat < 0 ? "S" : "N", abs(lon), lon < 0 ? "W" : "E")
    }

    static func grid(_ lat: Double, _ lon: Double) -> String {
        cString(16) { hcp_maidenhead(lat, lon, 6, $0, $1) }
    }

    /// "Short path\n8123 KM\n45° TRUE" (path_detail).
    static func path(de: (lat: Double, lon: Double), dx: (lat: Double, lon: Double), longPath: Bool, imperial: Bool) -> String {
        var p = HcpPath()
        guard hcp_path(de.lat, de.lon, dx.lat, dx.lon, &p) != 0, p.defined != 0 else { return "Set a DX target in Settings" }
        let km = longPath ? p.long_distance_km : p.distance_km
        let deg = longPath ? p.long_bearing_deg : p.short_bearing_deg
        let dist = Int((imperial ? km / 1.609344 : km).rounded())
        return "\(longPath ? "Long path" : "Short path")\n\(dist)\u{a0}\(imperial ? "MI" : "KM")\n\(Int(deg.rounded()) % 360)°\u{a0}TRUE"
    }

    // MARK: banner (banner_text.hpp)

    static func bannerModes(_ title: String) -> Int { title.trimmingCharacters(in: .whitespaces).isEmpty ? 3 : 4 }

    static func banner(mode: Int, call: String, title: String) -> String {
        let n = bannerModes(title)
        switch ((mode % n) + n) % n {
        case 1: return "ON AIR"
        case 2: return call.isEmpty ? "73" : "73 DE " + call
        case 3: return String(title.trimmingCharacters(in: .whitespaces).prefix(24))
        default: return call
        }
    }

    /// nil = theme accent. 5 is the rainbow, turning once per 12 s.
    static func bannerColor(style: Int, now: Date) -> Color? {
        switch style {
        case 1: return Theme.hex(0xffb000)
        case 2: return Theme.hex(0x3ddc84)
        case 3: return Theme.hex(0xff4d4d)
        case 4: return Theme.hex(0xffffff)
        case 5:
            let secs = Int(now.timeIntervalSince1970)
            return Color(hue: Double(secs % 12) / 12, saturation: 0.9, brightness: 0.95)
        default: return nil
        }
    }

    static func bannerBackground(_ style: Int) -> Color? {
        switch style {
        case 1: return Theme.hex(0x000000)
        case 2: return Theme.hex(0x12306b)
        case 3: return Theme.hex(0x6b1a1a)
        case 4: return Theme.hex(0xffffff)
        default: return nil
        }
    }
}
