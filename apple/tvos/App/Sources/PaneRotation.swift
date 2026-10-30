import Foundation

/// Automatic pane rotation (PaneSlot.qml's Timer) plus manual stepping with the Siri Remote.
/// A rotating slot shows the saved widget, then the next TV widget every `seconds`. Left/right
/// on a focused rotating pane steps it (PaneSlot.step: the stop becomes the saved widget) and
/// restarts the slot's clock there, so the next automatic change is a full interval away.
enum PaneRotation {
    /// Widgets a rotation cycles through: every TV widget, never the empty pane.
    static var stops: [String] { WidgetCatalog.onTV.map(\.id) }

    /// The widget a slot shows at `now`. `restart` is when the slot last stepped by hand (nil:
    /// the rotation runs on the wall clock, so equal intervals change together).
    static func shown(saved: String, seconds: Int, restart: Date?, now: Date, stops: [String] = stops) -> String {
        guard seconds > 0, saved != "none", !stops.isEmpty else { return saved }
        let start = stops.firstIndex(of: saved) ?? 0
        let origin = restart?.timeIntervalSince1970 ?? 0
        let ticks = Int(max(0, now.timeIntervalSince1970 - origin)) / seconds
        return stops[(start + ticks) % stops.count]
    }

    /// The stop `direction` (+1 right, -1 left) away from what is shown now, wrapping around.
    static func step(from shown: String, by direction: Int, stops: [String] = stops) -> String {
        guard !stops.isEmpty else { return shown }
        let at = stops.firstIndex(of: shown) ?? 0
        return stops[(at + direction % stops.count + stops.count) % stops.count]
    }
}
