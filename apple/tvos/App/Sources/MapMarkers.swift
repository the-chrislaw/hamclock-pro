import SwiftUI

/// Point overlays on the world map (marker shapes and colours after the main app's map).
@MainActor
struct MapMarkers {
    var ctx: GraphicsContext
    let theme: Theme
    let now: Date
    let pt: (Double, Double) -> CGPoint
    let settings: AppSettings
    let feeds: Feeds
    let sats: SatelliteStore
    let sub: (lat: Double, lon: Double)?
    let longPath: Bool
    var placed: [CGRect] = []

    private static let quake = Theme.hex(0xe0473c), storm = Theme.hex(0xf0a030), ota = Theme.hex(0xf5b400)
    private static let sat = Theme.hex(0x4fc3e0), fire = Theme.hex(0xff7a2f), pad = Theme.hex(0xd0d6e2)
    private static let planet = Theme.hex(0xf4e7b4)

    private func live(_ s: WidgetState) -> Bool { s == .live || s == .stale }

    mutating func draw() {
        // DE and DX labels are drawn last but always win: keep their places free for them.
        for (p, text) in [(settings.de, "DE"), (settings.dx, "DX")] {
            guard let p else { continue }
            let at = pt(p.lon, p.lat)
            placed.append(CGRect(x: at.x - 14, y: at.y - 16, width: 34 + CGFloat(text.count) * 18, height: 32))
        }
        if settings.isOn(.path), let de = settings.de, let dx = settings.dx { path(de, dx) }
        if settings.isOn(.fires), live(feeds.fires.state(now)) {
            for f in (feeds.fires.value ?? []).prefix(1500) { dot(pt(f.lon, f.lat), 3, Self.fire.opacity(0.8)) }
        }
        if settings.isOn(.ota) {
            let spots = feeds.otaSpots(now).filter { $0.position != nil }
            for s in spots.prefix(60) { diamond(pt(s.position!.lon, s.position!.lat), 8, Self.ota) }
            for s in spots.prefix(12) { label(s.call, at: pt(s.position!.lon, s.position!.lat), dx: 12) }
        }
        if settings.isOn(.satellites), sats.fetched.map({ now.timeIntervalSince($0) < SatelliteStore.offlineAfter }) == true {
            for p in sats.subPointsCached(at: now) { triangle(pt(p.x, p.y), 5, Self.sat.opacity(0.85)) }
        }
        if settings.isOn(.launches), live(feeds.launches.state(now)) {
            for l in (feeds.launches.value ?? []).prefix(10) where l.has_pad != 0 { square(pt(l.pad_lon, l.pad_lat), 7, Self.pad) }
        }
        if settings.isOn(.storms), live(feeds.storms.state(now)) {
            for s in feeds.storms.value ?? [] {
                let p = pt(s.lon, s.lat)
                diamond(p, 14, Self.storm)
                label(s.name, at: p, dx: 18)
            }
        }
        if settings.isOn(.quakes), live(feeds.quakes.state(now)) {
            for q in feeds.quakes.value ?? [] {
                let p = pt(q.lon, q.lat)
                dot(p, 4 + 3.2 * max(0, q.magnitude - 2.5), Self.quake.opacity(q.magnitude >= 5 ? 0.95 : 0.7))
                if q.magnitude >= 5.5 { label(String(format: "M%.1f", q.magnitude), at: p, dx: 6 + 3.2 * (q.magnitude - 2.5)) }
            }
        }
        if settings.isOn(.planets), let de = settings.de { planets(de) }
        if settings.isOn(.sun), let sub {
            let p = pt(sub.lon, sub.lat)
            dot(p, 15, Theme.hex(0xffd23f))
            ctx.stroke(Path(ellipseIn: CGRect(x: p.x - 15, y: p.y - 15, width: 30, height: 30)), with: .color(.black.opacity(0.5)), lineWidth: 2)
        }
        if let de = settings.de { station(pt(de.lon, de.lat), "DE", Theme.hex(0xff5a4e)) }
        if let dx = settings.dx, settings.de.map({ $0.lat != dx.lat || $0.lon != dx.lon }) ?? false {
            station(pt(dx.lon, dx.lat), "DX", Theme.hex(0x6fb3ff))
        }
    }

