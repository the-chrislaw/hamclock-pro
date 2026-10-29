import SwiftUI

/// Callsign banner, big clock, date, DE/DX boxes and the path (StationCard.qml).
struct StationCardView: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool
    let bannerMode: Int
    let showLocal: Bool
    let longPath: Bool

    var body: some View {
        let title = !settings.configured ? "Your station" : bannerMode == 0 ? "Station" : settings.callsign
        Card(title: title, focused: focused) {
            VStack(alignment: .leading, spacing: Theme.gap * 0.5) {
                if settings.configured { banner }
                clock
                if !settings.configured {
                    Text(StationText.dayOfYear(now)).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
                }
                Text(StationText.zoneLabel(settings.deZone, now: now)).font(.system(size: Theme.fontCaption))
                    .foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
                Rectangle().fill(theme.divider).frame(height: 1.5)
                if let de = settings.de {
                    let dx = settings.dx ?? de
                    place("DE · Home", de, local: "")
                    place("DX · Target", dx, local: dxLocal(dx))
                    Text(StationText.path(de: de, dx: dx, longPath: longPath, imperial: settings.imperialUnits)
                        .replacingOccurrences(of: "\n", with: " · "))
                        .font(.system(size: Theme.fontBody)).foregroundStyle(theme.ink)
                        .lineLimit(1).minimumScaleFactor(0.7)
                } else {
                    Text("Add callsign and location to see paths and local time.")
                        .font(.system(size: Theme.fontBody)).foregroundStyle(theme.secondary)
                        .fixedSize(horizontal: false, vertical: true)
                    Text("Press Select to set up").font(.system(size: Theme.fontBody, weight: .semibold))
                        .foregroundStyle(theme.accentInk)
                        .frame(maxWidth: .infinity).padding(.vertical, 16)
                        .background(Capsule().fill(theme.accent))
                }
                Spacer(minLength: 0)
            }
        }
        .accessibilityElement(children: .combine)
        .accessibilityAddTraits(.isButton)
        .accessibilityIdentifier("stationCard")
    }

    private var banner: some View {
        let text = StationText.banner(mode: bannerMode, call: settings.callsign, title: settings.bannerTitle)
        return Text(text.isEmpty ? " " : text)
            .font(.system(size: Theme.fontTitle * 1.7, weight: .bold))
            .foregroundStyle(StationText.bannerColor(style: settings.bannerStyle, now: now) ?? theme.accent)
            .lineLimit(1).minimumScaleFactor(0.5)
            .padding(.horizontal, StationText.bannerBackground(settings.bannerBackground) == nil ? 0 : 12)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(RoundedRectangle(cornerRadius: 10).fill(StationText.bannerBackground(settings.bannerBackground) ?? .clear))
            .accessibilityIdentifier("callsignBanner")
    }

    private var clock: some View {
        let zone = TimeZone(identifier: settings.deZone)
        let local = (showLocal != settings.localClockPrimary) && zone != nil
        let primaryZone = local ? zone! : StationText.utc
        return VStack(alignment: .leading, spacing: 2) {
            Text(StationText.clock(now, primaryZone, h12: settings.clock12h))
                .font(.system(size: Theme.fontClock, weight: .light).monospacedDigit())
                .foregroundStyle(theme.ink).lineLimit(1).minimumScaleFactor(0.5)
                .accessibilityIdentifier("bigClock")
            Text(StationText.date(now, primaryZone, order: settings.dateOrder) + (local ? " · local" : " · UTC"))
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            if let zone {
                Text(local ? StationText.clock(now, StationText.utc, h12: settings.clock12h) + " UTC"
                           : StationText.shortClock(now, zone, h12: settings.clock12h) + " local")
                    .font(.system(size: Theme.fontBody)).foregroundStyle(theme.secondary)
            }
        }
    }

    private func dxLocal(_ dx: (lat: Double, lon: Double)) -> String {
        let override = settings.dxZone
        let zone = override.isEmpty ? TimeZones.suggest(lat: dx.lat, lon: dx.lon) : override
        return StationText.zoneLocal(zone, now: now, h12: settings.clock12h, suggested: override.isEmpty)
    }

    private func place(_ label: String, _ p: (lat: Double, lon: Double), local: String) -> some View {
        VStack(alignment: .leading, spacing: 1) {
            Text(label).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            Text(StationText.grid(p.lat, p.lon)).font(.system(size: Theme.fontTitle * 1.25, weight: .semibold)).foregroundStyle(theme.ink)
            Text(StationText.coordinates(p.lat, p.lon)).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            if !local.isEmpty {
                Text(local).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
            }
            Text("☀ " + StationText.sunLine(now, lat: p.lat, lon: p.lon)).font(.system(size: Theme.fontCaption))
                .foregroundStyle(theme.secondary).lineLimit(1).minimumScaleFactor(0.7)
        }
    }
}
