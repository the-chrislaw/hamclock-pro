import SwiftUI

private func utcClock(_ unix: Int64) -> String { Age.utcClock(Date(timeIntervalSince1970: TimeInterval(unix))) }

/// Sun and Moon for DE and DX (SunMoonCard.qml), computed on device: no network, never stale.
struct SunMoonWidget: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        Card(title: "Sun & Moon", chooser: true, state: settings.configured ? .live : .unconfigured,
             status: "Computed on this device", focused: focused) {
            if let de = settings.de {
                let dx = settings.dx ?? de
                let t = Int64(now.timeIntervalSince1970)
                Grid(alignment: .leading, horizontalSpacing: 22, verticalSpacing: 6) {
                    GridRow {
                        Text("")
                        Text("DE").foregroundStyle(theme.secondary)
                        Text("DX").foregroundStyle(theme.secondary)
                    }.font(.system(size: Theme.fontCaption, weight: .semibold))
                    row("Sunrise", event(t, 0, de, sun: true), event(t, 0, dx, sun: true))
                    row("Sunset", event(t, 1, de, sun: true), event(t, 1, dx, sun: true))
                    row("Moonrise", event(t, 0, de, sun: false), event(t, 0, dx, sun: false))
                    row("Moonset", event(t, 1, de, sun: false), event(t, 1, dx, sun: false))
                }
                Text(moon(t)).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.ink)
            } else {
                Note(text: "Set your location in Settings.")
            }
        }
    }

    private func row(_ label: String, _ a: String, _ b: String) -> some View {
        GridRow {
            Text(label).foregroundStyle(theme.secondary)
            Text(a).foregroundStyle(theme.ink)
            Text(b).foregroundStyle(theme.ink)
        }.font(.system(size: Theme.fontCaption))
    }

    private func event(_ t: Int64, _ kind: Int32, _ p: (lat: Double, lon: Double), sun: Bool) -> String {
        var out: Int64 = 0
        let ok = sun ? hcp_next_sun_event(t, kind, p.lat, p.lon, &out) : hcp_next_moon_event(t, kind, p.lat, p.lon, &out)
        return ok != 0 ? utcClock(out) : "—"
    }

    private func moon(_ t: Int64) -> String {
        var ill = 0.0
        let name = cString(32) { hcp_moon_phase(t, $0, $1, &ill) }
        return name.isEmpty ? "" : String(format: "Moon: %@ · %.0f%% lit", name, ill * 100)
    }
}

/// Amateur satellites above the horizon at DE, propagated on device (SGP4) from CelesTrak or
/// AMSAT elements. Elements older than 14 days are labelled Stale.
struct SatellitesWidget: View {
    @EnvironmentObject var sats: SatelliteStore
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let state: WidgetState = !settings.configured ? .unconfigured
            : sats.tle == nil ? (sats.failedOnce ? .offline : .loading)
            : (sats.fetched.map { now.timeIntervalSince($0) > 86400 * 2 } ?? true) ? .stale : .live
        Card(title: "Satellites", chooser: true, state: state, status: "\(sats.source) elements, SGP4 on device",
             age: Age.text(sats.fetched, now: now), focused: focused) {
            if let de = settings.de, sats.tle != nil {
                let up = sats.visibleCached(at: now, lat: de.lat, lon: de.lon, limit: 6)
                if up.isEmpty { Note(text: "None above the horizon now.") }
                Grid(alignment: .leading, horizontalSpacing: 18, verticalSpacing: 4) {
                    ForEach(up) { s in
                        GridRow {
                            Text(s.name).foregroundStyle(theme.ink).lineLimit(1)
                            Text(String(format: "%.0f°", s.azimuth)).foregroundStyle(theme.secondary)
                            Text(String(format: "el %.0f°", s.elevation)).foregroundStyle(s.elevation > 30 ? theme.good : theme.ink)
                        }.font(.system(size: Theme.fontCaption))
                    }
                }
            } else {
                Note(text: settings.configured ? "Loading elements…" : "Set your location in Settings.")
            }
        }
    }
}

