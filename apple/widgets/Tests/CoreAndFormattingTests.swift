import XCTest

/// Shared NOAA fixtures (the Qt and tvOS parser tests use the same files).
enum Fixtures {
    static let repoRoot = URL(fileURLWithPath: #filePath)
        .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
    static func data(_ relative: String) throws -> Data {
        try Data(contentsOf: repoRoot.appendingPathComponent(relative))
    }
    static func spaceWeather(_ name: String) throws -> Data {
        try data("src/widgets/spaceweather/tests/fixtures/" + name)
    }
    /// ISS elements (Vallado/CelesTrak sample, epoch 2008-264) plus a bracketed name.
    static let tle = Data("""
    ISS (ZARYA)
    1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927
    2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537
    SAUDISAT 1C (SO-50)
    1 25544U 98067A   08264.51782528 -.00002182  00000-0 -11606-4 0  2927
    2 25544  51.6416 247.4627 0006703 130.5360 325.0288 15.72125391563537

    """.utf8)
    static let tleEpochPlus5h = Date(timeIntervalSince1970: 1_221_931_538)
}

final class CoreBridgeTests: XCTestCase {
    func testParsesNoaaFixtures() throws {
        let kp = try XCTUnwrap(Core.kp(try Fixtures.spaceWeather("noaa-planetary-k-index.json")))
        XCTAssertEqual(kp.value, 3.0)
        XCTAssertEqual(kp.observed.timeIntervalSince1970, 1_791_892_800)
        XCTAssertEqual(Core.solarFlux(try Fixtures.spaceWeather("10cm-flux.json"))?.value, 123)
        let x = try XCTUnwrap(Core.xray(try Fixtures.spaceWeather("xrays-6-hour.json")))
        XCTAssertEqual(Core.xrayClass(x.value), "B7.2")
    }

    func testRejectsGarbage() {
        XCTAssertNil(Core.kp(Data("garbage".utf8)))
        XCTAssertNil(Core.kp(Data()))
        XCTAssertNil(Core.xrayClass(-1))
    }

    func testHfOutlookMatchesSharedHeuristic() {
        let o = Core.hfOutlook(sfi: 200, kp: 3)
        XCTAssertEqual(o.count, 8)
        XCTAssertEqual(o.first { $0.band == "10m" }?.day, .good)
        XCTAssertEqual(o.first { $0.band == "10m" }?.night, .poor)
        XCTAssertEqual(o.first { $0.band == "80m" }?.day, .poor)
    }

    func testSatelliteLookupAndPasses() throws {
        XCTAssertEqual(Core.tleCount(Fixtures.tle), 2)
        XCTAssertEqual(Core.tleIndex(Fixtures.tle, shortName: "iss"), 0)
        XCTAssertEqual(Core.tleIndex(Fixtures.tle, shortName: "SO-50"), 1)
        XCTAssertNil(Core.tleIndex(Fixtures.tle, shortName: "AO-91"))
        XCTAssertEqual(Core.tleShortName(Fixtures.tle, index: 1), "SO-50")
        let passes = Core.passes(tle: Fixtures.tle, index: 0, from: Fixtures.tleEpochPlus5h, horizon: 86400,
                                 latitude: 41.5, longitude: -73.0, limit: 4)
        XCTAssertFalse(passes.isEmpty)
        for p in passes {
            XCTAssertEqual(p.satellite, "ISS")
            XCTAssertLessThanOrEqual(p.aos, p.tca)
            XCTAssertLessThanOrEqual(p.tca, p.los)
            XCTAssertGreaterThanOrEqual(p.aos, Fixtures.tleEpochPlus5h)
        }
        XCTAssertEqual(Core.tleEpoch(Fixtures.tle, index: 0, at: Fixtures.tleEpochPlus5h)?.timeIntervalSince1970 ?? 0, 1_221_913_538, accuracy: 5)
        XCTAssertTrue(Core.passes(tle: Fixtures.tle, index: 5, from: Date(), horizon: 3600, latitude: 0, longitude: 0).isEmpty)
    }
}

final class FormattingTests: XCTestCase {
    let now = Date(timeIntervalSince1970: 1_793_000_000)

