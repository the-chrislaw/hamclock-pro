import SwiftUI

/// NOAA SWPC alerts, watches and warnings in force (SolarAlertsCard.qml). The feed keeps every
/// parsed message; what is in force is worked out against the clock each render (core
/// hcp_alerts_active), so a warning drops at its end time without a refetch.
struct AlertsWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    private static let rowsShown = 3

    var body: some View {
        let state = feeds.alerts.state(now)
        let active = feeds.alerts.value.map { SpaceAlerts.active($0, at: now) }
        Card(title: "Space alerts", chooser: true, state: state, status: "NOAA SWPC",
             age: Age.text(feeds.alerts.status.fetched, now: now), focused: focused) {
            if let active, state != .offline {
                if active.isEmpty { Note(text: "No alerts, watches or warnings in force.") }
                ForEach(Array(active.prefix(Self.rowsShown).enumerated()), id: \.offset) { _, a in row(a) }
                if active.count > Self.rowsShown {
                    Text("+\(active.count - Self.rowsShown) more").font(.system(size: Theme.fontCaption))
                        .foregroundStyle(theme.secondary)
                }
            } else {
                // Offline with nothing cached is "no data", never "nothing in force".
                Note(text: state == .loading ? "Loading…" : "No data")
            }
        }
    }

    private func row(_ a: HcpAlert) -> some View {
        let color = a.level >= 2 ? theme.bad : a.level == 1 ? theme.warn : theme.good
        let scale = cText(a.scale)
        return VStack(alignment: .leading, spacing: 0) {
            HStack(spacing: 10) {
                Text(scale.isEmpty ? cText(a.kind) : scale)
                    .font(.system(size: Theme.fontCaption, weight: .bold)).foregroundStyle(color)
                    .lineLimit(1).fixedSize()
                Text(cText(a.headline)).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.ink)
                    .lineLimit(1).minimumScaleFactor(0.75)
            }
            Text(SpaceAlerts.when(a)).font(.system(size: Theme.fontCaption - 3)).foregroundStyle(theme.secondary)
                .lineLimit(1)
        }
    }
}

enum SpaceAlerts {
    static let capacity = 64

    /// Every parseable message, newest first (nil when the body is not the product).
    nonisolated static func parse(_ data: Data) -> [HcpAlert]? {
        var out = [HcpAlert](repeating: HcpAlert(), count: capacity)
        let n = out.withUnsafeMutableBufferPointer { o in data.withC { hcp_alerts_parse($0, $1, o.baseAddress, o.count) } }
        return n >= 0 ? Array(out.prefix(Int(n))) : nil
    }

    static func active(_ all: [HcpAlert], at now: Date) -> [HcpAlert] {
        var out = [HcpAlert](repeating: HcpAlert(), count: max(all.count, 1))
        let n = all.withUnsafeBufferPointer { a in
            out.withUnsafeMutableBufferPointer { o in
                hcp_alerts_active(a.baseAddress, a.count, Int64(now.timeIntervalSince1970 * 1000), o.baseAddress, o.count)
            }
        }
        return Array(out.prefix(Int(n)))
    }

    /// "Watch · issued 14:31Z", "Warning · until 21:00Z" (SolarAlertsModel row text).
    static func when(_ a: HcpAlert) -> String {
        let kind = cText(a.kind)
        if a.until_ms > 0 { return kind + " · until " + Age.utcClock(Date(timeIntervalSince1970: Double(a.until_ms) / 1000)) }
        return kind + " · issued " + Age.utcClock(Date(timeIntervalSince1970: Double(a.issued_ms) / 1000))
    }
}
