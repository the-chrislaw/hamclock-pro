import SwiftUI
import WidgetKit

struct SpaceWeatherWidget: Widget {
    static let kind = "SpaceWeather"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: Self.kind, provider: SpaceWeatherProvider()) { entry in
            SpaceWeatherFamilyView(entry: entry)
        }
        .configurationDisplayName("Space Weather")
        .description("Kp index, solar flux (SFI) and X-ray flare class from NOAA SWPC.")
        .supportedFamilies([.systemSmall, .systemMedium, .accessoryRectangular, .accessoryCircular, .accessoryInline])
    }
}

struct SpaceWeatherFamilyView: View {
    @Environment(\.widgetFamily) private var family
    let entry: SpaceWeatherEntry

    var body: some View { SpaceWeatherView(entry: entry, family: family).hamClockBackground() }
}

struct SpaceWeatherView: View {
    let entry: SpaceWeatherEntry
    let family: WidgetFamily

    private var freshness: Freshness { entry.weather.freshness(now: entry.date) }
    private var xrayClass: String? { entry.weather.xray.flatMap { Core.xrayClass($0.value) } }

    var body: some View {
        switch family {
        case .accessoryInline: inline
        case .accessoryCircular: circular
        case .accessoryRectangular: rectangular
        case .systemMedium: medium
        default: small
        }
    }

    private var inline: some View {
        Text(inlineText)
    }

    var inlineText: String {
        guard let kp = entry.weather.kp?.value else { return "Space weather: no data" }
        var parts = ["Kp \(Format.kp(kp))"]
        if let sfi = entry.weather.sfi?.value { parts.append("SFI \(Format.sfi(sfi))") }
        if let x = xrayClass { parts.append(x) }
        if let badge = freshness.badge { parts.append(badge) }
        return parts.joined(separator: " · ")
    }

    private var circular: some View {
        let kp = entry.weather.kp?.value
        return Gauge(value: min(kp ?? 0, 9), in: 0...9) {
            Text("Kp")
        } currentValueLabel: {
            Text(kp.map(Format.kp) ?? "–")
        }
        .gaugeStyle(.accessoryCircular)
        .widgetAccentable()
    }

    private var rectangular: some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(freshness.badge.map { "Space wx · \($0)" } ?? "Space weather")
                .font(.system(size: 13, weight: .semibold)).widgetAccentable()
            HStack(spacing: 8) {
                metric("Kp", entry.weather.kp.map { Format.kp($0.value) })
                metric("SFI", entry.weather.sfi.map { Format.sfi($0.value) })
                metric("X", xrayClass)
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func metric(_ label: String, _ value: String?) -> some View {
        VStack(alignment: .leading, spacing: -2) {
            Text(label).font(.system(size: 11, weight: .medium))
            Text(value ?? "–").font(Style.value(20)).minimumScaleFactor(0.6).lineLimit(1)
        }
    }

    private var small: some View {
        VStack(alignment: .leading, spacing: 2) {
            HeaderRow(title: "Space Wx", freshness: freshness)
            row("Kp", entry.weather.kp.map { Format.kp($0.value) },
                entry.weather.kp.map { Format.kpSeverity($0.value) })
            row("SFI", entry.weather.sfi.map { Format.sfi($0.value) },
                entry.weather.sfi.map { Format.sfiSeverity($0.value) })
            row("X-ray", xrayClass, xrayClass.map(Format.xraySeverity))
            Spacer(minLength: 0)
            AgeFooter(fetched: entry.weather.fetched, freshness: freshness)
        }
    }

    /// Traffic-light colour only for live data; old values are shown grey next to their badge and age.
    private func liveColor(_ severity: Severity?) -> Color {
        guard freshness == .live, let severity else { return Style.secondary }
        return Style.color(severity)
    }

    private func row(_ label: String, _ value: String?, _ severity: Severity?) -> some View {
        HStack(alignment: .firstTextBaseline) {
            Text(label).font(Style.label(15)).foregroundStyle(Style.secondary)
            Spacer(minLength: 4)
            Text(value ?? "–")
                .font(Style.value(30)).minimumScaleFactor(0.6).lineLimit(1)
                .foregroundStyle(liveColor(severity))
                .widgetAccentable()
        }
    }

    private var medium: some View {
        VStack(alignment: .leading, spacing: 6) {
            HeaderRow(title: "Space Weather", freshness: freshness)
            HStack(alignment: .top, spacing: 10) {
                tile("Kp", entry.weather.kp.map { Format.kp($0.value) },
                     entry.weather.kp.map { Format.kpWord($0.value) },
                     entry.weather.kp.map { Format.kpSeverity($0.value) })
                tile("SFI", entry.weather.sfi.map { Format.sfi($0.value) }, "Solar flux",
                     entry.weather.sfi.map { Format.sfiSeverity($0.value) })
                tile("X-ray", xrayClass, xrayClass.map(xrayWord), xrayClass.map(Format.xraySeverity))
            }
            Spacer(minLength: 0)
            AgeFooter(fetched: entry.weather.fetched, freshness: freshness)
        }
    }

    private func xrayWord(_ cls: String) -> String {
        switch Format.xraySeverity(cls) {
        case .calm: return "No flare"
        case .elevated: return "M flare"
        case .severe: return "X flare"
        }
    }

    private func tile(_ label: String, _ value: String?, _ word: String?, _ severity: Severity?) -> some View {
        VStack(alignment: .leading, spacing: 0) {
            Text(label).font(Style.label(14)).foregroundStyle(Style.secondary)
            Text(value ?? "–")
                .font(Style.value(40)).minimumScaleFactor(0.5).lineLimit(1)
                .foregroundStyle(liveColor(severity))
                .widgetAccentable()
            Text(word ?? " ").font(Style.label(13)).lineLimit(1).minimumScaleFactor(0.7)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}