    func testFreshness() {
        XCTAssertEqual(Freshness.evaluate(fetched: nil, lastAttemptFailed: true, now: now), .noData)
        XCTAssertEqual(Freshness.evaluate(fetched: now.addingTimeInterval(-600), lastAttemptFailed: false, now: now), .live)
        XCTAssertEqual(Freshness.evaluate(fetched: now.addingTimeInterval(-600), lastAttemptFailed: true, now: now), .stale)
        XCTAssertEqual(Freshness.evaluate(fetched: now.addingTimeInterval(-7200), lastAttemptFailed: false, now: now), .stale)
        XCTAssertEqual(Freshness.evaluate(fetched: now.addingTimeInterval(-7200), lastAttemptFailed: true, now: now), .offline)
        XCTAssertEqual(Freshness.evaluate(fetched: now.addingTimeInterval(-7 * 3600), lastAttemptFailed: false, now: now), .offline)
        XCTAssertNil(Freshness.live.badge)
        XCTAssertEqual(Freshness.offline.badge, "Offline")
    }

    func testClockAndCompass() {
        let utc = TimeZone(identifier: "UTC")!
        XCTAssertEqual(Format.asOf(now, zone: utc), "as of 07:33")
        XCTAssertEqual(Format.compass(0), "N")
        XCTAssertEqual(Format.compass(359), "N")
        XCTAssertEqual(Format.compass(45), "NE")
        XCTAssertEqual(Format.compass(-90), "W")
        XCTAssertEqual(Format.compass(202.5), "SSW")
        XCTAssertEqual(Format.compass(.nan), "–")
    }

    func testSeverities() {
        XCTAssertEqual(Format.kpSeverity(2.33), .calm)
        XCTAssertEqual(Format.kpSeverity(4), .elevated)
        XCTAssertEqual(Format.kpSeverity(5.67), .severe)
        XCTAssertEqual(Format.kpWord(3), "Unsettled")
        XCTAssertEqual(Format.xraySeverity("B7.2"), .calm)
        XCTAssertEqual(Format.xraySeverity("M1.0"), .elevated)
        XCTAssertEqual(Format.xraySeverity("X2.1"), .severe)
        XCTAssertEqual(Format.kp(2.333), "2.3")
        XCTAssertEqual(Format.sfi(147.6), "148")
    }

    func testBandGroupsTakeTheBetterBandOfEachPair() {
        let o = Core.hfOutlook(sfi: 123, kp: 3)
        let day = Format.bandGroups(o, day: true)
        XCTAssertEqual(day.map(\.label), ["80-40", "30-20", "17-15", "12-10"])
        XCTAssertEqual(day.first { $0.label == "30-20" }?.condition, .good) // 20m good by day at SFI 123
        XCTAssertEqual(day.first { $0.label == "12-10" }?.condition,
                       max(o.first { $0.band == "12m" }!.day, o.first { $0.band == "10m" }!.day))
        XCTAssertTrue(Format.bandGroups([], day: true).isEmpty)
    }

    func testOpenBands() {
        let o = [BandOutlook(band: "40m", day: .fair, night: .good),
                 BandOutlook(band: "20m", day: .good, night: .poor),
                 BandOutlook(band: "15m", day: .good, night: .poor)]
        XCTAssertEqual(Format.openBands(o, day: true)?.list, "20 15")
        XCTAssertEqual(Format.openBands(o, day: true)?.condition, .good)
        XCTAssertEqual(Format.openBands(o, day: false)?.list, "40")
        XCTAssertNil(Format.openBands([BandOutlook(band: "10m", day: .poor, night: .poor)], day: true))
    }

    func testDayNightAtStation() {
        // 2026-10-26 07:33 UTC: daytime in Europe, night on the US west coast.
        let europe = Station(callsign: "", latitude: 48.5, longitude: 11.0, tracked: [])
        let pacific = Station(callsign: "", latitude: 37.5, longitude: -122.0, tracked: [])
        XCTAssertTrue(Format.isDay(at: now, station: europe))
        XCTAssertFalse(Format.isDay(at: now, station: pacific))
        XCTAssertTrue(Format.isDay(at: now, station: nil)) // UTC 06-18 fallback
    }

    func testPassPhase() {
        let p = SatellitePass(satellite: "ISS", aos: now, tca: now + 300, los: now + 600,
                              maxElevation: 40, aosAzimuth: 0, losAzimuth: 180)
        XCTAssertEqual(PassPhase.of(p, at: now - 1), .upcoming)
        XCTAssertEqual(PassPhase.of(p, at: now + 10), .upNow)
        XCTAssertEqual(PassPhase.of(p, at: now + 600), .over)
    }
}
