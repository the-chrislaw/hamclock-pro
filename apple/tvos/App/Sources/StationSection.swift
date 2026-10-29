import SwiftUI

/// Settings > Station: callsign, home by place search (OpenStreetMap Nominatim, as the main app)
/// or grid square, DX grid, time zones. Nothing is sent anywhere until Find is pressed.
struct StationSection: View {
    @EnvironmentObject var settings: AppSettings
    @ObservedObject private var zones = ZoneChoice.shared
    @Environment(\.theme) private var theme
    let setup: Bool
    let pickZone: (ZonePickerTarget) -> Void
    let saved: () -> Void

    @State private var callsign = ""
    @State private var query = ""
    @State private var grid = ""
    @State private var dxGrid = ""
    @State private var home: (lat: Double, lon: Double)?
    @State private var placeName = ""
    @State private var results: [PlaceResult] = []
    @State private var status = ""
    @State private var error = ""
    @State private var searching = false
    @State private var lastSearch = Date.distantPast

    // Split into small pieces: one large builder exceeds Xcode 16's type-checking time limit.
    var body: some View {
        SettingsCard(title: "Station") {
            FormField(label: "Callsign", placeholder: "e.g. W1AW", text: $callsign, id: "callsignField")
            placeSearch
            gridFields
            summary
            zoneButtons
            messages
            saveRow
        }
        .onAppear(perform: loadFields)
        .onChange(of: grid) { _, text in gridTyped(text) }
    }

    private var placeSearch: some View {
        VStack(alignment: .leading, spacing: Theme.gap * 0.8) {
            HStack(alignment: .bottom, spacing: Theme.gap) {
                FormField(label: "Find a place (city or address; sent to OpenStreetMap when you press Find)",
                          placeholder: "e.g. Portland, Oregon", text: $query, id: "placeField") { find() }
                Button(searching ? "Searching…" : "Find") { find() }
                    .buttonStyle(ChipStyle())
                    .disabled(searching)
                    .accessibilityIdentifier("findButton")
            }
            if !results.isEmpty {
                VStack(alignment: .leading, spacing: 10) {
                    ForEach(results) { r in
                        Button(r.name) { use(r) }.buttonStyle(ChipStyle(wide: true))
                    }
                }
                .focusSection()
            }
        }
    }

    private var gridFields: some View {
        HStack(alignment: .bottom, spacing: Theme.gap) {
            FormField(label: "Or grid square (fills the home coordinates)", placeholder: "e.g. FN31pr",
                      text: $grid, id: "gridField") { applyGrid() }
            FormField(label: "DX target grid square (empty = same as home)", placeholder: "e.g. JN58td",
                      text: $dxGrid, id: "dxGridField")
        }
    }

    private var summaryText: String {
        guard let home else { return "Home: not set" }
        let place = placeName.isEmpty ? "" : " · " + placeName
        return "Home: " + StationText.coordinates(home.lat, home.lon) + " · " + StationText.grid(home.lat, home.lon) + place
    }

    private var summary: some View {
        Text(summaryText)
            .font(.system(size: Theme.fontCaption))
            .foregroundStyle(theme.secondary)
            .accessibilityIdentifier("homeSummary")
    }

    private var zoneButtons: some View {
        let de: String = zones.de.isEmpty ? "not set" : zones.de
        let dx: String = zones.dx.isEmpty ? "automatic" : zones.dx
        return HStack(spacing: Theme.gap) {
            Button("Time zone: " + de) { pickZone(.de) }.buttonStyle(ChipStyle())
            Button("DX zone: " + dx) { pickZone(.dx) }.buttonStyle(ChipStyle())
        }
    }

