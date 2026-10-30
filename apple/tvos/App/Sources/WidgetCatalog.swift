import Foundation

/// The main app's widget ids and titles (src/widgets/*/…_model.cpp registry.add) and the
/// chooser categories (src/ui/qml/PaneChooser.qml). `tv` marks widgets with a working TV data
/// source; only those are offered. Panes saved by another device with a non-TV widget show why.
struct WidgetInfo: Identifiable, Equatable {
    let id: String
    let title: String
    let category: Int
    let tv: Bool
}

enum WidgetCatalog {
    static let categoryNames = ["All", "Space Wx", "DX & Contest", "Weather", "Sky & Space", "Station"]

    static let all: [WidgetInfo] = [
        .init(id: "spaceweather", title: "Space weather", category: 1, tv: true),
        .init(id: "solarwind", title: "Solar wind", category: 1, tv: true),
        .init(id: "bands", title: "Band conditions", category: 1, tv: true),
        .init(id: "solarscales", title: "NOAA scales", category: 1, tv: true),
        .init(id: "solaraurora", title: "Aurora", category: 1, tv: true),
        .init(id: "solardrap", title: "D-RAP", category: 1, tv: true),
        .init(id: "solarimage", title: "Sun (SDO)", category: 1, tv: true),
        .init(id: "solaralerts", title: "Space alerts", category: 1, tv: false),
        .init(id: "solarhamqsl", title: "HamQSL conditions", category: 1, tv: false),
        .init(id: "bandsmatrix", title: "HF path reliability", category: 1, tv: false),
        .init(id: "solartrend", title: "Solar trend", category: 1, tv: false),
        .init(id: "solardst", title: "Dst index", category: 1, tv: true),
        .init(id: "solarimpact", title: "Space weather impact", category: 1, tv: false),
        .init(id: "ota", title: "On the air", category: 2, tv: true),
        .init(id: "dxcluster", title: "DX cluster", category: 2, tv: false),
        .init(id: "pskreporter", title: "PSKReporter", category: 2, tv: false),
        .init(id: "beacons", title: "Beacons", category: 2, tv: false),
        .init(id: "watchlist", title: "Watch list", category: 2, tv: false),
        .init(id: "weather", title: "Weather", category: 3, tv: true),
        .init(id: "quakes", title: "Earthquakes", category: 3, tv: true),
        .init(id: "storms", title: "Tropical storms", category: 3, tv: true),
        .init(id: "fires", title: "Active fires", category: 3, tv: true),
        .init(id: "sunmoon", title: "Sun & Moon", category: 4, tv: true),
        .init(id: "satellites", title: "Satellites", category: 4, tv: true),
        .init(id: "planets", title: "Planets", category: 4, tv: true),
        .init(id: "eclipse", title: "Next eclipse", category: 4, tv: true),
        .init(id: "launches", title: "Launches", category: 4, tv: true),
        .init(id: "passes", title: "Passes", category: 4, tv: false),
        .init(id: "hamsat", title: "hams.at", category: 4, tv: false),
        .init(id: "radio", title: "Radio", category: 5, tv: false),
        .init(id: "rotator", title: "Rotator", category: 5, tv: false),
    ]

    static var onTV: [WidgetInfo] { all.filter(\.tv) }

    static func info(_ id: String) -> WidgetInfo? { all.first { $0.id == id } }

    /// Categories with at least one TV widget (a tab shows only when it has panes on this device).
    static var tabs: [Int] {
        (0..<categoryNames.count).filter { c in c == 0 || onTV.contains { $0.category == c } }
    }

    /// Next/previous TV widget in registry order; "none" sits between last and first (PaneSlot.step).
    static func step(from id: String, by direction: Int) -> String {
        let list = ["none"] + onTV.map(\.id)
        let at = list.firstIndex(of: id) ?? 0
        return list[(at + direction + list.count) % list.count]
    }
}
