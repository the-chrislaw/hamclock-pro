import SwiftUI
import WidgetKit

struct BandsWidget: Widget {
    static let kind = "BandConditions"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: Self.kind, provider: BandsProvider()) { entry in
            BandsFamilyView(entry: entry)
        }
        .configurationDisplayName("HF Band Conditions")
        .description("Day and night HF outlook by band from solar flux and Kp (a rough guide, not VOACAP).")
        .supportedFamilies([.systemSmall, .systemMedium, .accessoryRectangular, .accessoryInline])
    }
}

struct BandsFamilyView: View {
    @Environment(\.widgetFamily) private var family
    let entry: BandsEntry

    var body: some View { BandsView(entry: entry, family: family).hamClockBackground() }
}

struct BandsView: View {
    let entry: BandsEntry
    let family: WidgetFamily

    private var freshness: Freshness { entry.weather.freshness(now: entry.date) }
    private var period: String { entry.isDay ? "Day" : "Night" }

    var body: some View {
        if let outlook = entry.outlook {
            switch family {
            case .accessoryInline: Text(inlineText(outlook))
            case .accessoryRectangular: rectangular(outlook)
            case .systemMedium: medium(outlook)
            default: small(outlook)
            }
        } else {
            noData
        }
    }

    func inlineText(_ outlook: [BandOutlook]) -> String {
        guard let open = Format.openBands(outlook, day: entry.isDay) else { return "HF \(period.lowercased()): all poor" }
        return "HF \(open.list) \(open.condition.word.lowercased())"
    }

    private var noData: some View {
        VStack(alignment: .leading, spacing: 4) {
            if family == .systemSmall || family == .systemMedium {
                HeaderRow(title: "HF Bands", freshness: freshness)
            }
            Spacer(minLength: 0)
            Text("No band data").font(Style.value(family == .accessoryInline ? 14 : 20))
            Text("Needs solar flux and Kp from NOAA").font(Style.label(12)).foregroundStyle(Style.secondary)
            Spacer(minLength: 0)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func rectangular(_ outlook: [BandOutlook]) -> some View {
        let open = Format.openBands(outlook, day: entry.isDay)
        return VStack(alignment: .leading, spacing: 0) {
            Text("HF \(period)" + (freshness.badge.map { " · \($0)" } ?? ""))
                .font(.system(size: 13, weight: .semibold)).widgetAccentable()
            Text(open?.list ?? "All poor").font(Style.value(22)).minimumScaleFactor(0.6).lineLimit(1)
            Text(open.map { "\($0.condition.word) (m)" } ?? "").font(.system(size: 12, weight: .medium))
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func small(_ outlook: [BandOutlook]) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            HeaderRow(title: "HF \(period)", freshness: freshness)
            ForEach(Format.bandGroups(outlook, day: entry.isDay), id: \.label) { g in
                HStack(alignment: .firstTextBaseline) {
                    Text(g.label).font(Style.value(19)).lineLimit(1)
                    Spacer(minLength: 4)
                    Text(g.condition.word).font(Style.value(19))
                        .foregroundStyle(Style.color(g.condition)).widgetAccentable()
                }
            }
            Spacer(minLength: 0)
            AgeFooter(fetched: entry.weather.fetched, freshness: freshness)
        }
    }

    private func medium(_ outlook: [BandOutlook]) -> some View {
        let day = Format.bandGroups(outlook, day: true)
        let night = Format.bandGroups(outlook, day: false)
        return VStack(alignment: .leading, spacing: 2) {
            HStack {
                HeaderRow(title: "HF Bands · now \(period.lowercased())", freshness: freshness)
            }
            HStack(alignment: .firstTextBaseline) {
                Text("m").font(Style.label(13)).foregroundStyle(Style.secondary).frame(width: 70, alignment: .leading)
                Text("Day").font(Style.label(14)).foregroundStyle(entry.isDay ? .white : Style.secondary)
                    .frame(maxWidth: .infinity, alignment: .leading)
                Text("Night").font(Style.label(14)).foregroundStyle(entry.isDay ? Style.secondary : .white)
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
            ForEach(Array(zip(day, night)), id: \.0.label) { d, n in
                HStack(alignment: .firstTextBaseline) {
                    Text(d.label).font(Style.value(18)).frame(width: 70, alignment: .leading)
                    Text(d.condition.word).font(Style.value(18)).foregroundStyle(Style.color(d.condition))
                        .widgetAccentable().frame(maxWidth: .infinity, alignment: .leading)
                    Text(n.condition.word).font(Style.value(18)).foregroundStyle(Style.color(n.condition))
                        .widgetAccentable().frame(maxWidth: .infinity, alignment: .leading)
                }
                .lineLimit(1)
            }
            Spacer(minLength: 0)
            footer
        }
    }

    private var footer: some View {
        HStack {
            if let sfi = entry.weather.sfi?.value, let kp = entry.weather.kp?.value {
                Text("SFI \(Format.sfi(sfi)) · Kp \(Format.kp(kp))").font(Style.label(11)).foregroundStyle(Style.secondary)
            }
            Spacer(minLength: 4)
            AgeFooter(fetched: entry.weather.fetched, freshness: freshness)
        }
        .lineLimit(1)
    }
}
