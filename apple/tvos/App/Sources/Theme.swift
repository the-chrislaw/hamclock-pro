import SwiftUI

/// Design tokens ported from src/ui/qml/Theme.qml so the TV app reads as the same product.
/// Sizes are for the 1920x1080-point tvOS canvas (docs/apple-tv.md section 3): body text is
/// never below 29 pt.
struct Theme: Equatable {
    var dark: Bool
    var accentStyle: Int

    static func hex(_ v: UInt32, alpha: Double = 1) -> Color {
        Color(.sRGB, red: Double((v >> 16) & 0xff) / 255, green: Double((v >> 8) & 0xff) / 255,
              blue: Double(v & 0xff) / 255, opacity: alpha)
    }

    var canvas: Color { Self.hex(dark ? 0x22282c : 0xeeede9) }
    var surface: Color { Self.hex(dark ? 0x2b3236 : 0xfafaf7) }
    var surfaceRaised: Color { Self.hex(dark ? 0x323a3f : 0xffffff) }
    var ink: Color { Self.hex(dark ? 0xe2e7e6 : 0x303a42) }
    var secondary: Color { Self.hex(dark ? 0x97a4a8 : 0x637074) }
    var divider: Color { Self.hex(dark ? 0x41494b : 0xe0e3df) }
    var accent: Color {
        let darkSet: [UInt32] = [0x9cb6c5, 0x8fc79a, 0xe6c06b, 0xec8f86, 0xb9a8dc]
        let lightSet: [UInt32] = [0x487991, 0x2f7d3f, 0x8a5f00, 0xb3362b, 0x6c4fa0]
        let i = max(0, min(4, accentStyle))
        return Self.hex(dark ? darkSet[i] : lightSet[i])
    }
    var accentInk: Color { Self.hex(dark ? 0x1d2427 : 0xffffff) }
    var focus: Color { Self.hex(dark ? 0xc7dbe6 : 0x2f6077) }
    var good: Color { Self.hex(dark ? 0x8fc79a : 0x2f7d3f) }
    var warn: Color { Self.hex(dark ? 0xe6c06b : 0x9a6a00) }
    var bad: Color { Self.hex(dark ? 0xec8f86 : 0xb3362b) }

    // Map palette from src/map/map_renderer.cpp.
    var sea: Color { Self.hex(dark ? 0x374a51 : 0xdce8eb) }
    var land: Color { Self.hex(dark ? 0x29363b : 0xd6ddce) }
    var coast: Color { Self.hex(dark ? 0x8b9fa3 : 0x647c7e) }
    var route: Color { Self.hex(dark ? 0xb4d9e8 : 0x315e77) }
    var mapInk: Color { Self.hex(dark ? 0xe6eef0 : 0x1b2a30) }
    var mapChip: Color { dark ? Self.hex(0x10181b, alpha: 0.8) : Self.hex(0xffffff, alpha: 0.8) }

    // TV type scale (Theme.qml sizes x ~2.1, floored at 29 pt).
    static let fontCaption: CGFloat = 29
    static let fontBody: CGFloat = 31
    static let fontTitle: CGFloat = 34
    static let fontValue: CGFloat = 56
    static let fontClock: CGFloat = 104
    static let radius: CGFloat = 26
    static let gap: CGFloat = 20
    static let pad: CGFloat = 26

    func stateColor(_ s: WidgetState) -> Color {
        switch s {
        case .live: return good
        case .stale, .loading: return warn
        case .unconfigured: return secondary
        case .offline, .error: return bad
        }
    }
}

enum WidgetState: String {
    case live, stale, loading, offline, error, unconfigured
    var label: String {
        switch self {
        case .live: return "Live"
        case .stale: return "Stale"
        case .loading: return "Loading"
        case .offline: return "Offline"
        case .error: return "Error"
        case .unconfigured: return "Set up"
        }
    }
}

private struct ThemeKey: EnvironmentKey {
    static let defaultValue = Theme(dark: true, accentStyle: 0)
}

extension EnvironmentValues {
    var theme: Theme {
        get { self[ThemeKey.self] }
        set { self[ThemeKey.self] = newValue }
    }
}
