import SwiftUI

struct DashboardView: View {
    @EnvironmentObject var weather: SpaceWeatherStore

    var body: some View {
        TimelineView(.periodic(from: .now, by: 1)) { ctx in
            VStack(alignment: .leading, spacing: 40) {
                ClockBar(now: ctx.date)
                HStack(alignment: .top, spacing: 40) {
                    Card(title: "Kp index", sample: weather.kp, now: ctx.date,
                         big: { String(format: "%.1f", $0) },
                         detail: { SpaceWeatherStore.kpDescription($0) })
                    Card(title: "Solar flux", sample: weather.flux, now: ctx.date,
                         big: { String(format: "%.0f sfu", $0) }, detail: { _ in "10.7 cm" })
                    Card(title: "X-ray", sample: weather.xray, now: ctx.date,
                         big: { SpaceWeatherStore.xrayClass($0) }, detail: { _ in "GOES 0.1–0.8 nm" })
                }
                HStack(alignment: .top, spacing: 40) {
                    Card(title: "Solar wind", sample: weather.windSpeed, now: ctx.date,
                         big: { String(format: "%.0f km/s", $0) },
                         detail: { SpaceWeatherStore.windSpeedDescription($0) })
                    Card(title: "IMF Bz (GSM)", sample: weather.bz, now: ctx.date,
                         big: { String(format: "%+.1f nT", $0) },
                         detail: { SpaceWeatherStore.bzDescription($0) })
                }
                SunCard(now: ctx.date)
                Spacer()
                Text("Data: NOAA Space Weather Prediction Center")
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            }
            .padding(.horizontal, 90).padding(.vertical, 60) // tvOS title-safe area
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
            .background(Color.black)
        }
    }
}

private struct ClockBar: View {
    let now: Date
    var body: some View {
        HStack(alignment: .firstTextBaseline, spacing: 40) {
            Text(now.formatted(Date.FormatStyle(timeZone: TimeZone(identifier: "UTC")!)
                .hour(.twoDigits(amPM: .omitted)).minute().second()) + " UTC")
                .font(.system(size: 96, weight: .semibold, design: .monospaced))
            Text(now.formatted(Date.FormatStyle(timeZone: TimeZone(identifier: "UTC")!)
                .weekday(.wide).day().month(.wide).year()))
                .font(.system(size: 48)).foregroundStyle(.secondary)
        }
    }
}

private struct Card: View {
    let title: String
    let sample: Sample?
    let now: Date
    let big: (Double) -> String
    let detail: (Double) -> String

    var body: some View {
        let state = SpaceWeatherStore.freshness(sample, now: now)
        VStack(alignment: .leading, spacing: 12) {
            Text(title).font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if let s = sample {
                Text(big(s.value)).font(.system(size: 96, weight: .bold, design: .rounded))
                Text(detail(s.value)).font(.system(size: 29))
                Text("as of " + s.observed.formatted(date: .omitted, time: .shortened))
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            } else {
                Text("No data").font(.system(size: 48)).foregroundStyle(.secondary)
            }
            if state != .live {
                Text(state == .stale ? "STALE" : "OFFLINE")
                    .font(.system(size: 29, weight: .bold)).foregroundStyle(.orange)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}

/// Computed offline by the shared C++ solar_position code from the device clock:
/// no network involved, so there is no Stale/Offline state (it is exact for `now`).
private struct SunCard: View {
    let now: Date
    var body: some View {
        var lat = 0.0, lon = 0.0
        let t = Int64(now.timeIntervalSince1970)
        let ok = hcp_subsolar_point(t, &lat, &lon) != 0
        VStack(alignment: .leading, spacing: 12) {
            Text("Sun").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            if ok {
                Text(String(format: "Overhead %.1f°%@ %.1f°%@",
                            abs(lat), lat >= 0 ? "N" : "S", abs(lon), lon >= 0 ? "E" : "W"))
                    .font(.system(size: 64, weight: .bold, design: .rounded))
                Text("Subsolar point, computed on device from the clock")
                    .font(.system(size: 29)).foregroundStyle(.secondary)
            } else {
                Text("Unavailable").font(.system(size: 48)).foregroundStyle(.secondary)
            }
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}
