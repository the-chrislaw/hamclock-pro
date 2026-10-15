import SwiftUI

struct DashboardView: View {
    @EnvironmentObject var weather: SpaceWeatherStore
    @EnvironmentObject var quakes: QuakeStore

    var body: some View {
        TimelineView(.periodic(from: .now, by: 1)) { ctx in
            VStack(alignment: .leading, spacing: 40) {
                ClockBar(now: ctx.date)
                HStack(alignment: .top, spacing: 40) {
                    Card(title: "Kp index", sample: weather.kp, now: ctx.date,
                         big: { String(format: "%.1f", $0) },
                         detail: { SpaceWeatherStore.kpDescription($0) })
                    Card(title: "Solar flux", sample: weather.flux, now: ctx.date,
                         big: { String(format: "%.0f sfu", $0) }, detail: { _ in "10.7 cm" })
                    Card(title: "X-ray", sample: weather.xray, now: ctx.date,
                         big: { SpaceWeatherStore.xrayClass($0) }, detail: { _ in "GOES 0.1–0.8 nm" })
                }
                HStack(alignment: .top, spacing: 40) {
                    Card(title: "Solar wind", sample: weather.windSpeed, now: ctx.date,
                         big: { String(format: "%.0f km/s", $0) },
                         detail: { SpaceWeatherStore.windSpeedDescription($0) })
                    Card(title: "IMF Bz (GSM)", sample: weather.bz, now: ctx.date,
                         big: { String(format: "%+.1f nT", $0) },
                         detail: { SpaceWeatherStore.bzDescription($0) })
                }
                HStack(alignment: .top, spacing: 40) {
                    SunCard(now: ctx.date)
                    StationSunCard(now: ctx.date)
                }
                SatelliteCard(now: ctx.date)
                QuakeCard(now: ctx.date)
                Spacer()
                Text("Data: NOAA Space Weather Prediction Center; USGS Earthquake Hazards Program; CelesTrak")
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            }
            .padding(.horizontal, 90).padding(.vertical, 60) // tvOS title-safe area
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
            .background(Color.black)
        }
    }
}

private struct ClockBar: View {
    let now: Date
    var body: some View {
        HStack(alignment: .firstTextBaseline, spacing: 40) {
            Text(now.formatted(Date.FormatStyle(timeZone: TimeZone(identifier: "UTC")!)
                .hour(.twoDigits(amPM: .omitted)).minute().second()) + " UTC")
                .font(.system(size: 96, weight: .semibold, design: .monospaced))
            Text(now.formatted(Date.FormatStyle(timeZone: TimeZone(identifier: "UTC")!)
                .weekday(.wide).day().month(.wide).year()))
                .font(.system(size: 48)).foregroundStyle(.secondary)
        }
    }
}

private struct Card: View {
    let title: String
    let sample: Sample?
    let now: Date
    let big: (Double) -> String
    let detail: (Double) -> String

