import Foundation
import WidgetKit

/// Kp, SFI and X-ray from NOAA SWPC, refreshed about every 30 minutes.
struct SpaceWeatherProvider: TimelineProvider {
    var feeds = Feeds()

    func placeholder(in context: Context) -> SpaceWeatherEntry { Samples.spaceWeatherEntry }

    func getSnapshot(in context: Context, completion: @escaping (SpaceWeatherEntry) -> Void) {
        if context.isPreview { completion(Samples.spaceWeatherEntry); return }
        Task { completion(SpaceWeatherEntry(date: Date(), weather: await feeds.spaceWeather())) }
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<SpaceWeatherEntry>) -> Void) {
        Task {
            let now = Date()
            let entry = SpaceWeatherEntry(date: now, weather: await feeds.spaceWeather(now: now))
            completion(Timeline(entries: [entry], policy: .after(now.addingTimeInterval(Refresh.interval))))
        }
    }
}

/// HF band outlook from the same SFI/Kp, with day/night switched at the station's sunrise/sunset.
struct BandsProvider: TimelineProvider {
    var feeds = Feeds()

    func placeholder(in context: Context) -> BandsEntry { Samples.bandsEntry }

    func getSnapshot(in context: Context, completion: @escaping (BandsEntry) -> Void) {
        if context.isPreview { completion(Samples.bandsEntry); return }
        Task {
            completion(BandsEntry(date: Date(), weather: await feeds.spaceWeather(),
                                  station: Station.read(from: AppGroup.defaults)))
        }
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<BandsEntry>) -> Void) {
        Task {
            let now = Date()
            let weather = await feeds.spaceWeather(now: now)
            let station = Station.read(from: AppGroup.defaults)
            // Several entries across the half hour so day/night flips near sunrise/sunset on time.
            let entries = stride(from: 0, to: Refresh.interval, by: 10 * 60).map {
                BandsEntry(date: now.addingTimeInterval($0), weather: weather, station: station)
            }
            completion(Timeline(entries: entries, policy: .after(now.addingTimeInterval(Refresh.interval))))
        }
    }
}

/// Next pass of the user's tracked satellite (else the ISS) over the shared station.
struct PassProvider: TimelineProvider {
    var feeds = Feeds()

    func placeholder(in context: Context) -> PassEntry { Samples.passEntry }

    func getSnapshot(in context: Context, completion: @escaping (PassEntry) -> Void) {
        if context.isPreview { completion(Samples.passEntry); return }
        Task { completion(await PassTimeline.load(feeds: feeds, now: Date()).first ?? Samples.passEntry) }
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<PassEntry>) -> Void) {
        Task {
            let now = Date()
            let entries = await PassTimeline.load(feeds: feeds, now: now)
            completion(Timeline(entries: entries, policy: .after(now.addingTimeInterval(Refresh.interval))))
        }
    }
}

enum PassTimeline {
    static let defaultSatellite = "ISS"

    /// First tracked satellite present in the elements, else the ISS, else nil.
    static func satelliteIndex(tle: Data, tracked: [String]) -> Int? {
        for name in tracked + [defaultSatellite] {
            if let i = Core.tleIndex(tle, shortName: name) { return i }
        }
        return nil
    }

    static func load(feeds: Feeds, now: Date, defaults: UserDefaults? = AppGroup.defaults) async -> [PassEntry] {
        guard let station = Station.read(from: defaults) else {
            return [PassEntry(date: now, state: .needsStation, callsign: "")]
        }
        guard let tle = await feeds.tle(now: now), let index = satelliteIndex(tle: tle.data, tracked: station.tracked) else {
            return [PassEntry(date: now, state: .noElements, callsign: station.callsign)]
        }
        let elements = Freshness.evaluate(fetched: tle.fetched, lastAttemptFailed: tle.lastAttemptFailed, now: now)
        let epoch = Core.tleEpoch(tle.data, index: index, at: now)
        let old = epoch.map { now.timeIntervalSince($0) > Format.tleWarnAge } ?? true
        let passes = Core.passes(tle: tle.data, index: index, from: now, horizon: Refresh.passHorizon,
                                 latitude: station.latitude, longitude: station.longitude,
                                 limit: Refresh.passesPerTimeline)
        let name = Core.tleShortName(tle.data, index: index) ?? defaultSatellite
        return entries(passes: passes, satellite: name, now: now, elements: elements, elementsOld: old,
                       callsign: station.callsign)
    }

    /// One entry now, one at each LOS so the widget moves on to the next pass by itself, and
    /// one when each pass comes within PassView.countdownWindow so the clock time turns into a countdown.
    static func entries(passes: [SatellitePass], satellite: String, now: Date, elements: Freshness,
                        elementsOld: Bool, callsign: String) -> [PassEntry] {
        guard !passes.isEmpty else {
            return [PassEntry(date: now, state: .noPass(satellite: satellite), callsign: callsign)]
        }
        var out: [PassEntry] = []
        for (i, pass) in passes.enumerated() {
            let shownFrom = i == 0 ? now : max(now, passes[i - 1].los)
            let state = PassState.pass(pass, elements: elements, elementsOld: elementsOld)
            out.append(PassEntry(date: shownFrom, state: state, callsign: callsign))
            let countdownFrom = pass.aos.addingTimeInterval(-PassView.countdownWindow)
            if countdownFrom > shownFrom {
                out.append(PassEntry(date: countdownFrom, state: state, callsign: callsign))
            }
        }
        return out
    }
}
