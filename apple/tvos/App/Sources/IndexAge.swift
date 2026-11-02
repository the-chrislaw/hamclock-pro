import Foundation

/// Whether the SWPC values a pane shows are too old, from the shared core (limits in swpc.hpp).
struct IndexAge {
    let stale: Bool
    /// The oldest old reading, for the footer age; nil when none is old.
    let oldest: Date?
    /// Footer text, longest first (see Card.statusForms). Empty when not stale.
    let notes: [String]

    init(_ check: HcpIndexCheck) {
        stale = check.stale != 0
        oldest = check.oldest_ms > 0 ? Date(timeIntervalSince1970: Double(check.oldest_ms) / 1000) : nil
        notes = stale ? [cText(check.note), cText(check.short_note), cText(check.tiny_note)] : []
    }

    static func input(_ time: Date?, _ feed: WidgetState) -> HcpIndexInput {
        HcpIndexInput(has_value: time == nil ? 0 : 1, waiting: time == nil && feed == .loading ? 1 : 0,
                      feed_late: feed == .stale || feed == .offline ? 1 : 0,
                      time_ms: Int64((time?.timeIntervalSince1970 ?? 0) * 1000))
    }

    static func spaceWeather(kp: HcpIndexInput, sfi: HcpIndexInput, xray: HcpIndexInput, now: Date) -> IndexAge {
        var kp = kp, sfi = sfi, xray = xray, out = HcpIndexCheck()
        _ = hcp_swpc_check_space_weather(&kp, &sfi, &xray, Int64(now.timeIntervalSince1970 * 1000), &out)
        return IndexAge(out)
    }

    static func hf(sfi: HcpIndexInput, kp: HcpIndexInput, now: Date) -> IndexAge {
        var sfi = sfi, kp = kp, out = HcpIndexCheck()
        _ = hcp_swpc_check_hf(&sfi, &kp, Int64(now.timeIntervalSince1970 * 1000), &out)
        return IndexAge(out)
    }
}

extension Feeds {
    /// HF bands and HF paths need both indices; without both the fetch state says why.
    func hfAge(_ now: Date) -> IndexAge? {
        guard let sfiReading = flux.value, let kpReading = kp.value?.last else { return nil }
        return IndexAge.hf(sfi: IndexAge.input(sfiReading.time, flux.state(now)),
                           kp: IndexAge.input(kpReading.time, kp.state(now)), now: now)
    }

    /// The older of the two fetch times the HF panes depend on.
    var hfFetched: Date? { [kp.status.fetched, flux.status.fetched].compactMap { $0 }.min() }
}

#if DEBUG
/// UI-test hook, absent from Release: `-hcpSeedIndexAges KP_HOURS,SFI_HOURS,XRAY_MINUTES` shows
/// made-up Kp, SFI and X-ray readings of those ages instead of polling NOAA, each feed fetched just
/// now; `-` means that feed answered with nothing. Real feeds are healthy, so CI could not otherwise
/// show the Stale footers.
enum IndexSeed {
    static let ages: [Double?]? = {
        let args = ProcessInfo.processInfo.arguments
        guard let i = args.firstIndex(of: "-hcpSeedIndexAges"), args.count > i + 1 else { return nil }
        let parts = args[i + 1].split(separator: ",").map { Double($0) }
        return parts.count == 3 ? parts : nil
    }()

    /// `-hcpSeedBandsAllGood`: every HF bands cell reads Good, the widest table. The outlook itself
    /// never gives this (80 m by day is always Poor), though high SFI makes every column as wide.
    static let allGoodBands = ProcessInfo.processInfo.arguments.contains("-hcpSeedBandsAllGood")
}

extension Feeds {
    func applyIndexSeed(_ ages: [Double?], now: Date) {
        let reading = { (value: Double, age: TimeInterval) in Reading(value: value, time: now.addingTimeInterval(-age)) }
        kp.seed(ages[0].map { h in [reading(2.33, (h + 6) * 3600), reading(3, (h + 3) * 3600), reading(3.67, h * 3600)] }, now: now)
        flux.seed(ages[1].map { reading(142, $0 * 3600) }, now: now)
        xray.seed(ages[2].map { m in (0..<60).reversed().map { reading(2e-6 + 1e-7 * Double($0 % 7), (m + Double($0)) * 60) } }, now: now)
    }
}
#endif
