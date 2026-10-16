import SwiftUI

/// World map: shared coastline table (C++ core) drawn in an equirectangular Canvas, with the
/// day/night terminator from the shared solar position code and the operator's station.
/// Everything is computed on device from the clock, so it has no Stale/Offline state.
/// Overlay data (quakes, satellites) is added here only while its own store is live.
struct MapCard: View {
    @EnvironmentObject var station: StationStore
    @EnvironmentObject var quakes: QuakeStore
    @EnvironmentObject var sats: SatelliteStore
    let now: Date
    /// Select on the Siri Remote toggles between the whole world and a regional view
    /// centred on the station (the "DE" preset); without a station it stays on the world.
    enum View_: Int { case world, de, dx }
    @State private var view = View_.world

    /// Visible lon/lat window: (west, east, south, north) in degrees, centred on `station`
    /// (the DE or DX point); nil gives the whole world.
    static func window(regional: Bool, station: (lat: Double, lon: Double)?) -> (w: Double, e: Double, s: Double, n: Double) {
        guard regional, let st = station else { return (-180, 180, -90, 90) }
        let lon = st.lon > 180 ? st.lon - 360 : st.lon
        let halfH = 45.0, halfV = 22.5 // keeps the 2:1 aspect of the world view
        let cy = min(max(st.lat, -90 + halfV), 90 - halfV)
        return (lon - halfH, lon + halfH, cy - halfV, cy + halfV)
    }

    private static let coast: [[CGPoint]] = {
        var n = 0
        guard let pts = hcp_coast_points(&n) else { return [] }
        return (0..<Int(hcp_coast_part_count())).compactMap { i in
            var start: UInt32 = 0, count: UInt32 = 0
            guard hcp_coast_part(Int32(i), &start, &count) != 0 else { return nil }
            return (0..<Int(count)).map { k in
                let j = Int(start) + k
                return CGPoint(x: Double(pts[2 * j]), y: Double(pts[2 * j + 1]))
            }
        }
    }()

    /// Night-side polygon (lon/lat) from the subsolar point: terminator lat = atan(-cos(dλ)/tan(φ0)).
    static func nightPolygon(subLat: Double, subLon: Double) -> [CGPoint] {
        let phi0 = max(min(subLat, 89.9), -89.9) * .pi / 180
        var edge: [CGPoint] = []
        for lon in stride(from: -180.0, through: 180.0, by: 2.0) {
            let d = (lon - subLon) * .pi / 180
            edge.append(CGPoint(x: lon, y: atan(-cos(d) / tan(phi0)) * 180 / .pi))
        }
        let pole: Double = subLat >= 0 ? -90 : 90 // night lies away from the Sun
        return edge + [CGPoint(x: 180, y: pole), CGPoint(x: -180, y: pole)]
    }

    /// A ring can be filled when it is closed and has no antimeridian jump between neighbours.
    static func isFillable(_ part: [CGPoint]) -> Bool {
        guard part.count >= 4, let a = part.first, let b = part.last,
              abs(a.x - b.x) < 1e-3, abs(a.y - b.y) < 1e-3 else { return false }
        return !zip(part, part.dropFirst()).contains { abs($0.x - $1.x) >= 180 }
    }

    private static func subsolar(_ now: Date) -> (lat: Double, lon: Double)? {
        var lat = 0.0, lon = 0.0
        return hcp_subsolar_point(Int64(now.timeIntervalSince1970), &lat, &lon) != 0 ? (lat, lon) : nil
    }

