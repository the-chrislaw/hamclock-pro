import Foundation

/// Dashboard panes the operator can switch on or off.
enum Pane: String, CaseIterable, Identifiable {
    case map, spaceWeather, bands, sun, station, satellites, quakes, weather, storms
    var id: String { rawValue }
    var title: String {
        switch self {
        case .map: return "World map"
        case .spaceWeather: return "Space weather"
        case .bands: return "HF band outlook"
        case .sun: return "Sun (subsolar point)"
        case .station: return "Sun and Moon at your station"
        case .satellites: return "Amateur satellites"
        case .quakes: return "Earthquakes"
        case .weather: return "Weather at your station"
        case .storms: return "Tropical storms"
        }
    }
}

/// Dashboard pages: the panes do not fit one 1080p screen, so Play/Pause (or auto-rotate)
/// steps through the pages that still have at least one visible pane.
enum Page: Int, CaseIterable {
    case overview, solar, sky, hazards
    var panes: [Pane] {
        switch self {
        case .overview: return [.map]
        case .solar: return [.spaceWeather, .bands]
        case .sky: return [.sun, .station, .satellites]
        case .hazards: return [.quakes, .weather, .storms]
        }
    }
}

/// Which panes are hidden. Same storage rule as StationStore: iCloud key-value store
/// (tvOS has no guaranteed local storage) mirrored to UserDefaults. Default is all visible.
@MainActor
final class PaneStore: ObservableObject {
    private static let key = "panes.hidden"
    private let kvs = NSUbiquitousKeyValueStore.default

    @Published private(set) var hidden: Set<String>

    init() {
        let saved = kvs.array(forKey: Self.key) as? [String]
            ?? UserDefaults.standard.stringArray(forKey: Self.key) ?? []
        hidden = Set(saved)
        NotificationCenter.default.addObserver(forName: NSUbiquitousKeyValueStore.didChangeExternallyNotification,
                                               object: kvs, queue: .main) { [weak self] _ in
            Task { @MainActor in
                if let v = self?.kvs.array(forKey: Self.key) as? [String] { self?.hidden = Set(v) }
            }
        }
    }

    @Published private(set) var autoRotate: Bool = UserDefaults.standard.bool(forKey: "panes.autoRotate")

    /// Pages with at least one visible pane, in order. Never empty (falls back to the overview).
    var pages: [Page] {
        let p = Page.allCases.filter { page in page.panes.contains { isOn($0) } }
        return p.isEmpty ? [.overview] : p
    }

    func next(after page: Page) -> Page {
        let p = pages
        guard let i = p.firstIndex(of: page) else { return p[0] }
        return p[(i + 1) % p.count]
    }

    /// Hours without remote input before the dashboard dims; 0 = never (default).
    @Published private(set) var dimAfterHours: Int = UserDefaults.standard.integer(forKey: "panes.dimAfterHours")

    func setDimAfterHours(_ hours: Int) {
        dimAfterHours = hours
        UserDefaults.standard.set(hours, forKey: "panes.dimAfterHours")
    }

    func setAutoRotate(_ on: Bool) {
        autoRotate = on
        UserDefaults.standard.set(on, forKey: "panes.autoRotate")
    }

    func isOn(_ pane: Pane) -> Bool { !hidden.contains(pane.rawValue) }

    func set(_ pane: Pane, on: Bool) {
        if on { hidden.remove(pane.rawValue) } else { hidden.insert(pane.rawValue) }
        let list = hidden.sorted()
        kvs.set(list, forKey: Self.key)
        kvs.synchronize()
        UserDefaults.standard.set(list, forKey: Self.key)
    }
}
