import SwiftUI

/// Open-Meteo conditions at DE (WeatherCard.qml): big temperature, description, wind, humidity, pressure.
struct WeatherWidget: View {
    @EnvironmentObject var wx: LocalWeatherStore
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let fetchState = FeedStatus(fetched: wx.fetched, failed: wx.failed, tried: wx.failed || wx.fetched != nil)
            .state(now, FreshnessPolicy(interval: 900, staleAfter: 3600, offlineAfter: 3 * 3600))
        let state: WidgetState = settings.configured ? fetchState : .unconfigured
        Card(title: "Weather", chooser: true, state: state, status: "Open-Meteo",
             age: Age.text(wx.fetched, now: now), focused: focused) {
            if !settings.configured {
                Note(text: "Set your location in Settings.")
            } else if let c = wx.current, state != .offline {
                Text(temp(c.temperatureC)).font(.system(size: Theme.fontValue * 1.2, weight: .semibold)).foregroundStyle(theme.ink)
                Text(c.description).font(.system(size: Theme.fontBody)).foregroundStyle(theme.ink)
                Text(wind(c) + " · humidity \(c.humidity)%").font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                    .lineLimit(1).minimumScaleFactor(0.7)
                if let p = c.pressureHpa {
                    Text(pressure(p) + (c.trend.map { ", " + $0.lowercased() } ?? ""))
                        .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                }
            } else {
                Note(text: state == .loading ? "Loading…" : "No data")
            }
        }
    }

    private func temp(_ c: Double) -> String {
        settings.imperialUnits ? String(format: "%.0f °F", c * 9 / 5 + 32) : String(format: "%.1f °C", c)
    }
    private func wind(_ c: LocalWeather) -> String {
        settings.imperialUnits ? String(format: "Wind %.0f mph %@", c.windKmh / 1.609344, c.windFrom)
            : String(format: "Wind %.0f km/h %@", c.windKmh, c.windFrom)
    }
    private func pressure(_ hpa: Double) -> String {
        settings.imperialUnits ? String(format: "%.2f inHg", hpa * 0.02953) : String(format: "%.0f hPa", hpa)
    }
}

struct QuakesWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let list = feeds.quakes.value ?? []
        let top = list.max { $0.magnitude < $1.magnitude }
        Card(title: "Earthquakes", chooser: true, state: feeds.quakes.state(now), status: "USGS M2.5+ 24 h",
             age: Age.text(feeds.quakes.status.fetched, now: now), focused: focused) {
            if let top {
                Text(String(format: "M%.1f %@", top.magnitude, SpaceText.quakeClass(top.magnitude)))
                    .font(.system(size: Theme.fontValue * 0.85, weight: .semibold)).foregroundStyle(top.magnitude >= 6 ? theme.bad : theme.ink)
                Text(top.place).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.ink).lineLimit(1)
                ForEach(list.prefix(4)) { q in
                    HStack {
                        Text(String(format: "M%.1f", q.magnitude)).frame(width: 90, alignment: .leading)
                        Text(q.place).lineLimit(1)
                        Spacer(minLength: 4)
                        Text(Age.text(q.time, now: now)).fixedSize()
                    }.font(.system(size: Theme.fontCaption - 3)).foregroundStyle(theme.secondary)
                }
            } else { Note(text: feeds.quakes.value == nil ? "No data" : "No M2.5+ quakes in the past day.") }
        }
    }
}

struct StormsWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        Card(title: "Tropical storms", chooser: true, state: feeds.storms.state(now), status: "NOAA NHC",
             age: Age.text(feeds.storms.status.fetched, now: now), focused: focused) {
            if let list = feeds.storms.value {
                if list.isEmpty { Note(text: "No active storms.") }
                ForEach(list.prefix(3)) { s in
                    VStack(alignment: .leading, spacing: 0) {
                        Text("\(s.name) · \(s.classification)\(s.category > 0 ? " cat \(s.category)" : "")")
                            .font(.system(size: Theme.fontBody, weight: .semibold)).foregroundStyle(s.category > 0 ? theme.bad : theme.ink)
                        Text("\(s.windKt) kt" + (s.pressureMb > 0 ? " · \(s.pressureMb) mb" : "")
                             + (s.speedKt >= 0 && !s.heading.isEmpty ? " · moving \(s.heading) \(s.speedKt) kt" : ""))
                            .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).lineLimit(1)
                            .minimumScaleFactor(0.75)
                    }
                }
            } else { Note(text: "No data") }
        }
    }
}

