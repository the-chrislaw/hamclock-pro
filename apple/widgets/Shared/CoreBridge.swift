import Foundation

/// One NOAA SWPC reading as the shared C++ core parsed it.
struct Reading: Equatable, Codable {
    var value: Double
    var observed: Date
}

enum BandCondition: Int, Codable, Comparable {
    case poor = 0, fair = 1, good = 2
    static func < (a: BandCondition, b: BandCondition) -> Bool { a.rawValue < b.rawValue }
    var word: String {
        switch self {
        case .poor: return "Poor"
        case .fair: return "Fair"
        case .good: return "Good"
        }
    }
}

struct BandOutlook: Equatable, Codable {
    var band: String   // "20m"
    var day: BandCondition
    var night: BandCondition
}

struct SatellitePass: Equatable, Codable {
    var satellite: String
    var aos: Date
    var tca: Date
    var los: Date
    var maxElevation: Double
    var aosAzimuth: Double
    var losAzimuth: Double
}

/// Swift wrappers over hcpcore.h (Qt-free C++ core shared with the Apple TV app).
/// Swift owns networking and layout; the core owns interpretation.
enum Core {
    private static func withBytes<T>(_ data: Data, _ body: (UnsafePointer<CChar>?, Int) -> T) -> T {
        data.withUnsafeBytes { raw in body(raw.bindMemory(to: CChar.self).baseAddress, data.count) }
    }

    private static func reading(_ data: Data, _ parse: (UnsafePointer<CChar>?, Int) -> HcpReading) -> Reading? {
        let r = withBytes(data, parse)
        guard r.valid != 0, r.value.isFinite else { return nil }
        return Reading(value: r.value, observed: Date(timeIntervalSince1970: Double(r.time_ms) / 1000))
    }

    static func kp(_ data: Data) -> Reading? { reading(data, hcp_swpc_latest_kp) }
    static func solarFlux(_ data: Data) -> Reading? { reading(data, hcp_swpc_latest_solar_flux) }
    static func xray(_ data: Data) -> Reading? { reading(data, hcp_swpc_latest_xray) }

    /// "M1.2" style class for a GOES 0.1-0.8 nm flux in W/m^2, nil for invalid flux.
    static func xrayClass(_ flux: Double) -> String? {
        var buf = [CChar](repeating: 0, count: 16)
        return hcp_swpc_xray_class(flux, &buf, buf.count) != 0 ? String(cString: buf) : nil
    }

    static func kpDescription(_ kp: Double) -> String {
        var buf = [CChar](repeating: 0, count: 64)
        return hcp_swpc_kp_description(kp, &buf, buf.count) != 0 ? String(cString: buf) : ""
    }

    /// Shared band_outlook heuristic (NOT VOACAP); A index unknown here, so only the Kp rule applies.
    static func hfOutlook(sfi: Double, kp: Double) -> [BandOutlook] {
        var out = [HcpBand](repeating: HcpBand(), count: 8)
        let n = Int(hcp_hf_outlook(sfi, kp, 0, &out, out.count))
        return out.prefix(max(0, n)).map { b in
            var copy = b
            let name = withUnsafePointer(to: &copy.band) {
                $0.withMemoryRebound(to: CChar.self, capacity: 8) { String(cString: $0) }
            }
            return BandOutlook(band: name, day: BandCondition(rawValue: Int(b.day)) ?? .poor,
                               night: BandCondition(rawValue: Int(b.night)) ?? .poor)
        }
    }

    static func sunElevation(at date: Date, latitude: Double, longitude: Double) -> Double? {
        var el = 0.0
        return hcp_sun_elevation(Int64(date.timeIntervalSince1970), latitude, longitude, &el) != 0 ? el : nil
    }

    static func tleCount(_ tle: Data) -> Int { Int(withBytes(tle) { hcp_tle_count($0, $1) }) }

    static func tleIndex(_ tle: Data, shortName: String) -> Int? {
        let i = withBytes(tle) { p, n in hcp_tle_find_short_name(p, n, shortName) }
        return i >= 0 ? Int(i) : nil
    }

    static func tleShortName(_ tle: Data, index: Int) -> String? {
        var buf = [CChar](repeating: 0, count: 48)
        let ok = withBytes(tle) { p, n in hcp_tle_short_name(p, n, Int32(index), &buf, buf.count) }
        return ok != 0 ? String(cString: buf) : nil
    }

    /// TLE epoch of the index-th set (age drives the "elements are old" label). The core reports
    /// it with a propagated state, so `at` must be a time SGP4 can reach (e.g. now for current elements).
    static func tleEpoch(_ tle: Data, index: Int, at date: Date) -> Date? {
        var s = HcpSatState()
        let t = Int64(date.timeIntervalSince1970)
        let ok = withBytes(tle) { p, n in hcp_sat_state(p, n, Int32(index), t, 0, 0, &s) }
        return ok != 0 ? Date(timeIntervalSince1970: TimeInterval(s.epoch_s)) : nil
    }

    static func passes(tle: Data, index: Int, from: Date, horizon: TimeInterval, latitude: Double,
                       longitude: Double, minElevation: Double = 0, limit: Int = 4) -> [SatellitePass] {
        guard limit > 0, let name = tleShortName(tle, index: index) else { return [] }
        var out = [HcpPass](repeating: HcpPass(), count: limit)
        let n = withBytes(tle) { p, len in
            hcp_sat_passes(p, len, Int32(index), Int64(from.timeIntervalSince1970), Int64(horizon),
                           latitude, longitude, minElevation, &out, out.count)
        }
        guard n > 0 else { return [] }
        return out.prefix(Int(n)).map {
            SatellitePass(satellite: name,
                          aos: Date(timeIntervalSince1970: TimeInterval($0.aos_s)),
                          tca: Date(timeIntervalSince1970: TimeInterval($0.tca_s)),
                          los: Date(timeIntervalSince1970: TimeInterval($0.los_s)),
                          maxElevation: $0.max_el_deg, aosAzimuth: $0.aos_az_deg, losAzimuth: $0.los_az_deg)
        }
    }
}
