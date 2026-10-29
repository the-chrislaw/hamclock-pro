import SwiftUI

/// Map view state chosen from the map menu: centre (world / DE / DX) and zoom 1–4x.
struct MapView: Equatable {
    enum Centre: String, CaseIterable { case world, de, dx }
    var centre = Centre.world
    var zoom = 1.0
    var longPath = false
}

/// Equirectangular world map drawn with Canvas from the shared Natural Earth tables
/// (src/map/land.hpp, coastline.hpp, borders.hpp via hcpcore.h), in the main app's map palette.
/// Overlays draw only while their source is live or stale-but-recent, never from offline data.
struct WorldMapView: View {
    @EnvironmentObject var settings: AppSettings
    @EnvironmentObject var feeds: Feeds
    @EnvironmentObject var sats: SatelliteStore
    @Environment(\.theme) private var theme
    let now: Date
    let view: MapView


    private var centre: MapCentre? {
        let c = view.centre == .de ? settings.de : view.centre == .dx ? settings.dx : nil
        return c.map { MapCentre(lat: $0.lat, lon: $0.lon) }
    }

    var body: some View {
        ZStack {
            // Land, coast, borders and grid only change with the view or the theme: drawn once,
            // not on every clock tick.
            BaseMap(centre: centre, zoom: view.zoom, theme: theme, borders: settings.isOn(.borders), grid: settings.isOn(.grid))
                .equatable()
            Canvas { ctx, size in draw(&ctx, size) }
        }
        .background(theme.sea)
        .clipShape(RoundedRectangle(cornerRadius: 10))
        .accessibilityIdentifier("worldMap")
    }

    private func usable(_ s: WidgetState) -> Bool { s == .live || s == .stale }

    /// Time-dependent layers: rasters, night side, markers.
    private func draw(_ ctx: inout GraphicsContext, _ size: CGSize) {
        let proj = MapProjection(size: size, centre: centre, zoom: view.zoom)
        if settings.isOn(.drap), let d = feeds.drap.value, usable(feeds.drap.state(now)) {
            drawImage(&ctx, MapRaster.drap(d.grid), lonW: -180, transform: proj.transform)
        }
        if settings.isOn(.aurora), let a = feeds.aurora.value, usable(feeds.aurora.state(now)) {
            drawImage(&ctx, MapRaster.aurora(a.grid), lonW: -180, transform: proj.transform)
        }
        var sub = (lat: 0.0, lon: 0.0)
        let hasSun = hcp_subsolar_point(Int64(now.timeIntervalSince1970), &sub.lat, &sub.lon) != 0
        if settings.isOn(.night), hasSun {
            let night = MapGeometry.night(subLat: sub.lat, subLon: sub.lon)
            for off in MapProjection.offsets {
                ctx.fill(Path(night).applying(proj.transform(off)), with: .color(.black.opacity(theme.dark ? 0.36 : 0.22)))
            }
        }
        var markers = MapMarkers(ctx: ctx, theme: theme, now: now, pt: proj.point, settings: settings, feeds: feeds, sats: sats,
                                 sub: hasSun ? sub : nil, longPath: view.longPath)
        markers.draw()
    }


    private func drawImage(_ ctx: inout GraphicsContext, _ image: CGImage?, lonW: Double,
                           transform: (Double) -> CGAffineTransform) {
        guard let image else { return }
        for off in MapProjection.offsets {
            let rect = CGRect(x: lonW, y: -90, width: 360, height: 180).applying(transform(off))
            ctx.draw(Image(decorative: image, scale: 1), in: rect)
        }
    }
}

struct MapCentre: Equatable { var lat: Double; var lon: Double }

/// Equirectangular projection of the visible window onto a canvas of `size`.
struct MapProjection {
    static let offsets = [-360.0, 0, 360]
    let win: (w: Double, e: Double, s: Double, n: Double)
    let size: CGSize
    private let sx: Double, sy: Double

    init(size: CGSize, centre: MapCentre?, zoom: Double) {
        self.size = size
        win = MapGeometry.window(centre: centre.map { ($0.lat, $0.lon) }, zoom: zoom, aspect: size.width / max(size.height, 1))
        sx = size.width / (win.e - win.w)
        sy = size.height / (win.n - win.s)
    }

    /// Degree space (x = lon, y = -lat) shifted by `off` degrees of longitude, to screen.
    func transform(_ off: Double) -> CGAffineTransform {
        CGAffineTransform(a: sx, b: 0, c: 0, d: sy, tx: (off - win.w) * sx, ty: win.n * sy)
    }

