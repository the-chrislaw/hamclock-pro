import Foundation
import WidgetKit

struct SpaceWeatherEntry: TimelineEntry {
    let date: Date
    let weather: SpaceWeatherSnapshot
}

struct BandsEntry: TimelineEntry {
    let date: Date
    let weather: SpaceWeatherSnapshot
    let station: Station?

    var isDay: Bool { Format.isDay(at: date, station: station) }
    var outlook: [BandOutlook]? {
        guard let sfi = weather.sfi?.value, let kp = weather.kp?.value else { return nil }
        return Core.hfOutlook(sfi: sfi, kp: kp)
    }
}

enum PassState: Equatable {
    /// The app has not shared a configured station yet.
    case needsStation
    /// No satellite elements (never downloaded, offline on first run).
    case noElements
    /// Elements fine but no pass of the satellite within the search window.
    case noPass(satellite: String)
    case pass(SatellitePass, elements: Freshness, elementsOld: Bool)
}

struct PassEntry: TimelineEntry {
    let date: Date
    let state: PassState
    let callsign: String
}

/// Timeline refresh cadence for every HamClock widget.
enum Refresh {
    static let interval: TimeInterval = 30 * 60
    /// How far ahead to search for satellite passes.
    static let passHorizon: TimeInterval = 48 * 3600
    static let passesPerTimeline = 4
}

/// Fixed sample data for the widget gallery, placeholders and snapshot tests. Not live
/// data and never shown as such outside the gallery. Station is a grid-square centre.
enum Samples {
    static let now = Date(timeIntervalSince1970: 1_793_000_000) // 2026-10-26 07:33 UTC
    static let weather = SpaceWeatherSnapshot(
        kp: Reading(value: 2.33, observed: now.addingTimeInterval(-1800)),
        sfi: Reading(value: 148, observed: now.addingTimeInterval(-3600)),
        xray: Reading(value: 2.4e-6, observed: now.addingTimeInterval(-300)),
        fetched: now, lastAttemptFailed: false)
    static let station = Station(callsign: "N0CALL", latitude: 41.5, longitude: -73.0, tracked: ["ISS"])
    static let pass = SatellitePass(satellite: "ISS", aos: now.addingTimeInterval(23 * 60),
                                    tca: now.addingTimeInterval(28 * 60), los: now.addingTimeInterval(33 * 60),
                                    maxElevation: 54, aosAzimuth: 225, losAzimuth: 45)
    static let spaceWeatherEntry = SpaceWeatherEntry(date: now, weather: weather)
    static let bandsEntry = BandsEntry(date: now, weather: weather, station: station)
    static let passEntry = PassEntry(date: now, state: .pass(pass, elements: .live, elementsOld: false), callsign: "N0CALL")
}
