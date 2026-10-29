import SwiftUI

/// Small pill: coloured dot + word, so state never relies on colour alone (StatusBadge.qml).
struct StatusBadge: View {
    @Environment(\.theme) private var theme
    let state: WidgetState
    var body: some View {
        let c = theme.stateColor(state)
        HStack(spacing: 10) {
            Circle().fill(c).frame(width: 14, height: 14)
            Text(state.label).font(.system(size: Theme.fontCaption, weight: .semibold)).foregroundStyle(c)
        }
        .padding(.horizontal, 16).padding(.vertical, 4)
        .background(Capsule().fill(c.opacity(0.14)))
        .fixedSize()
        .accessibilityElement(children: .combine)
    }
}

/// Standard widget card (Card.qml): title (with ▾ when it opens the chooser) and badge, the
/// content, then a status line with the source and the data age.
struct Card<Content: View>: View {
    @Environment(\.theme) private var theme
    let title: String
    var chooser = false
    var state: WidgetState?
    var status = ""
    var age = ""
    var focused = false
    var showTitle = true
    @ViewBuilder var content: () -> Content

    var body: some View {
        VStack(alignment: .leading, spacing: Theme.gap * 0.6) {
            if showTitle {
                HStack(alignment: .center, spacing: 12) {
                    Text(title + (chooser ? " ▾" : ""))
                        .font(.system(size: Theme.fontTitle, weight: .semibold))
                        .foregroundStyle(theme.ink).lineLimit(1).minimumScaleFactor(0.7)
                    Spacer(minLength: 0)
                    if let state { StatusBadge(state: state) }
                }
            }
            // The body never sizes the card: content lays out inside whatever the pane gives it.
            Color.clear
                .overlay(alignment: .topLeading) {
                    VStack(alignment: .leading, spacing: 10) { content() }
                        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                }
                .clipped()
            if !status.isEmpty || !age.isEmpty {
                HStack(alignment: .firstTextBaseline, spacing: 12) {
                    Text(status).lineLimit(1).truncationMode(.tail)
                    Spacer(minLength: 0)
                    Text(age).lineLimit(1).fixedSize()
                }
                .font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary)
            }
        }
        .padding(Theme.pad)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .background(RoundedRectangle(cornerRadius: Theme.radius).fill(theme.surface))
        .overlay(RoundedRectangle(cornerRadius: Theme.radius)
            .strokeBorder(focused ? theme.focus : theme.divider, lineWidth: focused ? 5 : 1.5))
        .scaleEffect(focused ? 1.015 : 1)
        .shadow(color: .black.opacity(focused ? 0.45 : 0), radius: 18, y: 8)
        .animation(.easeOut(duration: 0.15), value: focused)
    }
}

/// One labelled value (Metric.qml): small upper-case label, big value, optional caption.
/// Shows an em dash when the value is unavailable, never a made-up number.
struct Metric: View {
    @Environment(\.theme) private var theme
    let label: String
    let value: String?
    var caption = ""
    var color: Color?
    var size: CGFloat = Theme.fontValue
    var body: some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label.uppercased()).font(.system(size: Theme.fontCaption - 4, weight: .semibold)).kerning(0.8)
                .foregroundStyle(theme.secondary).lineLimit(1)
            Text(value ?? "—").font(.system(size: size, weight: .semibold))
                .foregroundStyle(value == nil ? theme.secondary : (color ?? theme.ink))
                .lineLimit(1).minimumScaleFactor(0.5)
            if !caption.isEmpty, value != nil {
                Text(caption).font(.system(size: Theme.fontCaption)).foregroundStyle(theme.secondary).lineLimit(1)
                    .minimumScaleFactor(0.8)
            }
        }
    }
}

/// Muted explanatory text inside a card ("No data", "Set your location in Settings").
struct Note: View {
    @Environment(\.theme) private var theme
    let text: String
    var body: some View {
        Text(text).font(.system(size: Theme.fontBody)).foregroundStyle(theme.secondary)
            .fixedSize(horizontal: false, vertical: true)
    }
}

/// AppButton.qml on TV: flat rounded button, accent-filled when `primary`; focus draws a ring.
struct ChipStyle: ButtonStyle {
    var primary = false
    var wide = false
    func makeBody(configuration: Configuration) -> some View {
        ChipBody(configuration: configuration, primary: primary, wide: wide)
    }

    private struct ChipBody: View {
        @Environment(\.theme) private var theme
        @Environment(\.isFocused) private var focused
        let configuration: Configuration
        let primary: Bool
        let wide: Bool
        var body: some View {
            // Focus must read from across the room: the focused chip turns ink-on-light (the
            // tvOS convention) and grows; a selected (primary) chip keeps an accent ring then.
            configuration.label
                .font(.system(size: Theme.fontBody, weight: focused ? .semibold : .medium))
                .foregroundStyle(focused ? theme.canvas : primary ? theme.accentInk : theme.ink)
                .lineLimit(1)
                .padding(.horizontal, 30).padding(.vertical, 14)
                .frame(maxWidth: wide ? .infinity : nil, alignment: wide ? .leading : .center)
                .background(RoundedRectangle(cornerRadius: 16)
                    .fill(focused ? theme.ink : primary ? theme.accent : theme.surface))
                .overlay(RoundedRectangle(cornerRadius: 16)
                    .strokeBorder(focused && primary ? theme.accent : theme.divider, lineWidth: focused && primary ? 6 : 1.5))
                .shadow(color: .black.opacity(focused ? 0.5 : 0), radius: 14, y: 6)
                .scaleEffect(focused ? 1.06 : (configuration.isPressed ? 0.97 : 1))
                .animation(.easeOut(duration: 0.12), value: focused)
        }
    }
}

/// Modal panel over a dimmed dashboard (Qt Popup): Menu on the remote closes it.
struct Overlay<Content: View>: View {
    @Environment(\.theme) private var theme
    let title: String
    var width: CGFloat = 760
    let close: () -> Void
    @ViewBuilder var content: () -> Content
    var body: some View {
        ZStack {
            Color.black.opacity(0.55).ignoresSafeArea()
            VStack(alignment: .leading, spacing: Theme.gap) {
                Text(title).font(.system(size: Theme.fontTitle + 4, weight: .semibold)).foregroundStyle(theme.ink)
                content()
            }
            .padding(Theme.pad * 1.4)
            .frame(width: width, alignment: .leading)
            .background(RoundedRectangle(cornerRadius: Theme.radius).fill(theme.surfaceRaised))
            .overlay(RoundedRectangle(cornerRadius: Theme.radius).strokeBorder(theme.divider, lineWidth: 1.5))
            .focusSection()
        }
        .onExitCommand(perform: close)
    }
}

/// A focusable dashboard tile driven by the Siri Remote: Select runs `select`, holding Select
/// runs `hold`. A Button (so the focus engine and Select work as on any tvOS control) whose
/// style draws nothing of its own: the card inside shows focus with its ring.
struct RemoteTile<Content: View>: View {
    let select: () -> Void
    let hold: () -> Void
    @ViewBuilder var content: () -> Content
    /// Set when the press became a hold, so the release does not also count as Select.
    @State private var held = false
    var body: some View {
        Button {
            if held { held = false } else { select() }
        } label: { content() }
            .buttonStyle(BareStyle())
            .simultaneousGesture(LongPressGesture(minimumDuration: 0.6).onEnded { _ in held = true; hold() })
    }

    private struct BareStyle: ButtonStyle {
        func makeBody(configuration: Configuration) -> some View {
            configuration.label.opacity(configuration.isPressed ? 0.92 : 1)
        }
    }
}
