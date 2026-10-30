import SwiftUI
import WidgetKit

struct PassWidget: Widget {
    static let kind = "SatellitePass"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: Self.kind, provider: PassProvider()) { entry in
            PassFamilyView(entry: entry)
        }
        .configurationDisplayName("Next Satellite Pass")
        .description("Next pass of your tracked satellite (or the ISS) over your station.")
        .supportedFamilies([.systemSmall, .systemMedium, .accessoryRectangular, .accessoryInline])
    }
}

struct PassFamilyView: View {
    @Environment(\.widgetFamily) private var family
    let entry: PassEntry

    var body: some View { PassView(entry: entry, family: family).hamClockBackground() }
}

struct PassView: View {
    let entry: PassEntry
    let family: WidgetFamily

    var body: some View {
        switch entry.state {
        case .needsStation:
            message(title: "Set your station", detail: "Open HamClock Pro to set your station")
        case .noElements:
            message(title: "No satellite data", detail: "Waiting for orbital elements (CelesTrak)")
        case .noPass(let sat):
            message(title: "\(sat): no pass", detail: "None in the next 48 hours")
        case .pass(let pass, let elements, let old):
            passBody(pass, elements: elements, old: old)
        }
    }

    private func message(title: String, detail: String) -> some View {
        Group {
            switch family {
            case .accessoryInline: Text(detail)
            case .accessoryRectangular:
                VStack(alignment: .leading, spacing: 0) {
                    Text("Satellite").font(.system(size: 13, weight: .semibold)).widgetAccentable()
                    Text(detail).font(.system(size: 14, weight: .semibold)).lineLimit(2).minimumScaleFactor(0.7)
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            default:
                VStack(alignment: .leading, spacing: 4) {
                    HeaderRow(title: "Next Pass", freshness: .live)
                    Spacer(minLength: 0)
                    Text(title).font(Style.value(20)).lineLimit(2).minimumScaleFactor(0.7)
                    Text(detail).font(Style.label(14)).foregroundStyle(Style.secondary).lineLimit(3)
                    Spacer(minLength: 0)
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            }
        }
    }

    private func passBody(_ pass: SatellitePass, elements: Freshness, old: Bool) -> some View {
        let up = PassPhase.of(pass, at: entry.date) == .upNow
        // Passes are computed on the phone, so being offline does not make them wrong; only
        // old orbital elements do (cached elements are refreshed every 12 h, not every 30 min).
        let badge: Freshness = old ? .stale : .live
        return Group {
            switch family {
            case .accessoryInline:
                if up { Text("\(pass.satellite) up now, max \(Int(pass.maxElevation))°") }
                else { Text("\(pass.satellite) \(Format.clock(pass.aos)) · \(Int(pass.maxElevation))°") }
            case .accessoryRectangular:
                VStack(alignment: .leading, spacing: 0) {
                    Text(pass.satellite + (old ? " · old data" : "")).font(.system(size: 13, weight: .semibold)).widgetAccentable()
                    countdown(pass, up: up).font(Style.value(22)).minimumScaleFactor(0.6).lineLimit(1)
                    Text("Max \(Int(pass.maxElevation))° · rises \(Format.compass(pass.aosAzimuth))")
                        .font(.system(size: 12, weight: .medium)).lineLimit(1)
                }
                .frame(maxWidth: .infinity, alignment: .leading)
            case .systemMedium:
                VStack(alignment: .leading, spacing: 4) {
                    HeaderRow(title: "Next Pass" + (entry.callsign.isEmpty ? "" : " · \(entry.callsign)"), freshness: badge)
                    HStack(alignment: .top) {
                        VStack(alignment: .leading, spacing: 0) {
                            Text(pass.satellite).font(Style.value(30)).lineLimit(1).minimumScaleFactor(0.5)
                            countdown(pass, up: up).font(Style.value(30)).lineLimit(1).minimumScaleFactor(0.5)
                                .foregroundStyle(up ? Style.color(Severity.calm) : .white).widgetAccentable()
                        }
                        Spacer(minLength: 8)
                        VStack(alignment: .leading, spacing: 2) {
                            fact("Rise", "\(Format.clock(pass.aos)) \(Format.compass(pass.aosAzimuth))")
                            fact("Max", "\(Int(pass.maxElevation))°")
                            fact("Set", "\(Format.clock(pass.los)) \(Format.compass(pass.losAzimuth))")
                        }
                    }
                    Spacer(minLength: 0)
                    if old { Text("Orbital elements over 14 days old").font(Style.label(11)).foregroundStyle(Style.secondary) }
                }
            default:
                VStack(alignment: .leading, spacing: 2) {
                    HeaderRow(title: "Next Pass", freshness: badge)
                    Text(pass.satellite).font(Style.value(26)).lineLimit(1).minimumScaleFactor(0.5)
                    countdown(pass, up: up).font(Style.value(26)).lineLimit(1).minimumScaleFactor(0.5)
                        .foregroundStyle(up ? Style.color(Severity.calm) : .white).widgetAccentable()
                    Spacer(minLength: 0)
                    Text("Max \(Int(pass.maxElevation))°").font(Style.value(18))
                    Text("Rises \(Format.compass(pass.aosAzimuth)) \(Format.clock(pass.aos))")
                        .font(Style.label(13)).foregroundStyle(Style.secondary).lineLimit(1).minimumScaleFactor(0.7)
                }
            }
        }
    }

    /// Within this time of AOS the widget shows a live countdown; before that, the rise time.
    static let countdownWindow: TimeInterval = 3600

    /// "Up now" during the pass, a live countdown (kept current by the system) in the last
    /// hour, otherwise "at 14:05": an hours-long timer is hard to read at a glance.
    @ViewBuilder
    private func countdown(_ pass: SatellitePass, up: Bool) -> some View {
        if up { Text("Up now") }
        else if pass.aos.timeIntervalSince(entry.date) > Self.countdownWindow { Text("at \(Format.clock(pass.aos))") }
        else { Text(pass.aos, style: .timer).monospacedDigit() }
    }

    private func fact(_ label: String, _ value: String) -> some View {
        HStack(alignment: .firstTextBaseline, spacing: 6) {
            Text(label).font(Style.label(13)).foregroundStyle(Style.secondary).frame(width: 34, alignment: .leading)
            Text(value).font(Style.value(18)).lineLimit(1).minimumScaleFactor(0.6)
        }
    }
}