    var body: some View {
        let sub = Self.subsolar(now)
        // Overlays appear only while their data is fresh enough to be honest about.
        let quakeState = SpaceWeatherStore.freshness(quakes.fetched.map { Sample(value: 0, observed: $0, fetched: $0) }, now: now)
        let quakePts = quakeState == .live ? quakes.all : []
        let satPts = sats.fetched.map { now.timeIntervalSince($0) <= 86400 * 2 } == true
            ? sats.subPointsCached(at: now) : []
        let centre = view == .dx ? station.dxPosition : (view == .de ? station.position : nil)
        let win = Self.window(regional: centre != nil, station: centre.map { ($0.lat, $0.lon) })
        VStack(alignment: .leading, spacing: 12) {
            Text(win.e - win.w < 360 ? (view == .dx ? "Regional map (around DX target)" : "Regional map (around your station)") : "World map").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            Canvas { ctx, size in
                func pt(_ p: CGPoint) -> CGPoint {
                    // Wrap longitude into the window so a view straddling +-180 still draws.
                    var x = p.x
                    let mid = (win.w + win.e) / 2
                    while x - mid > 180 { x -= 360 }
                    while x - mid < -180 { x += 360 }
                    return CGPoint(x: (x - win.w) / (win.e - win.w) * size.width,
                                   y: (win.n - p.y) / (win.n - win.s) * size.height)
                }
                if let sub {
                    // Raw linear mapping (no wrap); draw the polygon at -360/0/+360 so any window is covered.
                    func lin(_ p: CGPoint, _ off: Double) -> CGPoint {
                        CGPoint(x: (p.x + off - win.w) / (win.e - win.w) * size.width,
                                y: (win.n - p.y) / (win.n - win.s) * size.height)
                    }
                    let base = Self.nightPolygon(subLat: sub.lat, subLon: sub.lon)
                    for off in [-360.0, 0, 360] {
                        var night = Path()
                        night.addLines(base.map { lin($0, off) })
                        night.closeSubpath()
                        ctx.fill(night, with: .color(.black.opacity(0.55)))
                    }
                }
                // Land fill: only closed rings that never cross the antimeridian or a window edge
                // (Antarctica and the Russian Far East stay outline-only rather than filled wrongly).
                var fill = Path()
                for part in Self.coast where Self.isFillable(part) {
                    let pts = part.map(pt)
                    if zip(pts, pts.dropFirst()).contains(where: { abs($0.x - $1.x) > size.width / 2 }) { continue }
                    fill.addLines(pts); fill.closeSubpath()
                }
                ctx.fill(fill, with: .color(Color(red: 0.05, green: 0.2, blue: 0.1)))
                var land = Path()
                for part in Self.coast {
                    // Skip segments that wrap across the antimeridian.
                    for (a, b) in zip(part, part.dropFirst()) where abs(a.x - b.x) < 180 {
                        let pa = pt(a), pb = pt(b)
                        // Drop segments that wrap across the edge of a regional window.
                        if abs(pa.x - pb.x) > size.width / 2 { continue }
                        land.move(to: pa); land.addLine(to: pb)
                    }
                }
                ctx.stroke(land, with: .color(.green.opacity(0.85)), lineWidth: 2)
                for q in quakePts {
                    let r = 4 + 3 * max(0, q.magnitude - 2.5)
                    let s = pt(CGPoint(x: q.lon, y: q.lat))
                    ctx.stroke(Path(ellipseIn: CGRect(x: s.x - r, y: s.y - r, width: 2 * r, height: 2 * r)),
                               with: .color(.orange.opacity(0.9)), lineWidth: 2)
                }
                for p in satPts {
                    let s = pt(p)
                    ctx.fill(Path(CGRect(x: s.x - 3, y: s.y - 3, width: 6, height: 6)), with: .color(.cyan))
                }
                if let sub {
                    let s = pt(CGPoint(x: sub.lon, y: sub.lat))
                    ctx.fill(Path(ellipseIn: CGRect(x: s.x - 14, y: s.y - 14, width: 28, height: 28)),
                             with: .color(.yellow))
                }
                if let p = station.position {
                    let s = pt(CGPoint(x: p.lon > 180 ? p.lon - 360 : p.lon, y: p.lat))
                    ctx.stroke(Path(ellipseIn: CGRect(x: s.x - 12, y: s.y - 12, width: 24, height: 24)),
                               with: .color(.red), lineWidth: 5)
                }
                if let p = station.dxPosition {
                    let s = pt(CGPoint(x: p.lon > 180 ? p.lon - 360 : p.lon, y: p.lat))
                    ctx.stroke(Path(ellipseIn: CGRect(x: s.x - 12, y: s.y - 12, width: 24, height: 24)),
                               with: .color(.white), lineWidth: 5)
                }
            }
            .aspectRatio(2, contentMode: .fit)
            .background(Color(red: 0.02, green: 0.07, blue: 0.14))
            Text("Yellow: subsolar point. Red: your station. White: DX target. Orange rings: USGS M2.5+ quakes (shown only while live). Cyan: amateur satellites (CelesTrak, hidden if elements are over 2 days old). Land fill and coastlines: Natural Earth (public domain). Computed on device. Select cycles World / DE / DX views (DE and DX need their locators set).")
                .font(.system(size: 29)).foregroundStyle(.secondary)
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
        .onTapGesture {
            // Cycle World -> DE -> DX, skipping views whose locator is not set.
            var next = view
            for _ in 0..<3 {
                next = View_(rawValue: (next.rawValue + 1) % 3)!
                if next == .world || (next == .de && station.position != nil) || (next == .dx && station.dxPosition != nil) { break }
            }
            view = next
        }
    }
}
