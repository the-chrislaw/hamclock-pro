import SwiftUI

/// Space weather impact (SolarImpactCard.qml): the NOAA R, S and G levels and, while fresh, the
/// Kyoto Dst index, ranked by operating impact (worst first), each with its effect on HF, aurora
/// and GPS. Same feeds as the NOAA scales and Dst index panes; the state follows the scales feed.
struct ImpactWidget: View {
    @EnvironmentObject var feeds: Feeds
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    struct Ranking: Equatable {
        var headline: String
        var rows: [Row]
    }
    struct Row: Equatable {
        var tag: String    // "G4"
        var level: Int
        var effect: String
    }

    var body: some View {
        let state = feeds.scales.state(now)
        let ranking = feeds.scales.value.flatMap { Self.rank($0, dst: feeds.dst.value ?? [], now: now) }
        Card(title: "Space weather impact", chooser: true, state: state, status: "NOAA SWPC scales · Kyoto Dst",
             age: Age.text(feeds.scales.status.fetched, now: now), focused: focused) {
            if let ranking, state != .offline {
                // The short headline ("G4 Severe"): each row below carries its own effect.
                Text(ranking.headline.components(separatedBy: ":").first ?? ranking.headline)
                    .font(.system(size: Theme.fontBody, weight: .bold)).foregroundStyle(theme.ink).lineLimit(1)
                Grid(alignment: .leading, horizontalSpacing: 16, verticalSpacing: 4) {
                    ForEach(Array(ranking.rows.enumerated()), id: \.offset) { _, r in
                        GridRow {
                            Text(r.tag).fontWeight(.bold).foregroundStyle(tone(r.level))
                            Text(r.effect).foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
                        }
                        .font(.system(size: Theme.fontCaption))
                    }
                }
            } else {
                Note(text: state == .loading ? "Loading…" : "No data")
            }
        }
    }

    private func tone(_ level: Int) -> Color { level >= 3 ? theme.bad : level >= 1 ? theme.warn : theme.good }

    static func rank(_ scales: HcpScales, dst: [DstReading], now: Date) -> Ranking? {
        var s = scales
        let samples = dst.map { HcpDst(time_ms: Int64($0.time.timeIntervalSince1970 * 1000), dst: Int32($0.dst)) }
        var out = [HcpImpactRow](repeating: HcpImpactRow(), count: 4)
        var head = [CChar](repeating: 0, count: 128)
        let n = samples.withUnsafeBufferPointer { d in
            out.withUnsafeMutableBufferPointer { o in
                head.withUnsafeMutableBufferPointer { h in
                    hcp_impact_rank(&s, d.baseAddress, d.count, Int64(now.timeIntervalSince1970 * 1000), o.baseAddress, o.count,
                                    h.baseAddress, h.count)
                }
            }
        }
        guard n > 0 else { return nil }
        let rows = out.prefix(Int(n)).map { r in
            Row(tag: String(UnicodeScalar(UInt8(bitPattern: r.letter))) + (r.level >= 0 ? "\(r.level)" : "–"),
                level: Int(r.level), effect: cText(r.effect).isEmpty ? "Not stated" : cText(r.effect))
        }
        return Ranking(headline: String(cString: head), rows: rows)
    }
}