    var body: some View {
        let state = SpaceWeatherStore.freshness(sample, now: now)
        VStack(alignment: .leading, spacing: 12) {
            Text(title).font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if let s = sample {
                Text(big(s.value)).font(.system(size: 96, weight: .bold, design: .rounded))
                Text(detail(s.value)).font(.system(size: 29))
                Text("as of " + s.observed.formatted(date: .omitted, time: .shortened))
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            } else {
                Text("No data").font(.system(size: 48)).foregroundStyle(.secondary)
            }
            if state != .live {
                Text(state == .stale ? "STALE" : "OFFLINE")
                    .font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}

/// Computed offline by the shared C++ solar_position code from the device clock:
/// no network involved, so there is no Stale/Offline state (it is exact for `now`).
private struct SunCard: View {
    let now: Date
    var body: some View {
        var lat = 0.0, lon = 0.0
        let t = Int64(now.timeIntervalSince1970)
        let ok = hcp_subsolar_point(t, &lat, &lon) != 0
        VStack(alignment: .leading, spacing: 12) {
            Text("Sun").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if ok {
                Text(String(format: "Overhead %.1f°%@ %.1f°%@",
                            abs(lat), lat >= 0 ? "N" : "S", abs(lon), lon >= 0 ? "E" : "W"))
                    .font(.system(size: 64, weight: .bold, design: .rounded))
                Text("Subsolar point, computed on device from the clock")
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            } else {
                Text("Unavailable").font(.system(size: 48)).foregroundStyle(.secondary)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}

/// Sun elevation at the operator's locator, computed on device (no network, never stale).
private struct StationSunCard: View {
    @EnvironmentObject var station: StationStore
    let now: Date

    /// Next rise (0) / set (1) as UTC HH:mm, computed on device; "none" when the body does not rise/set soon.
    static func event(_ kind: Int32, _ t: Int64, _ p: (lat: Double, lon: Double), sun: Bool) -> String {
        var out: Int64 = 0
        let ok = sun ? hcp_next_sun_event(t, kind, p.lat, p.lon, &out) : hcp_next_moon_event(t, kind, p.lat, p.lon, &out)
        guard ok != 0 else { return "none" }
        return Date(timeIntervalSince1970: TimeInterval(out)).formatted(
            Date.FormatStyle(timeZone: TimeZone(identifier: "UTC")!).hour(.twoDigits(amPM: .omitted)).minute()) + "Z"
    }

    static func moon(_ t: Int64) -> String {
        var buf = [CChar](repeating: 0, count: 32)
        var ill = 0.0
        guard hcp_moon_phase(t, &buf, buf.count, &ill) != 0 else { return "" }
        return String(format: "Moon: %@, %.0f%% lit", String(cString: buf), ill * 100)
    }
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Sun at your station").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if let p = station.position {
                var el = 0.0
                if hcp_sun_elevation(Int64(now.timeIntervalSince1970), p.lat, p.lon, &el) != 0 {
                    Text(String(format: "%+.1f°", el))
                        .font(.system(size: 64, weight: .bold, design: .rounded))
                    Text(el > 0 ? "Daylight" : (el > -6 ? "Civil twilight" : "Night"))
                        .font(.system(size: 29))
                    let t = Int64(now.timeIntervalSince1970)
                    Text("Sunrise " + Self.event(0, t, p, sun: true) + "  Sunset " + Self.event(1, t, p, sun: true))
                        .font(.system(size: 29))
                    Text("Moonrise " + Self.event(0, t, p, sun: false) + "  Moonset " + Self.event(1, t, p, sun: false))
                        .font(.system(size: 29))
                    Text(Self.moon(t)).font(.system(size: 29)).foregroundStyle(.secondary)
                }
            } else {
                Text("Enter your locator").font(.system(size: 48)).foregroundStyle(.secondary)
            }
            TextField("Locator, e.g. JN58td", text: $station.locator)
                .textInputAutocapitalization(.never).autocorrectionDisabled()
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
    }
}

/// USGS M2.5+ past day: strongest quake and the five most recent, with the same Stale/Offline rule.
private struct QuakeCard: View {
    @EnvironmentObject var quakes: QuakeStore
    let now: Date
    var body: some View {
        let state = SpaceWeatherStore.freshness(quakes.fetched.map { Sample(value: 0, observed: $0, fetched: $0) }, now: now)
        VStack(alignment: .leading, spacing: 12) {
            Text("Earthquakes M2.5+, past day").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if let top = quakes.strongest {
                Text(String(format: "M%.1f %@", top.magnitude, QuakeStore.magnitudeClass(top.magnitude)))
                    .font(.system(size: 64, weight: .bold, design: .rounded))
                Text(top.place).font(.system(size: 29))
                ForEach(quakes.recent) { q in
                    Text(String(format: "M%.1f  ", q.magnitude) + q.place)
                        .font(.system(size: 29)).foregroundStyle(.secondary).lineLimit(1)
                }
            } else {
                Text("No data").font(.system(size: 48)).foregroundStyle(.secondary)
            }
            if state != .live {
                Text(state == .stale ? "STALE" : "OFFLINE")
                    .font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}

/// Amateur satellites currently above the horizon at the station, from CelesTrak elements
/// propagated on device. Labelled by element age (stale after 14 days) and by fetch freshness.
private struct SatelliteCard: View {
    @EnvironmentObject var sats: SatelliteStore
    @EnvironmentObject var station: StationStore
    let now: Date
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("Amateur satellites overhead").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if sats.tle != nil, let p = station.position {
                let up = sats.visibleCached(at: now, lat: p.lat, lon: p.lon)
                if up.isEmpty {
                    Text("None above the horizon").font(.system(size: 48)).foregroundStyle(.secondary)
                }
                ForEach(up) { s in
                    Text(String(format: "%@  az %.0f°  el %.0f°  %.0f km", s.name, s.azimuth, s.elevation, s.rangeKm)
                         + (s.sunlit ? "" : "  eclipsed"))
                        .font(.system(size: 29)).lineLimit(1)
                }
                if let oldest = up.map({ SatelliteStore.tleAgeDays($0, now: now) }).max(), oldest > 14 {
                    Text("STALE ELEMENTS").font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
                }
                if let f = sats.fetched, now.timeIntervalSince(f) > 86400 * 2 {
                    Text("OFFLINE, using cached elements").font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
                }
            } else if station.position == nil {
                Text("Enter your locator").font(.system(size: 48)).foregroundStyle(.secondary)
            } else {
                Text("No data").font(.system(size: 48)).foregroundStyle(.secondary)
                Text("OFFLINE").font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}