    /// A point, wrapped in longitude into the window.
    func point(_ lon: Double, _ lat: Double) -> CGPoint {
        let mid = (win.w + win.e) / 2
        var x = lon
        while x - mid > 180 { x -= 360 }
        while x - mid < -180 { x += 360 }
        return CGPoint(x: (x - win.w) * sx, y: (win.n - lat) * sy)
    }
}

/// The static map: land, coastline, borders, grid. Equatable, so the 1 Hz clock does not redraw it.
struct BaseMap: View, Equatable {
    let centre: MapCentre?
    let zoom: Double
    let theme: Theme
    let borders: Bool
    let grid: Bool

    var body: some View {
        Canvas { ctx, size in
            let proj = MapProjection(size: size, centre: centre, zoom: zoom)
            for off in MapProjection.offsets {
                let t = proj.transform(off)
                ctx.fill(Path(MapGeometry.land).applying(t), with: .color(theme.land))
                ctx.stroke(Path(MapGeometry.coast).applying(t), with: .color(theme.coast), lineWidth: 1.5)
                if borders { ctx.stroke(Path(MapGeometry.borders).applying(t), with: .color(theme.coast.opacity(0.55)), lineWidth: 1) }
            }
            if grid { drawGrid(&ctx, proj) }
        }
    }

    private func drawGrid(_ ctx: inout GraphicsContext, _ proj: MapProjection) {
        var p = Path()
        for lat in stride(from: -60.0, through: 60, by: 30) {
            let y = proj.point(0, lat).y
            p.move(to: CGPoint(x: 0, y: y)); p.addLine(to: CGPoint(x: proj.size.width, y: y))
        }
        var lon = (proj.win.w / 30).rounded(.up) * 30
        while lon <= proj.win.e {
            let x = (lon - proj.win.w) * proj.size.width / (proj.win.e - proj.win.w)
            p.move(to: CGPoint(x: x, y: 0)); p.addLine(to: CGPoint(x: x, y: proj.size.height))
            lon += 30
        }
        ctx.stroke(p, with: .color(theme.mapInk.opacity(0.3)), lineWidth: 1)
    }
}

/// Raster overlays (aurora probability, D-RAP MHz) as world images, west edge at -180.
enum MapRaster {
    private static var auroraCache: (key: Int, image: CGImage?)?
    private static var drapCache: (key: Int, image: CGImage?)?

    static func aurora(_ grid: [UInt8]) -> CGImage? {
        let key = grid.reduce(0) { $0 &* 31 &+ Int($1) }
        if let c = auroraCache, c.key == key { return c.image }
        // 360 x 181 cells, index (lat+90)*360 + lon(0..359); image rows north first, columns from -180.
        let img = make(width: 360, height: 181) { x, y in
            let lat = 90 - y, lon = (x + 180) % 360
            let p = Double(grid[(lat + 90) * 360 + lon])
            guard p >= 3 else { return nil }
            let a = min(1, p / 30)
            return p < 30 ? (0.25, 0.9, 0.35, a * 0.8) : p < 60 ? (0.95, 0.85, 0.2, 0.85) : (0.95, 0.3, 0.25, 0.9)
        }
        auroraCache = (key, img)
        return img
    }

    static func drap(_ grid: [Float]) -> CGImage? {
        let key = grid.reduce(0) { $0 &* 31 &+ Int($1 * 10) }
        if let c = drapCache, c.key == key { return c.image }
        // 90 x 90: row 0 = lat 89 (2 degree steps), col 0 = lon -178 (4 degree steps).
        let img = make(width: 90, height: 90) { x, y in
            let mhz = Double(grid[y * 90 + x])
            guard mhz >= 1 else { return nil }
            let t = min(1, mhz / 35)
            return (0.9, 0.55 - 0.4 * t, 0.2, 0.25 + 0.5 * t)
        }
        drapCache = (key, img)
        return img
    }

    private static func make(width: Int, height: Int, _ pixel: (Int, Int) -> (Double, Double, Double, Double)?) -> CGImage? {
        var bytes = [UInt8](repeating: 0, count: width * height * 4)
        for y in 0..<height {
            for x in 0..<width {
                guard let (r, g, b, a) = pixel(x, y) else { continue }
                let i = (y * width + x) * 4
                bytes[i] = UInt8(r * a * 255); bytes[i + 1] = UInt8(g * a * 255)
                bytes[i + 2] = UInt8(b * a * 255); bytes[i + 3] = UInt8(a * 255)
            }
        }
        guard let provider = CGDataProvider(data: Data(bytes) as CFData) else { return nil }
        return CGImage(width: width, height: height, bitsPerComponent: 8, bitsPerPixel: 32, bytesPerRow: width * 4,
                       space: CGColorSpaceCreateDeviceRGB(), bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue),
                       provider: provider, decode: nil, shouldInterpolate: true, intent: .defaultIntent)
    }
}
