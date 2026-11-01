import Foundation

struct Reading: Equatable {
    var value: Double
    var time: Date
}

struct QuakeItem: Identifiable, Equatable {
    var id: String { "\(time)-\(lat)-\(lon)" }
    var time: Date
    var magnitude: Double
    var place: String
    var lat: Double
    var lon: Double
}

struct StormItem: Identifiable, Equatable {
    var id: String { name }
    var name: String
    var classification: String
    var heading: String
    var windKt: Int
    var pressureMb: Int
    var category: Int
    var speedKt: Int
    var lat: Double
    var lon: Double
}

struct OtaSpot: Identifiable, Equatable {
    var id: String { program + call + reference }
    var program, call, reference, name, mode, comment: String
    var freqKhz: Double
    var time: Date
    var position: (lat: Double, lon: Double)?
    static func == (a: OtaSpot, b: OtaSpot) -> Bool { a.id == b.id && a.time == b.time }
}

struct AuroraData { var info: HcpAurora; var grid: [UInt8] }
struct DrapData { var info: HcpDrap; var grid: [Float] }

/// Every polled source on TV. Swift fetches; the Qt-free C++ core (hcpcore.h) parses.
/// URLs and cadences follow the Qt widget models (src/widgets/*/…_model.cpp).
@MainActor
final class Feeds: ObservableObject {
    private static let swpc = "https://services.swpc.noaa.gov"
    private static func minutes(_ m: Double) -> TimeInterval { m * 60 }
    private static func hours(_ h: Double) -> TimeInterval { h * 3600 }

