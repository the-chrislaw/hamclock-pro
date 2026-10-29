import SwiftUI

/// Worst of several feed states (a card is only Live when every input is).
func combined(_ states: [WidgetState]) -> WidgetState {
    let order: [WidgetState] = [.error, .offline, .stale, .loading, .unconfigured, .live]
    return order.first { states.contains($0) } ?? .live
}

/// Simple line plot of a series (Sparkline.qml).
struct Sparkline: View {
    @Environment(\.theme) private var theme
    let values: [Double]
    var log = false
    var body: some View {
        Canvas { ctx, size in
            let v = values.map { log ? Foundation.log10(max($0, 1e-9)) : $0 }
            guard v.count > 1, let lo = v.min(), let hi = v.max() else { return }
            let span = max(hi - lo, 1e-9)
            var p = Path()
            for (i, y) in v.enumerated() {
                let pt = CGPoint(x: size.width * CGFloat(i) / CGFloat(v.count - 1),
                                 y: size.height * (1 - CGFloat((y - lo) / span)) * 0.9 + size.height * 0.05)
                if i == 0 { p.move(to: pt) } else { p.addLine(to: pt) }
            }
            ctx.stroke(p, with: .color(theme.route), lineWidth: 3)
            var base = Path()
            base.move(to: CGPoint(x: 0, y: size.height - 1)); base.addLine(to: CGPoint(x: size.width, y: size.height - 1))
            ctx.stroke(base, with: .color(theme.divider), lineWidth: 1.5)
        }
    }
}

struct SpaceWeatherWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let kp = feeds.kp.value?.last, flux = feeds.flux.value, xray = feeds.xray.value?.last
        let state = combined([feeds.kp.state(now), feeds.flux.state(now), feeds.xray.state(now)])
        let oldest = [feeds.kp.status.fetched, feeds.flux.status.fetched, feeds.xray.status.fetched].compactMap { $0 }.min()
        Card(title: "Space weather", chooser: true, state: state, status: "NOAA SWPC",
             age: Age.text(oldest, now: now), focused: focused) {
            VStack(alignment: .leading, spacing: 10) {
                row("KP", kp.map { String(format: "%.1f ", $0.value) + SpaceText.kpWord($0.value) })
                row("SFI", flux.map { String(format: "%.0f", $0.value) })
                row("X-RAY", xray.map { SpaceText.xrayClass($0.value) })
                Spacer(minLength: 0)
                if let series = feeds.xray.value, series.count > 2 {
                    Sparkline(values: series.map(\.value), log: true).frame(height: 54)
                }
            }
        }
    }

    private func row(_ label: String, _ value: String?) -> some View {
        HStack(alignment: .firstTextBaseline, spacing: 20) {
            Text(label).font(.system(size: Theme.fontCaption, weight: .semibold)).foregroundStyle(theme.secondary)
                .frame(width: 120, alignment: .leading)
            Text(value ?? "—").font(.system(size: Theme.fontValue * 0.8, weight: .semibold))
                .foregroundStyle(value == nil ? theme.secondary : theme.ink).lineLimit(1).minimumScaleFactor(0.6)
        }
    }
}

