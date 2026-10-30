import SwiftUI

/// HamQSL (N0NBH) conditions (SolarHamqslCard.qml): N0NBH's own day/night HF calculation and the
/// VHF phenomena. Like SolarHamqslModel, a report older than 6 h is never shown as current.
struct HamqslWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    static let maxReportAge: TimeInterval = 6 * 3600

    var body: some View {
        let report = feeds.hamqsl.value
        let updated = report.map { Date(timeIntervalSince1970: Double($0.updated_ms) / 1000) }
        let current = report.map { $0.updated_ms > 0 && now.timeIntervalSince(updated!) <= Self.maxReportAge } ?? false
        let feedState = feeds.hamqsl.state(now)
        let state: WidgetState = report != nil && !current && feedState == .live ? .stale : feedState
        Card(title: "HamQSL conditions", chooser: true, state: state, status: "N0NBH hamqsl.com",
             age: Age.text(feeds.hamqsl.status.fetched, now: now), focused: focused) {
            if let report, current, state != .offline {
                bands(report)
                Text(Self.vhfLine(report) + " · issued " + Age.utcClock(updated!))
                    .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                    .lineLimit(1).minimumScaleFactor(0.75)
            } else {
                Note(text: report == nil ? (state == .loading ? "Loading…" : "No data") : "No report in the last 6 h.")
            }
        }
    }

    private func bands(_ r: HcpHamqsl) -> some View {
        let rows = Self.hf(r)
        return Grid(alignment: .leading, horizontalSpacing: 18, verticalSpacing: 2) {
            GridRow {
                Text("")
                Text("Day").foregroundStyle(theme.secondary)
                Text("Night").foregroundStyle(theme.secondary)
            }.font(.system(size: 26, weight: .semibold))
            ForEach(Array(rows.enumerated()), id: \.offset) { _, b in
                GridRow {
                    Text(cText(b.name)).foregroundStyle(theme.ink)
                    Text(cText(b.day)).foregroundStyle(color(b.day_rank))
                    Text(cText(b.night)).foregroundStyle(color(b.night_rank))
                }.font(.system(size: Theme.fontCaption))
            }
        }
    }

    private func color(_ rank: Int32) -> Color { rank >= 3 ? theme.good : rank == 2 ? theme.warn : rank == 1 ? theme.bad : theme.secondary }

    static func hf(_ r: HcpHamqsl) -> [HcpHfCondition] {
        withUnsafeBytes(of: r.hf) { Array($0.bindMemory(to: HcpHfCondition.self).prefix(Int(r.hf_count))) }
    }

    static func vhf(_ r: HcpHamqsl) -> [HcpVhfCondition] {
        withUnsafeBytes(of: r.vhf) { Array($0.bindMemory(to: HcpVhfCondition.self).prefix(Int(r.vhf_count))) }
    }

    /// "VHF: E-Skip europe open" or "VHF: all closed".
    static func vhfLine(_ r: HcpHamqsl) -> String {
        let open = vhf(r).filter { cText($0.status) != "Band Closed" }
        guard let first = open.first else { return "VHF: all closed" }
        let place = cText(first.location).replacingOccurrences(of: "_", with: " ")
        return "VHF: \(cText(first.name)) \(place) \(cText(first.status))" + (open.count > 1 ? " +\(open.count - 1)" : "")
    }

    nonisolated static func parse(_ data: Data) -> HcpHamqsl? {
        var out = HcpHamqsl()
        return data.withC { hcp_hamqsl_parse($0, $1, &out) } == 1 ? out : nil
    }
}
