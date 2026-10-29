import Foundation

/// Persistent options, mirroring the main app's AppSettings (src/ui/app_settings.*).
///
/// Storage: UserDefaults (the local copy) plus NSUbiquitousKeyValueStore. Every key is a flat
/// `hcp.*` name with a plain property-list value (string, number, bool, string/int array) so the
/// same keys can be shared with the iPhone/iPad app through iCloud key-value storage. Without the
/// `com.apple.developer.ubiquity-kvstore-identifier` entitlement the KVS writes are no-ops and the
/// TV keeps its settings locally (docs/apple-tv.md section 6).
@MainActor
final class AppSettings: ObservableObject {
    static let paneSlots = 12
    /// The main app's default pane order (app_settings.cpp), with widgets that have no TV data
    /// source (dxcluster, pskreporter) replaced by ones that do.
    static let defaultPanes = ["spaceweather", "solarwind", "bands", "sunmoon", "satellites", "ota",
                               "solarimage", "weather", "launches", "quakes", "solarscales", "planets"]

    enum Key {
        static let callsign = "hcp.station.callsign"
        static let deLat = "hcp.station.deLatitude", deLon = "hcp.station.deLongitude"
        static let dxLat = "hcp.station.dxLatitude", dxLon = "hcp.station.dxLongitude"
        static let deZone = "hcp.station.deZone", dxZone = "hcp.station.dxZone", place = "hcp.station.place"
        static let appearance = "hcp.display.appearance", imperial = "hcp.display.imperialUnits"
        static let clock12h = "hcp.display.clock12h", dateOrder = "hcp.display.dateOrder"
        static let localPrimary = "hcp.display.localClockPrimary"
        static let bannerStyle = "hcp.display.bannerStyle", bannerBackground = "hcp.display.bannerBackground"
        static let bannerTitle = "hcp.display.bannerTitle", accent = "hcp.display.accentStyle"
        static let panes = "hcp.panes.slots", rotations = "hcp.panes.rotations"
        static let mapSize = "hcp.map.size", mapLayers = "hcp.map.layers"
        static let dimAfterHours = "hcp.tv.dimAfterHours"
        static let all = [callsign, deLat, deLon, dxLat, dxLon, deZone, dxZone, place, appearance, imperial,
                          clock12h, dateOrder, localPrimary, bannerStyle, bannerBackground, bannerTitle, accent,
                          panes, rotations, mapSize, mapLayers, dimAfterHours]
    }

    private let defaults: UserDefaults
    private let kvs = NSUbiquitousKeyValueStore.default

    @Published private(set) var callsign = ""
    @Published private(set) var deLatitude: Double?
    @Published private(set) var deLongitude: Double?
    @Published private(set) var dxLatitude: Double?
    @Published private(set) var dxLongitude: Double?
    @Published private(set) var deZone = ""
    @Published private(set) var dxZone = ""
    @Published private(set) var place = ""
    @Published var appearance = "dark" { didSet { put(Key.appearance, appearance) } }
    @Published var imperialUnits = false { didSet { put(Key.imperial, imperialUnits) } }
    @Published var clock12h = false { didSet { put(Key.clock12h, clock12h) } }
    @Published var dateOrder = 0 { didSet { put(Key.dateOrder, dateOrder) } }
    @Published var localClockPrimary = false { didSet { put(Key.localPrimary, localClockPrimary) } }
    @Published var bannerStyle = 0 { didSet { put(Key.bannerStyle, bannerStyle) } }
    @Published var bannerBackground = 0 { didSet { put(Key.bannerBackground, bannerBackground) } }
    @Published var bannerTitle = "" { didSet { put(Key.bannerTitle, bannerTitle) } }
    @Published var accentStyle = 0 { didSet { put(Key.accent, accentStyle) } }
    @Published private(set) var panes: [String] = AppSettings.defaultPanes
    @Published private(set) var paneRotations: [Int] = Array(repeating: 0, count: AppSettings.paneSlots)
    /// "large" shows one row of panes over the map, "small" two rows.
    @Published var mapSize = "large" { didSet { put(Key.mapSize, mapSize) } }
    @Published private(set) var mapLayers: Set<String> = MapLayer.defaultOn
    @Published var dimAfterHours = 0 { didSet { put(Key.dimAfterHours, dimAfterHours) } }