struct SolarWindWidget: View {
    @EnvironmentObject var feeds: Feeds
    let now: Date
    let focused: Bool
    var body: some View {
        let speed = feeds.wind.value?[0], bz = feeds.mag.value?[1]
        let state = combined([feeds.wind.state(now), feeds.mag.state(now)])
        let observed = [speed?.last?.time, bz?.last?.time].compactMap { $0 }.min()
        Card(title: "Solar wind", chooser: true, state: state, status: "NOAA SWPC",
             age: Age.text(observed, now: now), focused: focused) {
            VStack(alignment: .leading, spacing: 14) {
                HStack(spacing: 16) {
                    Metric(label: "Wind", value: speed?.last.map { String(format: "%.0f km/s", $0.value) },
                           caption: speed?.last.map { SpaceText.windWord($0.value) } ?? "", size: Theme.fontValue * 0.7)
                        .frame(width: 210, alignment: .leading)
                    if let s = speed, s.count > 2 { Sparkline(values: s.map(\.value)).frame(height: 70) }
                }
                HStack(spacing: 16) {
                    Metric(label: "Bz", value: bz?.last.map { String(format: "%+.1f nT", $0.value) },
                           caption: bz?.last.map { SpaceText.bzWord($0.value) } ?? "", size: Theme.fontValue * 0.7)
                        .frame(width: 210, alignment: .leading)
                    if let s = bz, s.count > 2 { Sparkline(values: s.map(\.value)).frame(height: 70) }
                }
            }
        }
    }
}

/// HF outlook from the shared band_outlook heuristic (not VOACAP), driven by live SFI and Kp.
struct BandsWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let state = combined([feeds.kp.state(now), feeds.flux.state(now)])
        Card(title: "Band conditions", chooser: true, state: state, status: "Estimate from SFI and Kp, not VOACAP",
             age: Age.text(feeds.kp.status.fetched, now: now), focused: focused) {
            if let sfi = feeds.flux.value?.value, let kp = feeds.kp.value?.last?.value, state != .offline {
                let bands = outlook(sfi: sfi, kp: kp)
                // Two-up, as BandsCard.qml in a narrow pane: 80–20 m beside 17–10 m.
                HStack(alignment: .top, spacing: 10) {
                    table(Array(bands.prefix(4)))
                    table(Array(bands.dropFirst(4)))
                }
                Text(String(format: "SFI %.0f · Kp %.1f", sfi, kp)).font(.system(size: Theme.fontCaption))
                    .foregroundStyle(theme.secondary)
            } else {
                Note(text: state == .loading ? "Waiting for SFI and Kp…" : "No data")
            }
        }
    }

    private func outlook(sfi: Double, kp: Double) -> [HcpBand] {
        var out = [HcpBand](repeating: HcpBand(), count: 8)
        let n = Int(hcp_hf_outlook(sfi, kp, 0, &out, out.count))
        return Array(out.prefix(n))
    }

    private func table(_ bands: [HcpBand]) -> some View {
        Grid(alignment: .leading, horizontalSpacing: 6, verticalSpacing: 2) {
            GridRow {
                Text("")
                Text("Day").foregroundStyle(theme.secondary)
                Text("Night").foregroundStyle(theme.secondary)
            }.font(.system(size: 26, weight: .semibold))
            ForEach(0..<bands.count, id: \.self) { i in
                GridRow {
                    Text(cText(bands[i].band)).foregroundStyle(theme.ink).fontWeight(.semibold)
                    cond(bands[i].day)
                    cond(bands[i].night)
                }.font(.system(size: 26))
            }
        }
        .fixedSize()
    }

    private func cond(_ c: Int32) -> some View {
        Text(c == 2 ? "Good" : c == 1 ? "Fair" : "Poor").foregroundStyle(c == 2 ? theme.good : c == 1 ? theme.warn : theme.bad)
    }
}