    let kp = PolledFeed(["\(swpc)/products/noaa-planetary-k-index.json"], source: "NOAA SWPC",
                        policy: .init(interval: minutes(15), staleAfter: hours(4), offlineAfter: hours(12))) {
        Feeds.series($0, cap: 64, hcp_swpc_kp_series)
    }
    let flux = PolledFeed(["\(swpc)/products/summary/10cm-flux.json"], source: "NOAA SWPC",
                          policy: .init(interval: hours(6), staleAfter: hours(36), offlineAfter: hours(72))) { data -> Reading? in
        let r = data.withC(hcp_swpc_latest_solar_flux)
        return r.valid != 0 ? Reading(value: r.value, time: Date(timeIntervalSince1970: Double(r.time_ms) / 1000)) : nil
    }
    let xray = PolledFeed(["\(swpc)/json/goes/primary/xrays-6-hour.json"], source: "NOAA SWPC GOES",
                          policy: .init(interval: minutes(5), staleAfter: minutes(30), offlineAfter: hours(3))) {
        Feeds.series($0, cap: 400, hcp_swpc_xray_series)
    }
    let wind = PolledFeed(["\(swpc)/json/rtsw/rtsw_wind_1m.json"], source: "NOAA SWPC RTSW",
                          policy: .init(interval: minutes(2), staleAfter: minutes(15), offlineAfter: hours(2))) { Feeds.windPair($0, 0, 1) }
    let mag = PolledFeed(["\(swpc)/json/rtsw/rtsw_mag_1m.json"], source: "NOAA SWPC RTSW",
                         policy: .init(interval: minutes(2), staleAfter: minutes(15), offlineAfter: hours(2))) { Feeds.windPair($0, 2, 3) }
    let scales = PolledFeed(["\(swpc)/products/noaa-scales.json"], source: "NOAA SWPC",
                            policy: .init(interval: minutes(15), staleAfter: hours(2), offlineAfter: hours(12))) { data -> HcpScales? in
        var s = HcpScales()
        return data.withC { hcp_scales_parse($0, $1, &s) } != 0 ? s : nil
    }
    /// Kyoto Dst (SolarDstModel: 30 min interval, stale after 2 h).
    let dst = PolledFeed(["\(swpc)/products/kyoto-dst.json"], source: "NOAA SWPC Kyoto Dst",
                         policy: .init(interval: minutes(30), staleAfter: hours(2), offlineAfter: hours(12))) {
        Feeds.parseDst($0)
    }
    /// Alerts, watches and warnings (SolarAlertsModel: 10 min interval, stale after 2 h).
    let alerts = PolledFeed(["\(swpc)/products/alerts.json"], source: "NOAA SWPC alerts",
                            policy: .init(interval: minutes(10), staleAfter: hours(2), offlineAfter: hours(12))) {
        SpaceAlerts.parse($0)
    }
    /// Solar trend products (SolarTrendModel policies: 30 min / 6 h / 12 h, stale after 6 h / 36 h / 72 h).
    let kpForecast = PolledFeed(["\(swpc)/products/noaa-planetary-k-index-forecast.json"], source: "NOAA SWPC Kp forecast",
                                policy: .init(interval: minutes(30), staleAfter: hours(6), offlineAfter: hours(18))) {
        TrendParse.kp($0)
    }
    let fluxHistory = PolledFeed(["\(swpc)/json/f107_cm_flux.json"], source: "NOAA SWPC F10.7",
                                 policy: .init(interval: hours(6), staleAfter: hours(36), offlineAfter: hours(108))) {
        TrendParse.flux($0)
    }
    let sunspots = PolledFeed(["\(swpc)/json/solar-cycle/observed-solar-cycle-indices.json"], source: "NOAA SWPC SSN",
                              policy: .init(interval: hours(12), staleAfter: hours(72), offlineAfter: hours(216))) {
        TrendParse.sunspots($0)
    }
    /// N0NBH HamQSL solar XML (SolarHamqslModel: hourly, stale after 3 h).
    let hamqsl = PolledFeed(["https://www.hamqsl.com/solarxml.php"], source: "HamQSL",
                            policy: .init(interval: hours(1), staleAfter: hours(3), offlineAfter: hours(12))) {
        HamqslWidget.parse($0)
    }
    let aurora = PolledFeed(["\(swpc)/json/ovation_aurora_latest.json"], source: "NOAA SWPC OVATION",
                            policy: .init(interval: minutes(10), staleAfter: hours(1), offlineAfter: hours(6))) { data -> AuroraData? in
        var info = HcpAurora()
        var grid = [UInt8](repeating: 0, count: 360 * 181)
        let ok = grid.withUnsafeMutableBufferPointer { g in data.withC { hcp_aurora_parse($0, $1, &info, g.baseAddress) } }
        return ok != 0 ? AuroraData(info: info, grid: grid) : nil
    }
    let drap = PolledFeed(["\(swpc)/text/drap_global_frequencies.txt"], source: "NOAA SWPC D-RAP",
                          policy: .init(interval: minutes(10), staleAfter: hours(1), offlineAfter: hours(6))) { data -> DrapData? in
        var info = HcpDrap()
        var grid = [Float](repeating: 0, count: 90 * 90)
        let ok = grid.withUnsafeMutableBufferPointer { g in data.withC { hcp_drap_parse($0, $1, &info, g.baseAddress) } }
        return ok != 0 ? DrapData(info: info, grid: grid) : nil
    }
    let quakes = PolledFeed(["https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_day.geojson"],
                            source: "USGS", policy: .init(interval: minutes(5), staleAfter: hours(1), offlineAfter: hours(6))) {
        Feeds.parseQuakes($0)
    }
    let storms = PolledFeed(["https://www.nhc.noaa.gov/CurrentStorms.json"], source: "NOAA NHC",
                            policy: .init(interval: minutes(30), staleAfter: hours(6), offlineAfter: hours(24))) {
        Feeds.parseStorms($0)
    }
    let launches = PolledFeed(["https://ll.thespacedevs.com/2.2.0/launch/upcoming/?limit=10&mode=list"],
                              source: "Launch Library 2", policy: .init(interval: hours(1), staleAfter: hours(6), offlineAfter: hours(24))) { data -> [HcpLaunch]? in
        var out = [HcpLaunch](repeating: HcpLaunch(), count: 20)
        let n = data.withC { hcp_launches_parse($0, $1, &out, out.count) }
        return n >= 0 ? Array(out.prefix(Int(n))) : nil
    }
    let pota = PolledFeed(["https://api.pota.app/spot/activator"], source: "POTA",
                          policy: .init(interval: minutes(2), staleAfter: minutes(15), offlineAfter: hours(1))) {
        Feeds.parseOta($0, hcp_pota_parse)
    }
    let sota = PolledFeed(["https://api-db2.sota.org.uk/api/spots/50/all/all"], source: "SOTA",
                          policy: .init(interval: minutes(2), staleAfter: minutes(15), offlineAfter: hours(1))) {
        Feeds.parseOta($0, hcp_sota_parse)
    }
    let wwff = PolledFeed(["https://spots.wwff.co/static/spots.json"], source: "WWFF",
                          policy: .init(interval: minutes(2), staleAfter: minutes(15), offlineAfter: hours(1))) {
        Feeds.parseOta($0, hcp_wwff_parse)
    }
    let fires = PolledFeed(["https://firms.modaps.eosdis.nasa.gov/data/active_fire/noaa-20-viirs-c2/csv/J1_VIIRS_C2_USA_contiguous_and_Hawaii_24h.csv"],
                           source: "NASA FIRMS", policy: .init(interval: hours(1), staleAfter: hours(6), offlineAfter: hours(24))) { data -> [HcpFire]? in
        var out = [HcpFire](repeating: HcpFire(), count: Feeds.fireCap)
        let n = data.withC { hcp_fires_parse($0, $1, &out, out.count) }
        return n >= 0 ? Array(out.prefix(Int(n))) : nil
    }