    private var loading = false

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
        if ProcessInfo.processInfo.arguments.contains("-hcpResetSettings") {
            Key.all.forEach { defaults.removeObject(forKey: $0); kvs.removeObject(forKey: $0) }
        }
        load()
        seedFromLaunchArguments()
        NotificationCenter.default.addObserver(forName: NSUbiquitousKeyValueStore.didChangeExternallyNotification,
                                               object: kvs, queue: .main) { [weak self] note in
            let keys = note.userInfo?[NSUbiquitousKeyValueStoreChangedKeysKey] as? [String] ?? []
            Task { @MainActor in self?.adoptCloud(keys) }
        }
        kvs.synchronize()
    }

    var configured: Bool { deLatitude != nil && deLongitude != nil }
    var de: (lat: Double, lon: Double)? { deLatitude.flatMap { la in deLongitude.map { (la, $0) } } }
    var dx: (lat: Double, lon: Double)? { dxLatitude.flatMap { la in dxLongitude.map { (la, $0) } } }
    var dark: Bool { appearance != "light" }

    // MARK: station

    /// Validates and saves the station; returns an error message or nil.
    func saveStation(callsign: String, de: (lat: Double, lon: Double), dx: (lat: Double, lon: Double)?,
                     zone: String, place: String) -> String? {
        let call = callsign.trimmingCharacters(in: .whitespaces).uppercased()
        guard call.count <= 12, call.allSatisfy({ $0.isLetter || $0.isNumber || $0 == "/" }) else {
            return "Callsign may contain only letters, digits and /."
        }
        guard Self.valid(de) else { return "Home location is out of range." }
        if let dx, !Self.valid(dx) { return "DX location is out of range." }
        guard zone.isEmpty || TimeZone(identifier: zone) != nil else { return "Unknown time zone \(zone)." }
        self.callsign = call; put(Key.callsign, call)
        deLatitude = de.lat; put(Key.deLat, de.lat)
        deLongitude = de.lon; put(Key.deLon, de.lon)
        let target = dx ?? self.dx ?? de
        dxLatitude = target.lat; put(Key.dxLat, target.lat)
        dxLongitude = target.lon; put(Key.dxLon, target.lon)
        deZone = zone; put(Key.deZone, zone)
        self.place = place; put(Key.place, place)
        return nil
    }

    func saveDxZone(_ zone: String) { dxZone = zone; put(Key.dxZone, zone) }

    static func valid(_ p: (lat: Double, lon: Double)) -> Bool {
        p.lat.isFinite && p.lon.isFinite && abs(p.lat) <= 90 && abs(p.lon) <= 180
    }

    // MARK: panes (same semantics as AppSettings::setPane / resetPanes)

    func setPane(_ slot: Int, _ id: String) {
        guard panes.indices.contains(slot) else { return }
        var next = panes
        next[slot] = id
        panes = next
        put(Key.panes, next)
    }

    func setRotation(_ slot: Int, seconds: Int) {
        guard paneRotations.indices.contains(slot) else { return }
        var next = paneRotations
        next[slot] = seconds
        paneRotations = next
        put(Key.rotations, next)
    }

    func resetPanes() {
        panes = Self.defaultPanes
        paneRotations = Array(repeating: 0, count: Self.paneSlots)
        put(Key.panes, panes)
        put(Key.rotations, paneRotations)
    }

    // MARK: map

    func isOn(_ layer: MapLayer) -> Bool { mapLayers.contains(layer.rawValue) }

    func setLayer(_ layer: MapLayer, on: Bool) {
        var next = mapLayers
        if on { next.insert(layer.rawValue) } else { next.remove(layer.rawValue) }
        mapLayers = next
        put(Key.mapLayers, next.sorted())
    }

    // MARK: storage

    private func put(_ key: String, _ value: Any) {
        guard !loading else { return }
        defaults.set(value, forKey: key)
        kvs.set(value, forKey: key)
    }

    private func value(_ key: String) -> Any? { kvs.object(forKey: key) ?? defaults.object(forKey: key) }

    private func load() {
        loading = true
        defer { loading = false }
        callsign = value(Key.callsign) as? String ?? ""
        deLatitude = value(Key.deLat) as? Double
        deLongitude = value(Key.deLon) as? Double
        dxLatitude = value(Key.dxLat) as? Double
        dxLongitude = value(Key.dxLon) as? Double
        deZone = value(Key.deZone) as? String ?? ""
        dxZone = value(Key.dxZone) as? String ?? ""
        place = value(Key.place) as? String ?? ""
        appearance = value(Key.appearance) as? String ?? "dark"
        imperialUnits = value(Key.imperial) as? Bool ?? false
        clock12h = value(Key.clock12h) as? Bool ?? false
        dateOrder = value(Key.dateOrder) as? Int ?? 0
        localClockPrimary = value(Key.localPrimary) as? Bool ?? false
        bannerStyle = value(Key.bannerStyle) as? Int ?? 0
        bannerBackground = value(Key.bannerBackground) as? Int ?? 0
        bannerTitle = value(Key.bannerTitle) as? String ?? ""
        accentStyle = value(Key.accent) as? Int ?? 0
        mapSize = value(Key.mapSize) as? String ?? "large"
        dimAfterHours = value(Key.dimAfterHours) as? Int ?? 0
        if let saved = value(Key.panes) as? [String] {
            panes = (0..<Self.paneSlots).map { $0 < saved.count && !saved[$0].isEmpty ? saved[$0] : Self.defaultPanes[$0] }
        }
        if let saved = value(Key.rotations) as? [Int] {
            paneRotations = (0..<Self.paneSlots).map { $0 < saved.count ? saved[$0] : 0 }
        }
        if let saved = value(Key.mapLayers) as? [String] { mapLayers = Set(saved) }
        migrateLegacyLocator()
    }

    /// Screenshot and UI-test hook: `-hcpStation CALL DEGRID DXGRID ZONE` sets a station.
    private func seedFromLaunchArguments() {
        let args = ProcessInfo.processInfo.arguments
        if let p = args.firstIndex(of: "-hcpPanes"), args.count > p + 1 {
            for (slot, id) in args[p + 1].split(separator: ",").enumerated() { setPane(slot, String(id)) }
        }
        if let m = args.firstIndex(of: "-hcpMapSize"), args.count > m + 1 { mapSize = args[m + 1] }
        guard let i = args.firstIndex(of: "-hcpStation"), args.count > i + 4 else { return }
        var de = (0.0, 0.0), dx = (0.0, 0.0)
        guard hcp_locator_center(args[i + 2], &de.0, &de.1) != 0, hcp_locator_center(args[i + 3], &dx.0, &dx.1) != 0 else { return }
        _ = saveStation(callsign: args[i + 1], de: (de.0, de.1), dx: (dx.0, dx.1), zone: args[i + 4], place: "")
    }

    /// Builds before this one stored only a Maidenhead locator ("station.locator").
    private func migrateLegacyLocator() {
        guard deLatitude == nil, let old = defaults.string(forKey: "station.locator") else { return }
        var lat = 0.0, lon = 0.0
        guard hcp_locator_center(old, &lat, &lon) != 0 else { return }
        loading = false
        deLatitude = lat; put(Key.deLat, lat)
        deLongitude = lon; put(Key.deLon, lon)
        if let dxOld = defaults.string(forKey: "station.dxLocator"), hcp_locator_center(dxOld, &lat, &lon) != 0 {
            dxLatitude = lat; put(Key.dxLat, lat)
            dxLongitude = lon; put(Key.dxLon, lon)
        }
        loading = true
    }

    private func adoptCloud(_ keys: [String]) {
        guard keys.contains(where: { Key.all.contains($0) }) else { return }
        keys.forEach { key in if let v = kvs.object(forKey: key) { defaults.set(v, forKey: key) } }
        load()
    }
}