struct ScalesWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    var body: some View {
        let s = feeds.scales.value
        Card(title: "NOAA scales", chooser: true, state: feeds.scales.state(now), status: "NOAA SWPC",
             age: Age.text(feeds.scales.status.fetched, now: now), focused: focused) {
            if let s {
                HStack(spacing: 14) {
                    tile("R", s.current.r, "Radio", max: s.past24h.r)
                    tile("S", s.current.s, "Radiation", max: s.past24h.s)
                    tile("G", s.current.g, "Geomag.", max: s.past24h.g)
                }
                if s.forecast.0.r_minor >= 0 {
                    Text("Today: R1-2 \(s.forecast.0.r_minor)% · R3+ \(s.forecast.0.r_major)% · S1+ \(s.forecast.0.s_prob)%")
                        .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
                }
            } else { Note(text: "No data") }
        }
    }

    private func tile(_ letter: String, _ level: Int32, _ label: String, max: Int32) -> some View {
        let color = level <= 0 ? theme.good : level <= 2 ? theme.warn : theme.bad
        let word = SpaceText.scaleWord(level)
        return VStack(alignment: .leading, spacing: 2) {
            Text(level >= 0 ? "\(letter)\(level)" : "\(letter)–").font(.system(size: Theme.fontValue, weight: .bold)).foregroundStyle(color)
            Text(word.isEmpty ? "none" : word).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.ink)
            Text(max >= 0 ? "24 h max \(letter)\(max)" : label).font(.system(size: Theme.fontCaption - 3))
                .foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

struct AuroraWidget: View {
    @EnvironmentObject var feeds: Feeds
    let now: Date
    let focused: Bool
    var body: some View {
        let a = feeds.aurora.value?.info
        Card(title: "Aurora", chooser: true, state: feeds.aurora.state(now), status: "NOAA SWPC OVATION",
             age: Age.text(a.map { Date(timeIntervalSince1970: Double($0.observed_ms) / 1000) }, now: now), focused: focused) {
            if let a {
                HStack(spacing: 30) {
                    Metric(label: "North peak", value: "\(a.north_peak)%", caption: Self.word(Int(a.north_peak)))
                    Metric(label: "South peak", value: "\(a.south_peak)%", caption: Self.word(Int(a.south_peak)))
                }
                Note(text: "Highest chance of visible aurora in each hemisphere. Show it on the map from the map menu.")
            } else { Note(text: "No data") }
        }
    }
    static func word(_ p: Int) -> String { p >= 50 ? "High" : p >= 20 ? "Moderate" : p >= 5 ? "Low" : "Very low" }
}

struct DrapWidget: View {
    @EnvironmentObject var feeds: Feeds
    let now: Date
    let focused: Bool
    var body: some View {
        let d = feeds.drap.value?.info
        Card(title: "D-RAP", chooser: true, state: feeds.drap.state(now), status: "NOAA SWPC D-RAP",
             age: Age.text(d.map { Date(timeIntervalSince1970: Double($0.valid_ms) / 1000) }, now: now), focused: focused) {
            if let d {
                Metric(label: "Highest affected frequency", value: String(format: "%.1f MHz", d.peak_mhz),
                       caption: d.peak_mhz < 1 ? "No HF absorption" : "HF absorbed below this on the day side")
                Note(text: cText(d.xray))
                Note(text: cText(d.proton))
            } else { Note(text: "No data") }
        }
    }
}

/// Latest SDO image (solar_image_model.cpp): the image's own age is unknown, so the card shows the fetch time.
struct SolarImageWidget: View {
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    @State private var loadedAt: Date?
    @State private var failed = false
    private static let url = URL(string: "https://sdo.gsfc.nasa.gov/assets/img/latest/latest_512_0193.jpg")!
    var body: some View {
        let state: WidgetState = loadedAt.map { now.timeIntervalSince($0) > 7200 ? .stale : .live } ?? (failed ? .offline : .loading)
        Card(title: "Sun (SDO)", chooser: true, state: state, status: "NASA SDO · AIA 193 Å",
             age: Age.text(loadedAt, now: now), focused: focused) {
            AsyncImage(url: Self.url, transaction: Transaction(animation: .easeIn)) { phase in
                switch phase {
                case .success(let img):
                    img.resizable().scaledToFit().clipShape(Circle())
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                        .onAppear { loadedAt = Date(); failed = false }
                case .failure: Note(text: "Image unavailable").onAppear { failed = true }
                default: Note(text: "Loading…")
                }
            }
            .id(Int(now.timeIntervalSince1970) / 1800) // refetch every 30 minutes
        }
    }
}