    @ViewBuilder private var messages: some View {
        if !status.isEmpty { Text(status).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary) }
        if !error.isEmpty { Text(error).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.bad) }
    }

    private var saveRow: some View {
        HStack {
            Spacer()
            Button(setup ? "Save and start" : "Save station") { save() }
                .buttonStyle(ChipStyle(primary: true))
                .accessibilityIdentifier("saveStation")
        }
        .focusSection() // Down from any field above reaches Save
    }

    /// The tvOS keyboard returns with Menu as often as with Done: take a valid square as typed.
    private func gridTyped(_ text: String) {
        var lat = 0.0, lon = 0.0
        let t = text.trimmingCharacters(in: .whitespaces)
        guard hcp_locator_center(t, &lat, &lon) != 0 else { return }
        if let home, StationText.grid(home.lat, home.lon).lowercased() == t.lowercased() { return }
        applyGrid()
    }


    private func loadFields() {
        callsign = settings.callsign
        home = settings.de
        placeName = settings.place
        grid = home.map { StationText.grid($0.lat, $0.lon) } ?? ""
        if let dx = settings.dx, settings.dx.map({ $0.lat != home?.lat || $0.lon != home?.lon }) == true {
            dxGrid = StationText.grid(dx.lat, dx.lon)
        }
        zones.de = settings.deZone.isEmpty ? TimeZone.current.identifier : settings.deZone
        zones.dx = settings.dxZone
    }

    private func applyGrid() {
        var lat = 0.0, lon = 0.0
        guard hcp_locator_center(grid.trimmingCharacters(in: .whitespaces), &lat, &lon) != 0 else {
            if !grid.isEmpty { error = "\(grid) is not a Maidenhead grid square (2, 4 or 6 characters)." }
            return
        }
        error = ""
        home = (lat, lon)
        placeName = ""
        suggestZone(lat, lon)
    }

    private func use(_ r: PlaceResult) {
        home = (r.lat, r.lon)
        placeName = r.name.components(separatedBy: ",").prefix(2).joined(separator: ",")
        grid = StationText.grid(r.lat, r.lon)
        results = []
        status = "Using: " + r.name
        suggestZone(r.lat, r.lon)
    }

    /// Open-Meteo's zone for the coordinates (the same service the weather pane uses), else the
    /// main app's reference-city table. Shown as a suggestion the operator can change.
    private func suggestZone(_ lat: Double, _ lon: Double) {
        let fallback = TimeZones.suggest(lat: lat, lon: lon)
        if !fallback.isEmpty { zones.de = fallback }
        Task {
            let url = URL(string: String(format: "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f&current=temperature_2m&forecast_days=1&timezone=auto", lat, lon))
            guard let url, let data = await Net.get(url) else { return }
            let z = cString(64) { buf, cap in data.withC { hcp_open_meteo_timezone($0, $1, buf, cap) } }
            if TimeZone(identifier: z) != nil { zones.de = z; status = "Suggested time zone: \(z). Check it before saving." }
        }
    }

    private func find() {
        let q = query.trimmingCharacters(in: .whitespaces)
        guard q.count >= 3 else { status = "Type at least 3 characters to search."; return }
        guard Date().timeIntervalSince(lastSearch) >= 1 else { status = "Please wait a second and try again."; return }
        var c = URLComponents(string: "https://nominatim.openstreetmap.org/search")!
        c.queryItems = [.init(name: "q", value: q), .init(name: "format", value: "jsonv2"), .init(name: "limit", value: "5")]
        guard let url = c.url else { return }
        searching = true
        lastSearch = Date()
        status = "Searching…"
        Task {
            defer { searching = false }
            guard let data = await Net.get(url) else { status = "Search failed. Check the network and try again."; return }
            var raw = [HcpPlace](repeating: HcpPlace(), count: 5)
            let n = raw.withUnsafeMutableBufferPointer { o in data.withC { hcp_nominatim_parse($0, $1, o.baseAddress, 5) } }
            results = raw.prefix(max(0, Int(n))).enumerated().map { PlaceResult(id: $0.offset, name: cText($0.element.name), lat: $0.element.lat, lon: $0.element.lon) }
            status = results.isEmpty ? "No match." : "Choose a result:"
        }
    }

    private func save() {
        let typed = grid.trimmingCharacters(in: .whitespaces)
        if !typed.isEmpty, home.map({ StationText.grid($0.lat, $0.lon).lowercased() != typed.lowercased() }) ?? true {
            applyGrid()
            if !error.isEmpty { return }
        }
        guard let de = home else {
            error = "Add your home location: press Find, or type a grid square."
            return
        }
        var dx: (lat: Double, lon: Double)?
        let dxText = dxGrid.trimmingCharacters(in: .whitespaces)
        if !dxText.isEmpty {
            var lat = 0.0, lon = 0.0
            guard hcp_locator_center(dxText, &lat, &lon) != 0 else { error = "\(dxText) is not a grid square."; return }
            dx = (lat, lon)
        }
        if let message = settings.saveStation(callsign: callsign, de: de, dx: dx ?? de,
                                              zone: zones.de, place: placeName) {
            error = message
            return
        }
        settings.saveDxZone(zones.dx)
        error = ""
        status = "Saved."
        saved()
    }
}

struct PlaceResult: Identifiable {
    let id: Int
    let name: String
    let lat: Double
    let lon: Double
}