    /// Starts every feed's refresh loop; they run while the app is in the foreground.
    func run() async {
        await withTaskGroup(of: Void.self) { group in
            group.addTask { await self.kp.run() }
            group.addTask { await self.flux.run() }
            group.addTask { await self.xray.run() }
            group.addTask { await self.wind.run() }
            group.addTask { await self.mag.run() }
            group.addTask { await self.scales.run() }
            group.addTask { await self.dst.run() }
            group.addTask { await self.alerts.run() }
            group.addTask { await self.kpForecast.run() }
            group.addTask { await self.fluxHistory.run() }
            group.addTask { await self.sunspots.run() }
            group.addTask { await self.hamqsl.run() }
            group.addTask { await self.aurora.run() }
            group.addTask { await self.drap.run() }
            group.addTask { await self.quakes.run() }
            group.addTask { await self.storms.run() }
            group.addTask { await self.launches.run() }
            group.addTask { await self.pota.run() }
            group.addTask { await self.sota.run() }
            group.addTask { await self.wwff.run() }
            group.addTask { await self.fires.run() }
        }
    }

    /// OTA spots from the feeds that are Live or Stale (never Offline), merged newest first,
    /// one per program+call+reference (Qt ota::merge). Cached until a feed refreshes.
    func otaSpots(_ now: Date) -> [OtaSpot] {
        let feeds = [pota, sota, wwff].filter { [.live, .stale].contains($0.state(now)) }
        let key = feeds.map { "\($0.source)\($0.status.fetched?.timeIntervalSince1970 ?? 0)" }.joined()
        if let c = otaCache, c.key == key { return c.spots }
        var seen = Set<String>()
        let spots = feeds.compactMap(\.value).flatMap { $0 }
            .sorted { $0.time > $1.time }
            .filter { seen.insert($0.id).inserted }
        otaCache = (key, spots)
        return spots
    }
    private var otaCache: (key: String, spots: [OtaSpot])?

    /// Rows the fires parser keeps; a fuller file is reported as "at least" this many.
    nonisolated static let fireCap = 20000

    // MARK: parsers (thin wrappers over the C ABI)

    nonisolated static func series(_ data: Data, cap: Int,
                                   _ parse: (UnsafePointer<CChar>?, Int, UnsafeMutablePointer<HcpReading>?, Int) -> Int32) -> [Reading]? {
        var out = [HcpReading](repeating: HcpReading(), count: cap)
        let n = out.withUnsafeMutableBufferPointer { o in data.withC { parse($0, $1, o.baseAddress, cap) } }
        guard n > 0 else { return nil }
        return out.prefix(Int(n)).map { Reading(value: $0.value, time: Date(timeIntervalSince1970: Double($0.time_ms) / 1000)) }
    }

