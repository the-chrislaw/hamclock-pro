import SwiftUI

/// HF path reliability (BandsMatrixCard.qml): DE → DX reliability by band, 80 m down to 10 m,
/// for the next 24 UTC hours across, from the in-house path model over NOAA SWPC SFI and Kp
/// (approximate, not VOACAP). SSB, 100 W and 6 dBi as the Qt app's defaults; short path.
struct PathReliabilityWidget: View {
    @EnvironmentObject var feeds: Feeds
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let now: Date
    let focused: Bool

    struct PathGrid: Equatable {
        var bands: [String]
        var cells: [[Double]] // per band, per hour
        var best: String
        var start: String
        var km: Double
    }

    var body: some View {
        let state: WidgetState = settings.configured ? combined([feeds.kp.state(now), feeds.flux.state(now)]) : .unconfigured
        let sfi = feeds.flux.value?.value, kp = feeds.kp.value?.last?.value
        let grid = (sfi != nil && kp != nil && state != .offline) ? Self.card(de: settings.de, dx: settings.dx, now: now, sfi: sfi!, kp: kp!) : nil
        Card(title: "HF path reliability", chooser: true, state: state, status: status(grid),
             age: Age.text(feeds.kp.status.fetched, now: now), focused: focused) {
            if !settings.configured {
                Note(text: "Set your station in Settings.")
            } else if sfi == nil || kp == nil || state == .offline {
                Note(text: state == .loading ? "Waiting for SFI and Kp…" : "No data")
            } else if let grid {
                Text(grid.best + " · UTC from " + grid.start)
                    .font(.system(size: Theme.fontCaption, weight: .semibold)).foregroundStyle(theme.ink)
                    .lineLimit(1).minimumScaleFactor(0.8)
                heat(grid)
            } else {
                Note(text: "Set a DX target in Settings.")
            }
        }
    }

    private func status(_ g: PathGrid?) -> String {
        guard let g else { return "SFI + Kp · approximate, not VOACAP" }
        let dist = Int((settings.imperialUnits ? g.km / 1.609344 : g.km).rounded())
        return "\(dist) \(settings.imperialUnits ? "mi" : "km") · SSB 100 W · approximate, not VOACAP"
    }

    /// One row per band, one cell per hour, shaded good / warn / bad like BandsMatrixCard.shade.
    private func heat(_ g: PathGrid) -> some View {
        GeometryReader { geo in
            let rows = CGFloat(max(1, g.bands.count))
            let gap: CGFloat = 2
            let rowHeight = geo.size.height / rows
            let label: CGFloat = 64
            let cell = max(2, (geo.size.width - label - 24 * gap) / 24)
            VStack(alignment: .leading, spacing: 0) {
                ForEach(Array(g.bands.enumerated()), id: \.offset) { b, band in
                    HStack(spacing: gap) {
                        Text(band).font(.system(size: min(26, rowHeight * 0.95), weight: .semibold)).foregroundStyle(theme.ink)
                            .frame(width: label, alignment: .leading)
                        ForEach(0..<g.cells[b].count, id: \.self) { h in
                            let v = g.cells[b][h]
                            Rectangle().fill(shade(v)).opacity(0.25 + 0.75 * v)
                                .frame(width: cell, height: max(1, rowHeight - gap))
                        }
                    }
                    .frame(height: rowHeight)
                }
            }
        }
        .accessibilityLabel(g.best)
    }

    private func shade(_ v: Double) -> Color { v >= 0.6 ? theme.good : v >= 0.25 ? theme.warn : theme.bad }

    /// Recomputed when the hour, the indices or the station change (the core call is ~200 path runs).
    static func card(de: (lat: Double, lon: Double)?, dx: (lat: Double, lon: Double)?, now: Date, sfi: Double, kp: Double) -> PathGrid? {
        guard let de else { return nil }
        let dxp = dx ?? de
        let t = Int64(now.timeIntervalSince1970)
        return Memo.get("path|\(t / 3600)|\(sfi)|\(kp)|\(de.lat),\(de.lon)|\(dxp.lat),\(dxp.lon)") { () -> PathGrid? in
            var input = HcpPathInput(de_lat_deg: de.lat, de_lon_deg: de.lon, dx_lat_deg: dxp.lat, dx_lon_deg: dxp.lon,
                                     power_w: 100, gain_db: 6, mode: 0, long_path: 0)
            var out = HcpPathCard()
            guard hcp_path_card(&input, t, sfi, kp, &out) == 1, out.has_path != 0 else { return nil }
            let bands = Int(out.band_count), hours = Int(out.hours)
            let names: [String] = withUnsafeBytes(of: out.band) { raw in
                (0..<bands).map { b in
                    let row = raw.baseAddress!.advanced(by: b * MemoryLayout.size(ofValue: out.band.0)).assumingMemoryBound(to: CChar.self)
                    return String(cString: row)
                }
            }
            let cells: [[Double]] = withUnsafeBytes(of: out.cells) { raw in
                let all = raw.bindMemory(to: Double.self)
                return (0..<bands).map { b in (0..<hours).map { all[b * Int(HCP_PATH_HOURS) + $0] } }
            }
            return PathGrid(bands: names, cells: cells, best: cText(out.best), start: cText(out.start), km: out.distance_km)
        }
    }
}
