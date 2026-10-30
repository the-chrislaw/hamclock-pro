import SwiftUI

struct SunspotMonth: Equatable, Sendable {
    var month: String // "YYYY-MM"
    var number: Double
}

/// Solar trend (SolarTrendCard.qml): 3-day Kp bars (36 h observed, then the forecast drawn
/// lighter), 30 days of the 10.7 cm flux and the latest monthly sunspot number. Like
/// SolarTrendModel the card is Live only when all three products are; with some missing or out
/// of date it is Stale and says which.
struct TrendWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    var body: some View {
        let parts: [(label: String, has: Bool, state: WidgetState)] = [
            ("Kp", !(feeds.kpForecast.value ?? []).isEmpty, feeds.kpForecast.state(now)),
            ("Solar flux", !(feeds.fluxHistory.value ?? []).isEmpty, feeds.fluxHistory.state(now)),
            ("Sunspots", feeds.sunspots.value != nil, feeds.sunspots.state(now)),
        ]
        let lagging = parts.filter { !$0.has || $0.state != .live }.map(\.label)
        let state: WidgetState = parts.allSatisfy { !$0.has } ? combined(parts.map(\.state))
            : lagging.isEmpty ? .live : .stale
        Card(title: "Solar trend", chooser: true, state: state,
             status: lagging.isEmpty || state != .stale ? "NOAA SWPC" : "Not current: " + lagging.joined(separator: ", "),
             age: Age.text(feeds.kpForecast.status.fetched, now: now), focused: focused) {
            if state == .offline || state == .loading {
                Note(text: state == .loading ? "Loading…" : "No data")
            } else {
                content
            }
        }
    }

    @ViewBuilder private var content: some View {
        if let bars = feeds.kpForecast.value, !bars.isEmpty {
            Text("KP, 36 H AND FORECAST").font(.system(size: Theme.fontCaption - 4, weight: .semibold)).kerning(0.8)
                .foregroundStyle(theme.secondary)
            KpBarsView(bars: bars).frame(height: 70)
        }
        if let days = feeds.fluxHistory.value, days.count > 1 {
            HStack(alignment: .center, spacing: 12) {
                Metric(label: "SFI 30 d", value: String(format: "%.0f", days.last!.flux), caption: Self.fluxCaption(days),
                       size: Theme.fontValue * 0.6)
                Sparkline(values: days.map(\.flux)).frame(height: 54)
            }
        }
        if let s = feeds.sunspots.value {
            Text(String(format: "Sunspots %.0f · %@", s.number, Self.monthName(s.month)))
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.75)
        }
    }

    /// "+12 over 29 days" (SolarTrendModel::fluxCaption).
    static func fluxCaption(_ days: [HcpFluxDay]) -> String {
        guard let first = days.first, let last = days.last else { return "" }
        let delta = Int(last.flux.rounded()) - Int(first.flux.rounded())
        let span = (last.day_ms - first.day_ms) / 86_400_000
        return "\(delta > 0 ? "+" : "")\(delta) over \(span) days"
    }

    /// "2026-09" -> "Sep 2026".
    static func monthName(_ ym: String) -> String {
        let parts = ym.split(separator: "-")
        guard parts.count == 2, let m = Int(parts[1]), (1...12).contains(m) else { return ym }
        let names = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
        return "\(names[m - 1]) \(parts[0])"
    }
}

/// Kp bars on a 0-9 scale (KpBars.qml): quiet / active / storm colours, forecast lighter.
struct KpBarsView: View {
    @Environment(\.theme) private var theme
    let bars: [HcpKpBar]
    var body: some View {
        Canvas { ctx, size in
            guard !bars.isEmpty else { return }
            let gap: CGFloat = 2
            let w = max(1, (size.width - gap * CGFloat(bars.count - 1)) / CGFloat(bars.count))
            for (i, b) in bars.enumerated() {
                let h = max(2, size.height * CGFloat(b.kp / 9))
                let color = b.level >= 2 ? theme.bad : b.level == 1 ? theme.warn : theme.good
                let rect = CGRect(x: CGFloat(i) * (w + gap), y: size.height - h, width: w, height: h)
                ctx.fill(Path(rect), with: .color(color.opacity(b.predicted != 0 ? 0.4 : 0.95)))
            }
        }
        .accessibilityLabel("Kp bars")
    }
}

enum TrendParse {
    nonisolated static func kp(_ data: Data) -> [HcpKpBar]? {
        var out = [HcpKpBar](repeating: HcpKpBar(), count: 64)
        let n = out.withUnsafeMutableBufferPointer { o in data.withC { hcp_trend_kp_bars($0, $1, o.baseAddress, o.count) } }
        return n >= 0 ? Array(out.prefix(Int(n))) : nil
    }

    nonisolated static func flux(_ data: Data) -> [HcpFluxDay]? {
        var out = [HcpFluxDay](repeating: HcpFluxDay(), count: 30)
        let n = out.withUnsafeMutableBufferPointer { o in data.withC { hcp_trend_flux_daily($0, $1, 30, o.baseAddress, o.count) } }
        return n >= 0 ? Array(out.prefix(Int(n))) : nil
    }

    nonisolated static func sunspots(_ data: Data) -> SunspotMonth? {
        var month = [CChar](repeating: 0, count: 8)
        var number = 0.0
        let ok = month.withUnsafeMutableBufferPointer { m in
            data.withC { hcp_trend_sunspots($0, $1, m.baseAddress, m.count, &number) }
        }
        return ok == 1 ? SunspotMonth(month: String(cString: month), number: number) : nil
    }
}
