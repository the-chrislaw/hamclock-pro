import SwiftUI

struct DstReading: Equatable, Sendable {
    var time: Date
    var dst: Int
}

/// Kyoto Dst index (SolarDstCard.qml): latest hourly value with its storm word, the last 48 h as a
/// line, and the 24 h low. Like SolarDstModel, a latest record older than 6 h is not shown as
/// current (the card turns Stale and says so).
struct DstWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    static let maxObservationAge: TimeInterval = 6 * 3600

    var body: some View {
        let all = feeds.dst.value ?? []
        let latest = all.last
        let fresh = latest.map { now.timeIntervalSince($0.time) <= Self.maxObservationAge } ?? false
        let feedState = feeds.dst.state(now)
        let state: WidgetState = latest != nil && !fresh && feedState == .live ? .stale : feedState
        Card(title: "Dst index", chooser: true, state: state, status: "Kyoto Dst · NOAA",
             age: Age.text(feeds.dst.status.fetched, now: now), focused: focused) {
            if let latest, fresh, state != .offline {
                let tone = latest.dst <= -100 ? theme.bad : latest.dst <= -30 ? theme.warn : theme.good
                Metric(label: "Dst", value: "\(latest.dst) nT", caption: DstText.word(latest.dst), color: tone)
                let history = Self.window(all, hours: 48)
                if history.count > 2 {
                    Sparkline(values: history.map { Double($0.dst) }).frame(height: 60)
                }
                Text("24 h low \(Self.window(all, hours: 24).map(\.dst).min() ?? latest.dst) nT · observed \(Age.utcClock(latest.time))")
                    .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                    .lineLimit(1).minimumScaleFactor(0.75)
            } else {
                Note(text: latest == nil ? (state == .loading ? "Loading…" : "No data") : "No reading in the last 6 h.")
            }
        }
    }

    /// Records within `hours` of the newest one (SolarDstModel::history / min24).
    static func window(_ all: [DstReading], hours: Double) -> [DstReading] {
        guard let last = all.last else { return [] }
        let from = last.time.addingTimeInterval(-hours * 3600)
        return all.filter { $0.time >= from }
    }
}

enum DstText {
    static func word(_ dst: Int) -> String { cString(16) { hcp_dst_word(Int32(dst), $0, $1) } }
}
