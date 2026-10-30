import XCTest

final class StationShareTests: XCTestCase {
    private var suiteName = ""
    private var defaults: UserDefaults!

    override func setUp() {
        suiteName = "hcp.tests.\(UUID().uuidString)"
        defaults = UserDefaults(suiteName: suiteName)
    }

    override func tearDown() {
        defaults.removePersistentDomain(forName: suiteName)
    }

    /// Writes what src/platform/apple_widgetshare_ios.mm writes (same keys and types).
    private func share(configured: Bool, lat: Double = 41.5, lon: Double = -73.0, tracked: [String] = ["SO-50"]) {
        defaults.set(1, forKey: AppGroup.Key.version)
        defaults.set(configured, forKey: AppGroup.Key.configured)
        defaults.set("N0CALL", forKey: AppGroup.Key.callsign)
        defaults.set(lat, forKey: AppGroup.Key.latitude)
        defaults.set(lon, forKey: AppGroup.Key.longitude)
        defaults.set(tracked, forKey: AppGroup.Key.tracked)
    }

    func testReadsConfiguredStation() {
        share(configured: true)
        XCTAssertEqual(Station.read(from: defaults),
                       Station(callsign: "N0CALL", latitude: 41.5, longitude: -73.0, tracked: ["SO-50"]))
    }

    func testUnconfiguredOrInvalidStationIsNil() {
        XCTAssertNil(Station.read(from: defaults))
        XCTAssertNil(Station.read(from: nil))
        share(configured: false)
        XCTAssertNil(Station.read(from: defaults))
        share(configured: true, lat: 95)
        XCTAssertNil(Station.read(from: defaults))
    }

    func testKeysMatchTheQtSide() {
        // tests/widget_share_regression.cpp checks the same strings on the C++ side.
        XCTAssertEqual(AppGroup.identifier, "group.com.chrislaw.hamclockpro")
        XCTAssertEqual([AppGroup.Key.configured, AppGroup.Key.callsign, AppGroup.Key.latitude,
                        AppGroup.Key.longitude, AppGroup.Key.tracked, AppGroup.Key.version, AppGroup.Key.updated],
                       ["station.configured", "station.callsign", "station.lat", "station.lon",
                        "satellites.tracked", "share.version", "share.updated"])
    }

    func testPassTimelineNeedsStation() async {
        let feeds = Feeds(fetch: { _ in nil }, cacheDirectory: FileManager.default.temporaryDirectory)
        let entries = await PassTimeline.load(feeds: feeds, now: Date(), defaults: defaults)
        XCTAssertEqual(entries.map(\.state), [.needsStation])
    }
}

final class FeedTests: XCTestCase {
    private var dir: URL!

    override func setUpWithError() throws {
        dir = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
    }

    override func tearDownWithError() throws {
        try? FileManager.default.removeItem(at: dir)
    }

    private func fixtureFetch() throws -> Feeds.Fetch {
        let map: [URL: Data] = [
            Feeds.kpURL: try Fixtures.spaceWeather("noaa-planetary-k-index.json"),
            Feeds.fluxURL: try Fixtures.spaceWeather("10cm-flux.json"),
            Feeds.xrayURL: try Fixtures.spaceWeather("xrays-6-hour.json"),
            Feeds.tleURL: Fixtures.tle,
        ]
        return { map[$0] }
    }

    func testOnlineThenOfflineKeepsLastValuesAndFlagsThem() async throws {
        let t0 = Date(timeIntervalSince1970: 1_793_000_000)
        let online = Feeds(fetch: try fixtureFetch(), cacheDirectory: dir)
        let first = await online.spaceWeather(now: t0)
        XCTAssertEqual(first.kp?.value, 3.0)
        XCTAssertEqual(first.sfi?.value, 123)
        XCTAssertEqual(first.freshness(now: t0), .live)

        let offline = Feeds(fetch: { _ in nil }, cacheDirectory: dir)
        let later = t0.addingTimeInterval(1800)
        let second = await offline.spaceWeather(now: later)
        XCTAssertEqual(second.kp, first.kp, "cached value survives an offline refresh")
        XCTAssertEqual(second.fetched, t0, "fetch time is not advanced")
        XCTAssertEqual(second.freshness(now: later), .stale)
        XCTAssertEqual(second.freshness(now: t0.addingTimeInterval(7200)), .offline)
    }

