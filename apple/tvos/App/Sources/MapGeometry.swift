import CoreGraphics
import Foundation

/// Map overlays the operator can switch on or off (MapMenu.qml, Settings > Map).
enum MapLayer: String, CaseIterable, Identifiable {
    case night, grid, borders, path, sun, quakes, storms, satellites, ota, fires, launches, planets, aurora, drap
    var id: String { rawValue }
    var title: String {
        switch self {
        case .night: return "Day/night"
        case .grid: return "Lat/lon grid"
        case .borders: return "Borders"
        case .path: return "DE–DX path"
        case .sun: return "Sun"
        case .quakes: return "Earthquakes"
        case .storms: return "Tropical storms"
        case .satellites: return "Satellites"
        case .ota: return "POTA/SOTA/WWFF"
        case .fires: return "Fires"
        case .launches: return "Launch pads"
        case .planets: return "Planets"
        case .aurora: return "Aurora"
        case .drap: return "D-RAP absorption"
        }
    }
    static let defaultOn: Set<String> = ["night", "borders", "path", "sun", "quakes", "storms", "satellites", "ota", "planets"]
}

/// Static map tables from the shared core (Natural Earth land, coastline, borders) as CGPaths
/// in degree space: x = longitude, y = -latitude. Rings that cross the antimeridian are
/// unwrapped (consecutive longitudes kept within 180 degrees), so the renderer draws each path
/// at -360/0/+360 offsets and every part lands on screen whole.
enum MapGeometry {
    static let land: CGPath = build(count: Int(hcp_land_ring_count()), part: { i in
        var s: UInt32 = 0, c: UInt32 = 0
        return hcp_land_ring(Int32(i), &s, &c) != 0 ? (s, c) : nil
    }, points: { n in hcp_land_points(&n) }, close: true)

    static let coast: CGPath = build(count: Int(hcp_coast_part_count()), part: { i in
        var s: UInt32 = 0, c: UInt32 = 0
        return hcp_coast_part(Int32(i), &s, &c) != 0 ? (s, c) : nil
    }, points: { n in hcp_coast_points(&n) }, close: false)

    static let borders: CGPath = build(count: Int(hcp_border_part_count()), part: { i in
        var s: UInt32 = 0, c: UInt32 = 0
        return hcp_border_part(Int32(i), &s, &c) != 0 ? (s, c) : nil
    }, points: { n in hcp_border_points(&n) }, close: false)

    private static func build(count: Int, part: (Int) -> (UInt32, UInt32)?,
                              points: (inout Int) -> UnsafePointer<Float>?, close: Bool) -> CGPath {
        var n = 0
        let path = CGMutablePath()
        guard let pts = points(&n) else { return path }
        for i in 0..<count {
            guard let (start, cnt) = part(i), cnt > 1, Int(start) + Int(cnt) <= n else { continue }
            var prev = Double(pts[2 * Int(start)])
            for k in 0..<Int(cnt) {
                let j = Int(start) + k
                var lon = Double(pts[2 * j])
                let lat = Double(pts[2 * j + 1])
                while lon - prev > 180 { lon -= 360 }
                while lon - prev < -180 { lon += 360 }
                prev = lon
                let p = CGPoint(x: lon, y: -lat)
                if k == 0 { path.move(to: p) } else { path.addLine(to: p) }
            }
            if close { path.closeSubpath() }
        }
        return path
    }

    /// Night side from the subsolar point: terminator latitude = atan(-cos(dλ)/tan(φ0)).
    static func night(subLat: Double, subLon: Double) -> CGPath {
        let phi0 = max(min(subLat, 89.9), -89.9) * .pi / 180
        let path = CGMutablePath()
        for (i, lon) in stride(from: -180.0, through: 180.0, by: 2).enumerated() {
            let lat = atan(-cos((lon - subLon) * .pi / 180) / tan(phi0)) * 180 / .pi
            let p = CGPoint(x: lon, y: -lat)
            if i == 0 { path.move(to: p) } else { path.addLine(to: p) }
        }
        let pole: Double = subLat >= 0 ? 90 : -90 // night lies away from the Sun (y = -lat)
        path.addLine(to: CGPoint(x: 180, y: pole))
        path.addLine(to: CGPoint(x: -180, y: pole))
        path.closeSubpath()
        return path
    }

    /// Great-circle points from DE to DX (short or long path) as (lon, lat) samples.
    static func greatCircle(_ a: (lat: Double, lon: Double), _ b: (lat: Double, lon: Double), longPath: Bool) -> [CGPoint] {
        func vec(_ p: (lat: Double, lon: Double)) -> (Double, Double, Double) {
            let la = p.lat * .pi / 180, lo = p.lon * .pi / 180
            return (cos(la) * cos(lo), cos(la) * sin(lo), sin(la))
        }
        let u = vec(a), v = vec(b)
        let dot = max(-1, min(1, u.0 * v.0 + u.1 * v.1 + u.2 * v.2))
        let omega = acos(dot)
        guard omega > 1e-6, abs(omega - .pi) > 1e-6 else { return [] }
        let total = longPath ? omega - 2 * .pi : omega
        let steps = 180
        return (0...steps).map { i in
            let t = total * Double(i) / Double(steps)
            let s = sin(omega)
            let k1 = sin(omega - t) / s, k2 = sin(t) / s
            let x = k1 * u.0 + k2 * v.0, y = k1 * u.1 + k2 * v.1, z = k1 * u.2 + k2 * v.2
            return CGPoint(x: atan2(y, x) * 180 / .pi, y: atan2(z, (x * x + y * y).squareRoot()) * 180 / .pi)
        }
    }

    /// Visible window (west, east, south, north) for a centre and zoom on a canvas of `aspect` (w/h).
    static func window(centre: (lat: Double, lon: Double)?, zoom: Double, aspect: Double) -> (w: Double, e: Double, s: Double, n: Double) {
        let lonSpan = 360 / max(1, zoom)
        let latSpan = min(180, lonSpan / max(aspect, 0.5))
        let cLon = centre?.lon ?? 0
        let cLat = min(max(centre?.lat ?? 12, -90 + latSpan / 2), 90 - latSpan / 2)
        return (cLon - lonSpan / 2, cLon + lonSpan / 2, cLat - latSpan / 2, cLat + latSpan / 2)
    }
}