    private mutating func path(_ de: (lat: Double, lon: Double), _ dx: (lat: Double, lon: Double)) {
        let pts = MapGeometry.greatCircle(de, dx, longPath: longPath).map { pt($0.x, $0.y) }
        var p = Path()
        for (i, q) in pts.enumerated() {
            if i == 0 || abs(q.x - pts[i - 1].x) > 400 { p.move(to: q) } else { p.addLine(to: q) }
        }
        ctx.stroke(p, with: .color(theme.route), style: StrokeStyle(lineWidth: 3.5, lineCap: .round, dash: longPath ? [12, 8] : []))
    }

    private mutating func planets(_ de: (lat: Double, lon: Double)) {
        for i in 0..<5 {
            var p = HcpPlanet()
            guard hcp_planet(Int32(i), Int64(now.timeIntervalSince1970), de.lat, de.lon, &p) != 0 else { continue }
            let at = pt(p.sub_lon, p.sub_lat)
            dot(at, 7, Self.planet)
            label(cText(p.name), at: at, dx: 12)
        }
    }

    private mutating func station(_ p: CGPoint, _ text: String, _ color: Color) {
        ctx.fill(Path(ellipseIn: CGRect(x: p.x - 13, y: p.y - 13, width: 26, height: 26)), with: .color(color))
        ctx.stroke(Path(ellipseIn: CGRect(x: p.x - 13, y: p.y - 13, width: 26, height: 26)), with: .color(.white), lineWidth: 3)
        label(text, at: p, dx: 20, bold: true)
    }

    private mutating func dot(_ p: CGPoint, _ r: Double, _ c: Color) {
        ctx.fill(Path(ellipseIn: CGRect(x: p.x - r, y: p.y - r, width: 2 * r, height: 2 * r)), with: .color(c))
    }

    private mutating func diamond(_ p: CGPoint, _ r: Double, _ c: Color) {
        var d = Path()
        d.move(to: CGPoint(x: p.x, y: p.y - r)); d.addLine(to: CGPoint(x: p.x + r, y: p.y))
        d.addLine(to: CGPoint(x: p.x, y: p.y + r)); d.addLine(to: CGPoint(x: p.x - r, y: p.y)); d.closeSubpath()
        ctx.fill(d, with: .color(c))
    }

    private mutating func triangle(_ p: CGPoint, _ r: Double, _ c: Color) {
        var d = Path()
        d.move(to: CGPoint(x: p.x, y: p.y - r)); d.addLine(to: CGPoint(x: p.x + r, y: p.y + r * 0.8))
        d.addLine(to: CGPoint(x: p.x - r, y: p.y + r * 0.8)); d.closeSubpath()
        ctx.fill(d, with: .color(c))
    }

    private mutating func square(_ p: CGPoint, _ r: Double, _ c: Color) {
        ctx.fill(Path(CGRect(x: p.x - r, y: p.y - r, width: 2 * r, height: 2 * r)), with: .color(c))
    }

    /// Labels never overlap: one that would collide with an earlier label is skipped
    /// (DE/DX are drawn last but always shown).
    private mutating func label(_ text: String, at p: CGPoint, dx: Double, bold: Bool = false) {
        let t = ctx.resolve(Text(text).font(.system(size: 24, weight: bold ? .bold : .medium)).foregroundColor(theme.mapInk))
        let size = t.measure(in: CGSize(width: 400, height: 60))
        let rect = CGRect(x: p.x + dx, y: p.y - size.height / 2, width: size.width, height: size.height).insetBy(dx: -4, dy: -2)
        if !bold, placed.contains(where: { $0.intersects(rect) }) { return }
        placed.append(rect)
        ctx.draw(t, at: CGPoint(x: p.x + dx, y: p.y), anchor: .leading)
    }
}