struct PlanetsWidget: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        Card(title: "Planets", chooser: true, state: settings.configured ? .live : .unconfigured,
             status: "Computed on this device", focused: focused) {
            if let de = settings.de {
                let t = Int64(now.timeIntervalSince1970)
                Grid(alignment: .leading, horizontalSpacing: 20, verticalSpacing: 4) {
                    ForEach(0..<5, id: \.self) { i in
                        var p = HcpPlanet()
                        if hcp_planet(Int32(i), t, de.lat, de.lon, &p) != 0 {
                            GridRow {
                                Text(cText(p.name)).foregroundStyle(theme.ink).fontWeight(.semibold)
                                Text(p.el_deg > 0 ? String(format: "up %.0f°", p.el_deg) : "below")
                                    .foregroundStyle(p.el_deg > 0 ? theme.good : theme.secondary)
                                Text(next(i, t, de, up: p.el_deg > 0)).foregroundStyle(theme.secondary).lineLimit(1)
                            }.font(.system(size: Theme.fontCaption))
                        }
                    }
                }
            } else { Note(text: "Set your location in Settings.") }
        }
    }

    private func next(_ i: Int, _ t: Int64, _ de: (lat: Double, lon: Double), up: Bool) -> String {
        var e: Int64 = 0
        guard hcp_next_planet_event(Int32(i), t, up ? 1 : 0, de.lat, de.lon, &e) != 0 else { return "" }
        return (up ? "set " : "rise ") + utcClock(e)
    }
}

struct EclipseWidget: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let e = eclipse()
        Card(title: "Next eclipse", chooser: true, state: .live, status: "Computed on this device", focused: focused) {
            if let e {
                let when = Date(timeIntervalSince1970: TimeInterval(e.greatest_s))
                Metric(label: "Solar eclipse · " + cText(e.type),
                       value: StationText.date(when, StationText.utc, order: settings.dateOrder),
                       caption: "Greatest " + utcClock(e.greatest_s) + " · in \(max(0, Int(when.timeIntervalSince(now) / 86400))) days",
                       size: Theme.fontValue * 0.75)
                Text(local(e)).font(.system(size: Theme.fontCaption)).foregroundStyle(e.visibility == 2 ? theme.good : theme.secondary)
                    .fixedSize(horizontal: false, vertical: true)
            } else { Note(text: "Unavailable") }
        }
    }

    /// The search walks lunations and samples the station's sky, so it is cached per hour and place.
    @MainActor private static var cache: [String: HcpEclipse] = [:]
    private func eclipse() -> HcpEclipse? {
        let de = settings.de
        let hour = Int64(now.timeIntervalSince1970) / 3600
        let key = "\(hour)/\(de?.lat ?? 999)/\(de?.lon ?? 999)"
        if let hit = Self.cache[key] { return hit.valid != 0 ? hit : nil }
        var e = HcpEclipse()
        _ = hcp_next_solar_eclipse(hour * 3600, de?.lat ?? .nan, de?.lon ?? .nan, &e)
        Self.cache = [key: e]
        return e.valid != 0 ? e : nil
    }

    private func local(_ e: HcpEclipse) -> String {
        guard settings.configured else { return "Set your location to see whether it is visible from DE." }
        switch e.visibility {
        case 2: return String(format: "Visible from DE: maximum %@, %.0f%% of the Sun covered", utcClock(e.local_max_s), min(100, e.local_magnitude * 100))
        case 1: return "Marginal from DE (Sun low or barely covered)"
        default: return "Not visible from DE"
        }
    }
}

struct LaunchesWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let nowMs = Int64(now.timeIntervalSince1970 * 1000)
        let list = (feeds.launches.value ?? []).filter { $0.net_ms >= nowMs - 2 * 3_600_000 }.prefix(4)
        Card(title: "Launches", chooser: true, state: feeds.launches.state(now), status: "Launch Library 2",
             age: Age.text(feeds.launches.status.fetched, now: now), focused: focused) {
            if list.isEmpty { Note(text: feeds.launches.value == nil ? "No data" : "No upcoming launches listed.") }
            ForEach(Array(list.enumerated()), id: \.offset) { _, l in
                VStack(alignment: .leading, spacing: 0) {
                    HStack {
                        Text(cText(l.name)).foregroundStyle(theme.ink).lineLimit(1)
                        Spacer(minLength: 8)
                        Text(cString(32) { hcp_launch_countdown(l.net_ms, nowMs, $0, $1) }).foregroundStyle(theme.accent).fixedSize()
                    }.font(.system(size: Theme.fontCaption, weight: .medium))
                    Text(cText(l.provider) + " · " + cText(l.status) + (cText(l.hint).isEmpty ? "" : " · " + cText(l.hint)))
                        .font(.system(size: Theme.fontCaption - 3)).foregroundStyle(theme.secondary).lineLimit(1)
                }
            }
        }
    }
}