struct FiresWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let list = feeds.fires.value ?? []
        Card(title: "Active fires", chooser: true, state: feeds.fires.state(now), status: "NASA FIRMS 24 h",
             age: Age.text(feeds.fires.status.fetched, now: now), focused: focused) {
            if feeds.fires.value == nil { Note(text: "No data") } else {
                Metric(label: "Detections", value: (list.count >= Feeds.fireCap ? "≥ " : "") + "\(list.count)",
                       caption: "\(list.filter { $0.high_confidence != 0 }.count) high confidence")
                ForEach(Array(list.prefix(3).enumerated()), id: \.offset) { _, f in
                    Text(String(format: "%.0f MW at %.2f°, %.2f°", f.frp_mw, f.lat, f.lon))
                        .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                }
            }
        }
    }
}

/// POTA/SOTA/WWFF activators (OtaCard.qml), newest first.
struct OtaWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let spots = feeds.otaSpots(now)
        let all = [feeds.pota, feeds.sota, feeds.wwff]
        let states = all.map { $0.state(now) }
        // Spots come only from Live/Stale feeds, so the card is as good as its best feed;
        // the status line names the feeds that are out.
        let state = [WidgetState.live, .stale, .loading].first { states.contains($0) } ?? .offline
        // The badge is the best feed's state, so the age is the newest fetch; feeds that are not
        // Live are named in the status line ("WWFF stale"), so a Live badge never hides an old feed.
        let lagging = zip(all, states).filter { $0.1 == .stale || $0.1 == .offline }
            .map { "\($0.0.source) \($0.1 == .stale ? "stale" : "offline")" }
        Card(title: "On the air", chooser: true, state: state,
             status: lagging.isEmpty || lagging.count == 3 ? "POTA · SOTA · WWFF" : lagging.joined(separator: " · "),
             age: Age.text(all.compactMap(\.status.fetched).max(), now: now), focused: focused) {
            if spots.isEmpty { Note(text: state == .loading ? "Loading…" : "No spots") }
            Grid(alignment: .leading, horizontalSpacing: 14, verticalSpacing: 3) {
                ForEach(spots.prefix(7)) { s in
                    GridRow {
                        Text(s.call).foregroundStyle(theme.ink).fontWeight(.semibold).lineLimit(1)
                        Text(String(format: "%.3f", s.freqKhz / 1000)).foregroundStyle(theme.ink)
                        Text(s.mode).foregroundStyle(theme.secondary)
                        Text(s.reference).foregroundStyle(theme.secondary).lineLimit(1)
                    }.font(.system(size: Theme.fontCaption - 2))
                }
            }
        }
    }
}

/// A pane whose saved widget has no TV data source (e.g. set on the iPad), or an empty pane.
struct PlaceholderWidget: View {
    let title: String
    let text: String
    let focused: Bool
    var body: some View {
        Card(title: title, chooser: true, focused: focused) { Note(text: text) }
    }
}

/// The widget for one pane id.
struct PaneWidget: View {
    let id: String
    let now: Date
    let focused: Bool
    var body: some View {
        switch id {
        case "spaceweather": SpaceWeatherWidget(now: now, focused: focused)
        case "solarwind": SolarWindWidget(now: now, focused: focused)
        case "bands": BandsWidget(now: now, focused: focused)
        case "solarscales": ScalesWidget(now: now, focused: focused)
        case "solardst": DstWidget(now: now, focused: focused)
        case "solaralerts": AlertsWidget(now: now, focused: focused)
        case "solaraurora": AuroraWidget(now: now, focused: focused)
        case "solardrap": DrapWidget(now: now, focused: focused)
        case "solarimage": SolarImageWidget(now: now, focused: focused)
        case "sunmoon": SunMoonWidget(now: now, focused: focused)
        case "satellites": SatellitesWidget(now: now, focused: focused)
        case "planets": PlanetsWidget(now: now, focused: focused)
        case "eclipse": EclipseWidget(now: now, focused: focused)
        case "launches": LaunchesWidget(now: now, focused: focused)
        case "weather": WeatherWidget(now: now, focused: focused)
        case "quakes": QuakesWidget(now: now, focused: focused)
        case "storms": StormsWidget(now: now, focused: focused)
        case "fires": FiresWidget(now: now, focused: focused)
        case "ota": OtaWidget(now: now, focused: focused)
        case "none": PlaceholderWidget(title: "Empty pane", text: "Press Select to choose a widget.", focused: focused)
        default:
            PlaceholderWidget(title: WidgetCatalog.info(id)?.title ?? id,
                              text: "Not available on Apple TV yet. Press Select to choose another widget.", focused: focused)
        }
    }
}
