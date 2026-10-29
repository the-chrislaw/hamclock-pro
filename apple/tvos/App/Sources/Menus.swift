import SwiftUI

/// Widget list for one pane (PaneChooser.qml): category tabs, then every widget with a TV data
/// source; the current one is in the accent colour. Select picks, Menu closes.
struct PaneChooserView: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    let slot: Int
    let close: () -> Void
    @State private var category = 0
    @FocusState private var focused: String?

    var body: some View {
        let current = settings.panes[slot]
        let items = WidgetCatalog.onTV.filter { category == 0 || $0.category == category }
        Overlay(title: "Show in pane \(slot + 1)", width: 1260, close: close) {
            HStack(spacing: 12) {
                ForEach(WidgetCatalog.tabs, id: \.self) { c in
                    Button(WidgetCatalog.categoryNames[c]) { category = c }
                        .buttonStyle(ChipStyle(primary: category == c))
                        .focused($focused, equals: "tab\(c)")
                        .accessibilityIdentifier("tab\(c)")
                        .accessibilityAddTraits(category == c ? .isSelected : [])
                }
            }
            .focusSection()
            ScrollView {
                // Not lazy: every widget exists for the focus engine even below the fold.
                Grid(horizontalSpacing: 16, verticalSpacing: 14) {
                    ForEach(Array(stride(from: 0, to: items.count, by: 2)), id: \.self) { row in
                        GridRow {
                            ForEach(items[row..<min(row + 2, items.count)]) { w in
                                Button { settings.setPane(slot, w.id); close() } label: {
                                    Text((w.id == current ? "✓ " : "") + w.title)
                                }
                                .buttonStyle(ChipStyle(wide: true))
                                .focused($focused, equals: w.id)
                                .accessibilityIdentifier("choose_\(w.id)")
                            }
                        }
                    }
                }
                .padding(12)
            }
            .frame(maxHeight: 620)
            .focusSection()
            Button("Empty pane") { settings.setPane(slot, "none"); close() }
                .buttonStyle(ChipStyle(wide: true))
                .focused($focused, equals: "none")
        }
        .defaultFocus($focused, WidgetCatalog.onTV.contains { $0.id == current } ? current : "tab0")
        .onAppear {
            // A pane saved by another device may sit in a hidden tab: show All then.
            category = WidgetCatalog.info(current).map { WidgetCatalog.tabs.contains($0.category) && $0.tv ? $0.category : 0 } ?? 0
            // Start on the pane's current widget, as PaneChooser.qml does.
            let start = WidgetCatalog.onTV.contains { $0.id == current } ? current : "tab0"
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) { focused = start }
        }
    }
}

/// Long-press / Play-Pause menu for one pane (PaneOptions.qml).
struct PaneOptionsView: View {
    @EnvironmentObject var settings: AppSettings
    let slot: Int
    let change: () -> Void
    let close: () -> Void
    private static let steps = [0, 5, 10, 30, 60]
    @FocusState private var startHere: Bool

    var body: some View {
        let rotate = settings.paneRotations[slot]
        Overlay(title: "Pane \(slot + 1)", width: 640, close: close) {
            VStack(spacing: 14) {
                Button("Change widget…", action: change).buttonStyle(ChipStyle(wide: true))
                    .focused($startHere)
                    .accessibilityIdentifier("optChange")
                Button("Next widget") { step(1) }.buttonStyle(ChipStyle(wide: true)).accessibilityIdentifier("optNext")
                Button("Previous widget") { step(-1) }.buttonStyle(ChipStyle(wide: true)).accessibilityIdentifier("optPrevious")
                Button("Rotate: " + (rotate > 0 ? "every \(rotate) s" : "off")) {
                    let i = Self.steps.firstIndex(of: rotate) ?? 0
                    settings.setRotation(slot, seconds: Self.steps[(i + 1) % Self.steps.count])
                }
                .buttonStyle(ChipStyle(wide: true)).accessibilityIdentifier("optRotate")
                Button("Empty pane") { settings.setPane(slot, "none"); close() }.buttonStyle(ChipStyle(wide: true))
                    .accessibilityIdentifier("optEmpty")
                Button("Reset all panes") { settings.resetPanes(); close() }.buttonStyle(ChipStyle(wide: true))
                    .accessibilityIdentifier("optReset")
            }
        }
        .onAppear { DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) { startHere = true } }
    }

    private func step(_ d: Int) {
        settings.setPane(slot, WidgetCatalog.step(from: settings.panes[slot], by: d))
        close()
    }
}

/// Map menu (MapMenu.qml): view centre, zoom, path, overlays.
struct MapMenuView: View {
    @EnvironmentObject var settings: AppSettings
    @Environment(\.theme) private var theme
    @Binding var view: MapView
    let close: () -> Void
    @FocusState private var centreFocus: MapView.Centre?

    var body: some View {
        Overlay(title: "Map", width: 1100, close: close) {
            row("Centre") {
                ForEach(MapView.Centre.allCases, id: \.self) { c in
                    Button(c == .world ? "World" : c == .de ? "DE" : "DX") { view.centre = c }
                        .buttonStyle(ChipStyle(primary: view.centre == c))
                        .focused($centreFocus, equals: c)
                        .disabled(c != .world && !settings.configured)
                }
            }
            row("Zoom") {
                ForEach([1.0, 2, 3, 4], id: \.self) { z in
                    Button("\(Int(z))×") { view.zoom = z }.buttonStyle(ChipStyle(primary: view.zoom == z))
                }
            }
            row("Path") {
                Button("Short path") { view.longPath = false }.buttonStyle(ChipStyle(primary: !view.longPath))
                Button("Long path") { view.longPath = true }.buttonStyle(ChipStyle(primary: view.longPath))
            }
            Text("Show on map").font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            LayerGrid()
        }
        .onAppear { DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) { centreFocus = view.centre } }
    }

    private func row<C: View>(_ label: String, @ViewBuilder _ content: () -> C) -> some View {
        HStack(spacing: 12) {
            Text(label).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).frame(width: 130, alignment: .leading)
            content()
        }
        .focusSection()
    }
}

/// Overlay switches, shared by the map menu and Settings.
struct LayerGrid: View {
    @EnvironmentObject var settings: AppSettings
    var body: some View {
        LazyVGrid(columns: Array(repeating: GridItem(.flexible(), spacing: 14), count: 3), spacing: 14) {
            ForEach(MapLayer.allCases) { layer in
                let on = settings.isOn(layer)
                Button((on ? "✓ " : "   ") + layer.title) { settings.setLayer(layer, on: !on) }
                    .buttonStyle(ChipStyle(primary: on, wide: true))
                    .accessibilityIdentifier("layer_\(layer.rawValue)")
            }
        }
    }
}
