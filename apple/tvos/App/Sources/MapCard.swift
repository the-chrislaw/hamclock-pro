import SwiftUI

/// World map: shared coastline table (C++ core) drawn in an equirectangular Canvas, with the
/// day/night terminator from the shared solar position code and the operator's station.
/// Everything is computed on device from the clock, so it has no Stale/Offline state.
/// Overlay data (quakes, satellites) is added here only while its own store is live.
struct MapCard: View {
    @EnvironmentObject var station: StationStore
    let now: Date

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

    private static func subsolar(_ now: Date) -> (lat: Double, lon: Double)? {
        var lat = 0.0, lon = 0.0
        return hcp_subsolar_point(Int64(now.timeIntervalSince1970), &lat, &lon) != 0 ? (lat, lon) : nil
    }

    var body: some View {
        let sub = Self.subsolar(now)
        VStack(alignment: .leading, spacing: 12) {
            Text("World map").font(.system(size: 36, weight: .medium)).foregroundStyle(.secondary)
            Canvas { ctx, size in
                func pt(_ p: CGPoint) -> CGPoint {
                    CGPoint(x: (p.x + 180) / 360 * size.width, y: (90 - p.y) / 180 * size.height)
                }
                if let sub {
                    var night = Path()
                    let poly = Self.nightPolygon(subLat: sub.lat, subLon: sub.lon).map(pt)
                    night.addLines(poly)
                    night.closeSubpath()
                    ctx.fill(night, with: .color(.black.opacity(0.55)))
                }
                var land = Path()
                for part in Self.coast {
                    // Skip segments that wrap across the antimeridian.
                    for (a, b) in zip(part, part.dropFirst()) where abs(a.x - b.x) < 180 {
                        land.move(to: pt(a)); land.addLine(to: pt(b))
                    }
                }
                ctx.stroke(land, with: .color(.green.opacity(0.85)), lineWidth: 2)
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
            }
            .aspectRatio(2, contentMode: .fit)
            .background(Color(red: 0.02, green: 0.07, blue: 0.14))
            Text("Yellow: subsolar point. Red: your station. Coastlines: Natural Earth (public domain). Computed on device.")
                .font(.system(size: 29)).foregroundStyle(.secondary)
        }
        .padding(32)
        .frame(maxWidth: .infinity, alignment: .leading)
        .background(RoundedRectangle(cornerRadius: 24).fill(Color(white: 0.12)))
        .focusable()
    }
}