    func testFirstRunOfflineHasNoData() async {
        let offline = Feeds(fetch: { _ in nil }, cacheDirectory: dir)
        let s = await offline.spaceWeather(now: Date())
        XCTAssertNil(s.kp)
        XCTAssertEqual(s.freshness(now: Date()), .noData)
    }

    func testPartialFetchIsNotLive() {
        let now = Date()
        let s = SpaceWeatherSnapshot().merged(kp: Reading(value: 2, observed: now), sfi: nil, xray: nil, now: now)
        XCTAssertEqual(s.kp?.value, 2)
        XCTAssertEqual(s.freshness(now: now), .stale)
    }

    func testTleIsCachedAndNotRefetchedWithinTwelveHours() async throws {
        let t0 = Date()
        var calls = 0
        let base = try fixtureFetch()
        let counting = Feeds(fetch: { url in calls += 1; return await base(url) }, cacheDirectory: dir)
        let first = await counting.tle(now: t0)
        XCTAssertEqual(first.map { Core.tleCount($0.data) }, 2)
        XCTAssertEqual(calls, 1)
        _ = await counting.tle(now: t0.addingTimeInterval(3600))
        XCTAssertEqual(calls, 1, "cache used within the CelesTrak refresh window")
        let offline = Feeds(fetch: { _ in nil }, cacheDirectory: dir)
        let stale = await offline.tle(now: t0.addingTimeInterval(13 * 3600))
        XCTAssertEqual(stale?.lastAttemptFailed, true)
        XCTAssertEqual(stale.map { Core.tleCount($0.data) }, 2)
    }

    func testPassTimelineRollsToTheNextPassAtEachLos() {
        let now = Date(timeIntervalSince1970: 1_793_000_000)
        func pass(_ start: TimeInterval) -> SatellitePass {
            SatellitePass(satellite: "ISS", aos: now + start, tca: now + start + 300, los: now + start + 600,
                          maxElevation: 30, aosAzimuth: 200, losAzimuth: 40)
        }
        let passes = [pass(600), pass(6000), pass(12000)]
        let e = PassTimeline.entries(passes: passes, satellite: "ISS", now: now, elements: .live,
                                     elementsOld: false, callsign: "N0CALL")
        // Pass 1 and 2 are first shown with their rise time, then as a countdown in the last hour.
        XCTAssertEqual(e.map(\.date), [now, passes[0].los, passes[1].aos - 3600, passes[1].los, passes[2].aos - 3600])
        XCTAssertEqual(e.map(\.state), [passes[0], passes[1], passes[1], passes[2], passes[2]]
            .map { .pass($0, elements: .live, elementsOld: false) })
        XCTAssertEqual(PassView.countdownWindow, 3600)
        XCTAssertEqual(PassTimeline.entries(passes: [], satellite: "ISS", now: now, elements: .live,
                                            elementsOld: false, callsign: "").map(\.state), [.noPass(satellite: "ISS")])
    }

    func testTrackedSatelliteWinsOverIss() {
        XCTAssertEqual(PassTimeline.satelliteIndex(tle: Fixtures.tle, tracked: ["SO-50"]), 1)
        XCTAssertEqual(PassTimeline.satelliteIndex(tle: Fixtures.tle, tracked: ["AO-91"]), 0, "falls back to the ISS")
        XCTAssertEqual(PassTimeline.satelliteIndex(tle: Fixtures.tle, tracked: []), 0)
        XCTAssertNil(PassTimeline.satelliteIndex(tle: Data(), tracked: []))
    }
}
