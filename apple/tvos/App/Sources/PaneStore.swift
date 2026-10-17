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

    func isOn(_ pane: Pane) -> Bool { !hidden.contains(pane.rawValue) }

    func set(_ pane: Pane, on: Bool) {
        if on { hidden.remove(pane.rawValue) } else { hidden.insert(pane.rawValue) }
        let list = hidden.sorted()
        kvs.set(list, forKey: Self.key)
        kvs.synchronize()
        UserDefaults.standard.set(list, forKey: Self.key)
    }
}
