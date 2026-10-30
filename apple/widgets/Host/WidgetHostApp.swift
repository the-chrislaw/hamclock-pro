import SwiftUI

/// Simulator-only stand-in for the Qt app so the widget extension can be installed and
/// registered without a Qt build. Never shipped: the TestFlight job embeds the
/// extension in the real app. Buttons write a test station into the App Group defaults.
@main
struct WidgetHostApp: App {
    var body: some Scene {
        WindowGroup {
            VStack(spacing: 16) {
                Text("HamClock widgets host (Simulator only)").font(.headline)
                Button("Share test station (FN31)") {
                    let d = UserDefaults(suiteName: "group.com.chrislaw.hamclockpro")
                    d?.set(true, forKey: "station.configured")
                    d?.set("N0CALL", forKey: "station.callsign")
                    d?.set(41.5, forKey: "station.lat")
                    d?.set(-73.0, forKey: "station.lon")
                    d?.set(["ISS"], forKey: "satellites.tracked")
                }
                Button("Clear station") {
                    UserDefaults(suiteName: "group.com.chrislaw.hamclockpro")?.set(false, forKey: "station.configured")
                }
            }
            .padding()
        }
    }
}
