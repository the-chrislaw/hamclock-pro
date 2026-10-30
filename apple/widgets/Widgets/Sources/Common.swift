import SwiftUI
import WidgetKit

/// Glanceable in a car: few values, large bold type, high contrast on a dark card.
enum Style {
    static let background = Color(red: 0.06, green: 0.07, blue: 0.09)
    static let secondary = Color(white: 0.72)

    static func color(_ s: Severity) -> Color {
        switch s {
        case .calm: return Color(red: 0.30, green: 0.85, blue: 0.40)
        case .elevated: return Color(red: 1.0, green: 0.80, blue: 0.20)
        case .severe: return Color(red: 1.0, green: 0.36, blue: 0.32)
        }
    }

    static func color(_ c: BandCondition) -> Color {
        switch c {
        case .good: return color(Severity.calm)
        case .fair: return color(Severity.elevated)
        case .poor: return color(Severity.severe)
        }
    }

    static func value(_ size: CGFloat) -> Font { .system(size: size, weight: .heavy, design: .rounded) }
    static func label(_ size: CGFloat) -> Font { .system(size: size, weight: .semibold) }
}

/// Header line: widget title and, when not live, a Stale/Offline badge.
struct HeaderRow: View {
    let title: String
    let freshness: Freshness

    var body: some View {
        HStack(spacing: 4) {
            Text(title).font(Style.label(13)).foregroundStyle(Style.secondary)
            Spacer(minLength: 2)
            if let badge = freshness.badge {
                Text(badge.uppercased())
                    .font(.system(size: 11, weight: .bold))
                    .padding(.horizontal, 5).padding(.vertical, 1)
                    .background(Capsule().fill(Style.color(Severity.elevated)))
                    .foregroundStyle(.black)
            }
        }
        .lineLimit(1)
    }
}

/// Footer shown whenever data is not live: when it was last fetched.
struct AgeFooter: View {
    let fetched: Date?
    let freshness: Freshness

    var body: some View {
        if freshness != .live {
            Text(fetched.map { Format.asOf($0) } ?? "Waiting for data")
                .font(Style.label(11)).foregroundStyle(Style.secondary).lineLimit(1)
        }
    }
}

extension View {
    /// Dark card everywhere; removable so StandBy/CarPlay can drop it.
    func hamClockBackground() -> some View {
        containerBackground(Style.background, for: .widget)
    }
}
