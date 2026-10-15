import Foundation

/// The operator's Maidenhead locator. tvOS gives apps no guaranteed local storage,
/// so it lives in iCloud key-value storage (NSUbiquitousKeyValueStore), mirrored to
/// UserDefaults as a best-effort cache for when iCloud is unavailable.
@MainActor
final class StationStore: ObservableObject {
    private static let key = "station.locator"
    private let kvs = NSUbiquitousKeyValueStore.default

    @Published var locator: String {
        didSet {
            kvs.set(locator, forKey: Self.key)
            kvs.synchronize()
            UserDefaults.standard.set(locator, forKey: Self.key)
        }
    }

    init() {
        locator = kvs.string(forKey: Self.key) ?? UserDefaults.standard.string(forKey: Self.key) ?? ""
        NotificationCenter.default.addObserver(forName: NSUbiquitousKeyValueStore.didChangeExternallyNotification,
                                               object: kvs, queue: .main) { [weak self] _ in
            Task { @MainActor in
                if let v = self?.kvs.string(forKey: Self.key), v != self?.locator { self?.locator = v }
            }
        }
        kvs.synchronize()
    }

    /// Centre of the locator square from the shared C++ decoder; nil when not a valid 2/4/6-char locator.
    var position: (lat: Double, lon: Double)? {
        var lat = 0.0, lon = 0.0
        return hcp_locator_center(locator, &lat, &lon) != 0 ? (lat, lon) : nil
    }
}
