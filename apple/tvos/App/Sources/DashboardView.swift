import SwiftUI

/// What the Siri Remote focus can rest on.
enum DashFocus: Hashable { case settings, station, pane(Int), map }

/// Modal layers over the dashboard; Menu closes the top one.
enum DashOverlay: Equatable { case chooser(Int), options(Int), mapMenu }

/// One page, like Main.qml: header, station column on the left, a row of pane slots over the
/// world map on the right (or two rows of panes when the map is hidden).
struct DashboardView: View {
    @EnvironmentObject var settings: AppSettings
    @FocusState private var focus: DashFocus?
    @State private var overlay: DashOverlay?
    @State private var showSettings = false
    @State private var setupMode = false
    @State private var bannerMode = 0
    @State private var showLocal = false
    @State private var map = MapView()
    @State private var lastInput = Date()
    /// When each rotating slot was last stepped by hand (its rotation restarts from there).
    @State private var rotationRestart: [Int: Date] = [:]

    private var theme: Theme { Theme(dark: settings.dark, accentStyle: settings.accentStyle) }
    private var paneCount: Int { settings.mapSize == "hidden" ? 6 : 3 }

    var body: some View {
        TimelineView(.periodic(from: .now, by: 1)) { ctx in
            ZStack {
                theme.canvas.ignoresSafeArea()
                page(ctx.date)
                    .offset(BurnInShift.offset(at: ctx.date))
                    .disabled(overlay != nil || showSettings)
                    .opacity(DimPolicy.opacity(idle: ctx.date.timeIntervalSince(lastInput), afterHours: settings.dimAfterHours))
                if let overlay { overlayView(overlay) }
                if showSettings {
                    SettingsView(setup: setupMode) { closeSettings() }
                        .transition(.opacity)
                }
            }
        }
        .environment(\.theme, theme)
        .preferredColorScheme(settings.dark ? .dark : .light)
        .onAppear {
            UIApplication.shared.isIdleTimerDisabled = true // the dashboard is meant to stay on
            if !settings.configured { setupMode = true; showSettings = true }
            // UI-test hook: start as if the remote had been idle for a day (dimmed when dimming is on).
            if ProcessInfo.processInfo.arguments.contains("-hcpIdle") { lastInput = Date().addingTimeInterval(-86400) }
        }
        .onDisappear { UIApplication.shared.isIdleTimerDisabled = false }
        .onPlayPauseCommand { lastInput = Date(); playPause() }
        .onMoveCommand { _ in lastInput = Date() }
        // Menu closes Settings or an open overlay (also before the focus has moved into it); with
        // nothing open there is NO handler, so tvOS takes the app back to the Home screen.
        .onExitCommand(perform: overlay == nil && !showSettings ? nil : { showSettings ? closeSettings() : closeOverlay() })
        .onChange(of: overlay) { _, _ in lastInput = Date() }
        .onChange(of: showSettings) { _, _ in lastInput = Date() }
        // A rotation just switched on (pane options) starts its first interval now.
        .onChange(of: settings.paneRotations) { old, new in
            for i in new.indices where i >= old.count || old[i] != new[i] { rotationRestart[i] = Date() }
        }
    }

    // MARK: layout

    private func page(_ now: Date) -> some View {
        VStack(alignment: .leading, spacing: Theme.gap) {
            HStack {
                Text("HamClock Pro").font(.system(size: Theme.fontTitle * 1.15, weight: .bold)).foregroundStyle(theme.ink)
                Spacer()
                Button("Settings") { openSettings() }
                    .buttonStyle(ChipStyle())
                    .focused($focus, equals: .settings)
                    .accessibilityIdentifier("settingsButton")
            }
            .frame(height: 70)
            // Full-width section: Up from any pane (even one whose left/right step a rotation) reaches Settings.
            .frame(maxWidth: .infinity)
            .focusSection()
            HStack(alignment: .top, spacing: Theme.gap) {
                RemoteTile(select: { woke(); settings.configured ? showLocal.toggle() : openSettings() }, hold: { woke(); openSettings() }) {
                    StationCardView(now: now, focused: focus == .station, bannerMode: bannerMode,
                                    showLocal: showLocal, longPath: map.longPath)
                }
                .frame(width: 400)
                .focused($focus, equals: .station)
                VStack(spacing: Theme.gap) {
                    paneGrid(now)
                    if settings.mapSize != "hidden" { mapCard(now) }
                }
            }
        }
        .focusSection()
    }

    private func paneGrid(_ now: Date) -> some View {
        let rows = paneCount / 3
        return VStack(spacing: Theme.gap) {
            ForEach(0..<rows, id: \.self) { r in
                HStack(spacing: Theme.gap) {
                    ForEach(0..<3, id: \.self) { c in pane(r * 3 + c, now) }
                }
            }
        }
        .frame(height: settings.mapSize == "hidden" ? nil : 390)
    }