    /// Two rtsw fields from ONE parse of the body (it was parsed once per field).
    nonisolated static func windPair(_ data: Data, _ a: Int32, _ b: Int32) -> [[Reading]]? {
        let cap = 240
        var outA = [HcpReading](repeating: HcpReading(), count: cap), outB = outA
        var nb: Int32 = 0
        let na = outA.withUnsafeMutableBufferPointer { pa in
            outB.withUnsafeMutableBufferPointer { pb in
                data.withC { hcp_solarwind_series_pair($0, $1, a, b, pa.baseAddress, pb.baseAddress, cap, &nb) }
            }
        }
        guard na > 0, nb > 0 else { return nil }
        let reading = { (r: HcpReading) in Reading(value: r.value, time: Date(timeIntervalSince1970: Double(r.time_ms) / 1000)) }
        return [outA.prefix(Int(na)).map(reading), outB.prefix(Int(nb)).map(reading)]
    }

    nonisolated static func parseDst(_ data: Data) -> [DstReading]? {
        var raw = [HcpDst](repeating: HcpDst(), count: 24 * 8)
        let n = raw.withUnsafeMutableBufferPointer { o in data.withC { hcp_dst_parse($0, $1, o.baseAddress, o.count) } }
        guard n > 0 else { return nil }
        return raw.prefix(Int(n)).map { DstReading(time: Date(timeIntervalSince1970: Double($0.time_ms) / 1000), dst: Int($0.dst)) }
    }

    nonisolated static func parseQuakes(_ data: Data) -> [QuakeItem]? {
        var raw = [HcpQuake](repeating: HcpQuake(), count: 500)
        let n = data.withC { hcp_quakes_parse($0, $1, &raw, raw.count) }
        guard n >= 0 else { return nil }
        return raw.prefix(Int(n)).map {
            QuakeItem(time: Date(timeIntervalSince1970: Double($0.time_ms) / 1000), magnitude: $0.magnitude,
                      place: cText($0.place), lat: $0.latitude, lon: $0.longitude)
        }
    }

    nonisolated static func parseStorms(_ data: Data) -> [StormItem]? {
        var raw = [HcpStorm](repeating: HcpStorm(), count: 20)
        let n = data.withC { hcp_storms_parse($0, $1, &raw, raw.count) }
        guard n >= 0 else { return nil }
        return raw.prefix(Int(n)).map {
            StormItem(name: cText($0.name), classification: cText($0.classification), heading: cText($0.heading),
                      windKt: Int($0.wind_kt), pressureMb: Int($0.pressure_mb), category: Int($0.category),
                      speedKt: Int($0.speed_kt), lat: $0.latitude, lon: $0.longitude)
        }
    }

    nonisolated static func parseOta(_ data: Data,
                                     _ parse: (UnsafePointer<CChar>?, Int, UnsafeMutablePointer<HcpOtaSpot>?, Int) -> Int32) -> [OtaSpot]? {
        var raw = [HcpOtaSpot](repeating: HcpOtaSpot(), count: 300)
        let n = raw.withUnsafeMutableBufferPointer { o in data.withC { parse($0, $1, o.baseAddress, 300) } }
        guard n >= 0 else { return nil }
        return raw.prefix(Int(n)).map {
            OtaSpot(program: cText($0.program), call: cText($0.call), reference: cText($0.reference), name: cText($0.name),
                    mode: cText($0.mode), comment: cText($0.comment), freqKhz: $0.freq_khz,
                    time: Date(timeIntervalSince1970: Double($0.time_ms) / 1000),
                    position: $0.has_position != 0 ? ($0.lat, $0.lon) : nil)
        }
    }
}

enum SpaceText {
    static func xrayClass(_ flux: Double) -> String { cString(16) { hcp_swpc_xray_class(flux, $0, $1) } }
    static func kpWord(_ kp: Double) -> String { cString(64) { hcp_swpc_kp_description(kp, $0, $1) } }
    static func windWord(_ v: Double) -> String { cString(32) { hcp_solarwind_speed_description(v, $0, $1) } }
    static func bzWord(_ v: Double) -> String { cString(32) { hcp_solarwind_bz_description(v, $0, $1) } }
    static func scaleWord(_ level: Int32) -> String { cString(16) { hcp_scale_word(level, $0, $1) } }
    static func quakeClass(_ m: Double) -> String { cString(16) { hcp_quake_magnitude_class(m, $0, $1) } }
    static func band(_ khz: Double) -> String { cString(8) { hcp_band_name(khz, $0, $1) } }
}
