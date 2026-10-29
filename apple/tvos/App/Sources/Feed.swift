import Foundation

/// Shared networking for every source: honest User-Agent, 20 s timeout.
enum Net {
    static let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "0"
    static let userAgent = "HamClockPro/\(version) (+https://github.com/the-chrislaw/hamclock-pro)"
    static let session: URLSession = {
        let cfg = URLSessionConfiguration.default
        cfg.timeoutIntervalForRequest = 20
        cfg.httpAdditionalHeaders = ["User-Agent": userAgent]
        return URLSession(configuration: cfg)
    }()

    /// Body of a 200 reply, or nil.
    static func get(_ url: URL) async -> Data? {
        guard let (data, resp) = try? await session.data(from: url),
              (resp as? HTTPURLResponse)?.statusCode == 200 else { return nil }
        return data
    }
}

extension Data {
    /// Runs a C parser over the raw bytes.
    func withC<T>(_ body: (UnsafePointer<CChar>?, Int) -> T) -> T {
        withUnsafeBytes { body($0.bindMemory(to: CChar.self).baseAddress, count) }
    }
}

/// String from a fixed-size C char array imported as a tuple.
func cText<T>(_ tuple: T) -> String {
    var copy = tuple
    return withUnsafePointer(to: &copy) {
        $0.withMemoryRebound(to: CChar.self, capacity: MemoryLayout<T>.size) { String(cString: $0) }
    }
}

/// String from a C function writing into a buffer and returning non-zero on success.
func cString(_ cap: Int = 64, _ fill: (UnsafeMutablePointer<CChar>, Int) -> Int32) -> String {
    var buf = [CChar](repeating: 0, count: cap)
    return buf.withUnsafeMutableBufferPointer { fill($0.baseAddress!, cap) } != 0 ? String(cString: buf) : ""
}

/// Fetch freshness thresholds, same idea as the Qt PolledSource policy: data turns Stale after
/// `staleAfter` without a successful refresh and Offline after `offlineAfter`.
struct FreshnessPolicy {
    var interval: TimeInterval
    var staleAfter: TimeInterval
    var offlineAfter: TimeInterval
}

/// Common state of a polled document: when it last succeeded, whether the last try failed.
struct FeedStatus {
    var fetched: Date?
    var failed = false
    var tried = false

    func state(_ now: Date, _ p: FreshnessPolicy) -> WidgetState {
        guard let fetched else { return tried ? .offline : .loading }
        let age = now.timeIntervalSince(fetched)
        return age > p.offlineAfter ? .offline : age > p.staleAfter ? .stale : .live
    }
}

/// One polled HTTPS document handed to a parser from the Qt-free core. A failed fetch or parse
/// keeps the previous value; its age then drives Stale/Offline (never shown as live).
@MainActor
final class PolledFeed<Value>: ObservableObject {
    @Published private(set) var value: Value?
    @Published private(set) var status = FeedStatus()
    let urls: [URL]
    let policy: FreshnessPolicy
    let source: String
    private let parse: (Data) -> Value?

    /// After a failed refresh, try again sooner than the normal interval.
    private static var retryAfterFailure: TimeInterval { 300 }

    /// `urls` are tried in order (fallback mirrors) until one parses.
    init(_ urls: [String], source: String, policy: FreshnessPolicy, parse: @escaping (Data) -> Value?) {
        self.urls = urls.compactMap(URL.init(string:))
        self.source = source
        self.policy = policy
        self.parse = parse
        loadCache()
    }

    func state(_ now: Date) -> WidgetState { status.state(now, policy) }

    func run() async {
        while !Task.isCancelled {
            // A cached copy younger than the interval (relaunch) is not fetched again yet:
            // some sources (Launch Library 2) allow only a few requests an hour.
            if let f = status.fetched, Date().timeIntervalSince(f) < policy.interval {
                try? await Task.sleep(nanoseconds: UInt64((policy.interval - Date().timeIntervalSince(f)) * 1e9))
                continue
            }
            await refresh()
            let wait = status.failed ? min(policy.interval, Self.retryAfterFailure) : policy.interval
            try? await Task.sleep(nanoseconds: UInt64(wait * 1e9))
        }
    }

    func refresh() async {
        for url in urls {
            if let data = await Net.get(url), let v = parse(data) {
                value = v
                status = FeedStatus(fetched: Date(), failed: false, tried: true)
                try? data.write(to: cacheURL, options: .atomic)
                return
            }
        }
        status.failed = true
        status.tried = true
    }

    // MARK: disposable cache (tvOS may purge Caches at any time; absence just means a fresh fetch)

    private var cacheURL: URL {
        let name = (urls.first?.absoluteString ?? source).map { $0.isLetter || $0.isNumber ? $0 : "_" }
        return FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("feed-" + String(String(name).suffix(120)))
    }

    /// The last good body, shown with its real fetch time so its age drives Stale/Offline.
    private func loadCache() {
        guard let data = try? Data(contentsOf: cacheURL),
              let date = (try? FileManager.default.attributesOfItem(atPath: cacheURL.path))?[.modificationDate] as? Date,
              let v = parse(data) else { return }
        value = v
        status = FeedStatus(fetched: date, failed: false, tried: false)
    }
}

enum Age {
    /// "just now", "5 min ago", "3 h ago", "7 days ago" (same wording as the Qt cards).
    static func text(_ date: Date?, now: Date) -> String {
        guard let date else { return "" }
        let s = max(0, now.timeIntervalSince(date))
        if s < 90 { return "just now" }
        if s < 3600 { return "\(Int(s / 60)) min ago" }
        if s < 86400 * 2 { return "\(Int(s / 3600)) h ago" }
        return "\(Int(s / 86400)) days ago"
    }

    static func utcClock(_ date: Date) -> String {
        StationText.formatter("HH:mm'z'", StationText.utc).string(from: date)
    }
}