    private func pane(_ i: Int, _ now: Date) -> some View {
        let rotating = isRotating(i)
        return RemoteTile(select: { overlay = .chooser(i) }, hold: { overlay = .options(i) }) {
            PaneWidget(id: shownWidget(i, now), now: now, focused: focus == .pane(i))
                .overlay(alignment: .bottom) {
                    if rotating && focus == .pane(i) { RotationHint(seconds: settings.paneRotations[i]) }
                }
        }
        .focused($focus, equals: .pane(i))
        // While a pane rotates, left/right step it by hand (PaneSlot.qml); up/down still move on.
        // Attached only to rotating panes, so other panes never see a move handler at all.
        .modifier(RotationStepper(enabled: rotating) { stepRotation(i, $0) })
        .accessibilityIdentifier("pane\(i)")
        .accessibilityValue(settings.panes[i])
        .accessibilityHint(rotating ? "Rotating. Left and right change the widget." : "")
    }

    private func mapCard(_ now: Date) -> some View {
        RemoteTile(select: { overlay = .mapMenu }, hold: { overlay = .mapMenu }) {
            Card(title: "World map" + (map.centre == .world ? "" : map.centre == .de ? " · DE" : " · DX"), chooser: true,
                 focused: focus == .map) {
                WorldMapView(now: now, view: map)
            }
        }
        .focused($focus, equals: .map)
        .accessibilityLabel("World map")
        .accessibilityIdentifier("mapCard")
    }


    /// The saved widget, or the current stop of the slot's automatic rotation (never saved).
    private func shownWidget(_ i: Int, _ now: Date) -> String {
        PaneRotation.shown(saved: settings.panes[i], seconds: settings.paneRotations[i], restart: rotationRestart[i], now: now)
    }

    private func isRotating(_ i: Int) -> Bool { settings.paneRotations[i] > 0 && settings.panes[i] != "none" }

    /// Left/right on a rotating pane: show the previous/next widget now (saved, as PaneSlot.step
    /// does) and restart the slot's interval so it does not change again straight away.
    private func stepRotation(_ i: Int, _ direction: MoveCommandDirection) {
        lastInput = Date()
        let by: Int
        switch direction {
        case .left: by = -1
        case .right: by = 1
        default: return
        }
        let next = PaneRotation.step(from: shownWidget(i, Date()), by: by)
        rotationRestart[i] = Date()
        settings.setPane(i, next)
        focus = .pane(i)
    }

    // MARK: overlays

    @ViewBuilder private func overlayView(_ o: DashOverlay) -> some View {
        switch o {
        case .chooser(let slot):
            PaneChooserView(slot: slot) { overlay = nil; focus = .pane(slot) }
        case .options(let slot):
            PaneOptionsView(slot: slot, change: { overlay = .chooser(slot) }) { overlay = nil; focus = .pane(slot) }
        case .mapMenu:
            MapMenuView(view: $map) { overlay = nil; focus = .map }
        }
    }

    /// Any remote press counts as activity (restores a dimmed screen).
    private func woke() { lastInput = Date() }

    /// Closes the overlay and puts the focus back where it was opened from.
    private func closeOverlay() {
        switch overlay {
        case .chooser(let i), .options(let i): focus = .pane(i)
        case .mapMenu: focus = .map
        case nil: break
        }
        overlay = nil
    }

    private func playPause() {
        guard overlay == nil, !showSettings else { return }
        switch focus {
        case .pane(let i): overlay = .options(i)
        case .station: bannerMode += 1
        case .map: overlay = .mapMenu
        default: break
        }
    }

    private func openSettings() { setupMode = !settings.configured; showSettings = true }

    private func closeSettings() {
        showSettings = false
        setupMode = false
        focus = .station
    }
}

/// Burn-in protection: the whole layout drifts one point per minute around a 9-point square
/// ring (max 4 pt from centre, well inside the title-safe margins).
enum BurnInShift {
    static func offset(at date: Date) -> CGSize {
        let minute = Int(date.timeIntervalSince1970 / 60)
        let side = 8, step = minute % (side * 4)
        let (x, y): (Int, Int) = step < side ? (step, 0) : step < 2 * side ? (side, step - side)
            : step < 3 * side ? (3 * side - step, side) : (0, 4 * side - step)
        return CGSize(width: x / 2 - 2, height: y / 2 - 2)
    }
}

/// Optional night dimming: after `afterHours` without remote input the dashboard drops to 25 %.
enum DimPolicy {
    static let dimmedOpacity = 0.25
    static func opacity(idle: TimeInterval, afterHours: Int) -> Double {
        afterHours > 0 && idle >= Double(afterHours) * 3600 ? dimmedOpacity : 1
    }
}

/// Shown on a focused rotating pane: the remote's left/right step it (owner request).
private struct RotationHint: View {
    @Environment(\.theme) private var theme
    let seconds: Int
    var body: some View {
        Text("◀ ▶ change · rotates every \(seconds) s")
            .font(.system(size: Theme.fontCaption - 3, weight: .semibold))
            .foregroundStyle(theme.accentInk)
            .padding(.horizontal, 18).padding(.vertical, 6)
            .background(Capsule().fill(theme.accent))
            .offset(y: 18)
            .accessibilityHidden(true)
    }
}

/// Installs the left/right stepping handler only while the pane rotates (no handler otherwise).
private struct RotationStepper: ViewModifier {
    let enabled: Bool
    let step: (MoveCommandDirection) -> Void
    func body(content: Content) -> some View {
        if enabled {
            content.onMoveCommand(perform: step)
        } else {
            content
        }
    }
}
